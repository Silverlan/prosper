/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_context.hpp"
#include "image/prosper_texture.hpp"
#include "prosper_util.hpp"
#include "shader/prosper_shader_blur.hpp"
#include "shader/prosper_shader_copy_image.hpp"
#include "prosper_util_square_shape.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "buffers/prosper_buffer.hpp"
#include "image/prosper_sampler.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_fence.hpp"
#include "prosper_pipeline_cache.hpp"
#include <misc/buffer_create_info.h>
#include <misc/fence_create_info.h>
#include <misc/semaphore_create_info.h>
#include <misc/framebuffer_create_info.h>
#include <misc/swapchain_create_info.h>
#include <misc/rendering_surface_create_info.h>

/* Uncomment the #define below to enable off-screen rendering */
// #define ENABLE_OFFSCREEN_RENDERING

#include <string>
#include <cmath>
#include <iglfw/glfw_window.h>

#ifdef _WIN32
	#define GLFW_EXPOSE_NATIVE_WIN32
#else
	#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3native.h>

/* Sanity checks */
#if defined(_WIN32)
	#if !defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT) && !defined(ENABLE_OFFSCREEN_RENDERING)
		#error Anvil has not been built with Win32/64 window system support. The application can only be built in offscreen rendering mode.
	#endif
#else
	#if !defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT) && !defined(ENABLE_OFFSCREEN_RENDERING)
		#error Anvil has not been built with XCB window system support. The application can only be built in offscreen rendering mode.
	#endif
#endif

using namespace prosper;

REGISTER_BASIC_BITWISE_OPERATORS(prosper::Context::StateFlags)

#pragma optimize("",off)
decltype(Context::s_devToContext) Context::s_devToContext = {};

const std::string PIPELINE_CACHE_PATH = "cache/shader.cache";

Context &Context::GetContext(Anvil::BaseDevice &dev)
{
	auto it = s_devToContext.find(&dev);
	if(it == s_devToContext.end())
		throw std::runtime_error("Invalid context for device!");
	return *it->second;
}

static std::shared_ptr<prosper::Buffer> s_vertexBuffer = nullptr;
static std::shared_ptr<prosper::Buffer> s_uvBuffer = nullptr;
static std::shared_ptr<prosper::Buffer> s_vertexUvBuffer = nullptr;
Context::Context(const std::string &appName,bool bEnableValidation)
	: m_lastSemaporeUsed(0),m_appName(appName),
	m_windowCreationInfo(std::make_unique<GLFW::WindowCreationInfo>())
{
	umath::set_flag(m_stateFlags,StateFlags::ValidationEnabled,bEnableValidation);
	m_windowCreationInfo->title = appName;
}

Context::~Context() {Release();}

void Context::SetValidationEnabled(bool b) {umath::set_flag(m_stateFlags,StateFlags::ValidationEnabled,b);}
bool Context::IsValidationEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::ValidationEnabled);}

Anvil::SGPUDevice &Context::GetDevice() {return *m_pGpuDevice;}
const std::shared_ptr<Anvil::RenderPass> &Context::GetMainRenderPass() const {return m_renderPass;}
Anvil::SubPassID Context::GetMainSubPassID() const {return m_mainSubPass;}
std::shared_ptr<Anvil::Swapchain> Context::GetSwapchain() {return m_swapchainPtr;}

const std::string &Context::GetAppName() const {return m_appName;}
uint32_t Context::GetSwapchainImageCount() const {return m_numSwapchainImages;}

GLFW::Window &Context::GetWindow() {return *m_glfwWindow;}
std::array<uint32_t,2> Context::GetWindowSize() const {return {m_windowCreationInfo->width,m_windowCreationInfo->height};}
uint32_t Context::GetWindowWidth() const {return m_windowCreationInfo->width;}
uint32_t Context::GetWindowHeight() const {return m_windowCreationInfo->height;}

void Context::Release()
{
	if(m_devicePtr == nullptr)
		return;
	WaitIdle();

	ReleaseSwapchain();

	auto &dev = GetDevice();
	auto it = s_devToContext.find(&dev);
	s_vertexBuffer = nullptr;
	s_uvBuffer = nullptr;
	s_vertexUvBuffer = nullptr;
	if(it != s_devToContext.end())
		s_devToContext.erase(it);

	m_shaderManager = nullptr;
	m_dummyTexture = nullptr;
	m_dummyCubemapTexture = nullptr;
	m_dummyBuffer = nullptr;
	m_setupCmdBuffer = nullptr;
	m_keepAliveResources.clear();
	while(m_scheduledBufferUpdates.empty() == false)
		m_scheduledBufferUpdates.pop();

	m_devicePtr.reset(); // All Vulkan resources related to the device have to be cleared at this point!

	vkDestroySurfaceKHR(m_instancePtr->get_instance_vk(),m_surface,nullptr);

	m_instancePtr.reset();

	m_windowPtr.reset();

	m_glfwWindow = nullptr;
}

uint64_t Context::GetLastFrameId() const {return m_frameId;}

void Context::EndFrame() {++m_frameId;}

vk::Result Context::WaitForFence(const Fence &fence,uint64_t timeout) const
{
	auto vkFence = fence.GetAnvilFence().get_fence();
	return static_cast<vk::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),1,&vkFence,true,timeout));
}

vk::Result Context::WaitForFences(const std::vector<Fence> &fences,bool waitAll,uint64_t timeout) const
{
	std::vector<VkFence> vkFences {};
	vkFences.reserve(fences.size());
	for(auto &fence : fences)
		vkFences.push_back(fence->get_fence());
	return static_cast<vk::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),vkFences.size(),vkFences.data(),waitAll,timeout));
}

void Context::DrawFrame(const std::function<void(const std::shared_ptr<prosper::PrimaryCommandBuffer>&,uint32_t)> &drawFrame)
{
	Anvil::Semaphore *curr_frame_signal_semaphore_ptr;
	Anvil::Semaphore *curr_frame_wait_semaphore_ptr;
	static uint32_t n_frames_rendered = 0;
	auto *present_queue_ptr = m_devicePtr->get_universal_queue(0);
	Anvil::PipelineStageFlags wait_stage_mask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    m_lastSemaporeUsed = (m_lastSemaporeUsed +1) %m_numSwapchainImages;

    curr_frame_signal_semaphore_ptr = m_frameSignalSemaphores[m_lastSemaporeUsed].get();
    curr_frame_wait_semaphore_ptr = m_frameWaitSemaphores[m_lastSemaporeUsed].get();

    /* Determine the semaphore which the swapchain image */
    auto errCode = m_swapchainPtr->acquire_image(curr_frame_wait_semaphore_ptr,&m_n_swapchain_image);

	if(
		errCode != Anvil::SwapchainOperationErrorCode::SUCCESS ||
		static_cast<vk::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),1,m_cmdFences[m_n_swapchain_image]->get_fence_ptr(),true,std::numeric_limits<uint64_t>::max())) != vk::Result::eSuccess ||
		m_cmdFences[m_n_swapchain_image]->reset() == false
	)
		throw std::runtime_error("Unable to reset fence.");
	
	ClearKeepAliveResources();

	//auto &keepAliveResources = m_keepAliveResources.at(m_n_swapchain_image);
	//auto numKeepAliveResources = keepAliveResources.size(); // We can clear the resources from the previous render pass of this swapchain after we've waited for the semaphore (i.e. after the frame rendering is complete)
	
	auto &cmd_buffer_ptr = m_commandBuffers.at(m_n_swapchain_image);
	/* Start recording commands */
	static_cast<Anvil::PrimaryCommandBuffer&>(cmd_buffer_ptr->GetAnvilCommandBuffer()).start_recording(false,true);
	umath::set_flag(m_stateFlags,StateFlags::IsRecording);
	umath::set_flag(m_stateFlags,StateFlags::Idle,false);
	while(m_scheduledBufferUpdates.empty() == false)
	{
		auto &f = m_scheduledBufferUpdates.front();
		f(*cmd_buffer_ptr);
		m_scheduledBufferUpdates.pop();
	}
	drawFrame(GetDrawCommandBuffer(),m_n_swapchain_image);
	/* Close the recording process */
	umath::set_flag(m_stateFlags,StateFlags::IsRecording,false);
	static_cast<Anvil::PrimaryCommandBuffer&>(cmd_buffer_ptr->GetAnvilCommandBuffer()).stop_recording();


    /* Submit work chunk and present */
	auto *signalSemaphore = curr_frame_signal_semaphore_ptr;
	auto *waitSemaphore = curr_frame_wait_semaphore_ptr;
    m_devicePtr->get_universal_queue(0)->submit(Anvil::SubmitInfo::create(
		&m_commandBuffers[m_n_swapchain_image]->GetAnvilCommandBuffer(),
		1, /* n_semaphores_to_signal */
		&signalSemaphore,
		1, /* n_semaphores_to_wait_on */
		&waitSemaphore,
		&wait_stage_mask,
		false, /* should_block  */
		m_cmdFences.at(m_n_swapchain_image).get()
	)); /* opt_fence_ptr */

	//ClearKeepAliveResources(numKeepAliveResources);

	present_queue_ptr->present(
		m_swapchainPtr.get(),
		m_n_swapchain_image,
		1, /* n_wait_semaphores */
		&signalSemaphore,
		&errCode
	);
	if(errCode != Anvil::SwapchainOperationErrorCode::SUCCESS)
	{
		std::cout<<"ERROR!"<<std::endl;
		if(errCode == Anvil::SwapchainOperationErrorCode::OUT_OF_DATE)
		{
			ChangeResolution(m_windowCreationInfo->width,m_windowCreationInfo->height); // prosper TODO: Force reload
			/*InitVulkan();
			auto hWindow = glfwGetWin32Window(const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()));
			m_windowPtr = Anvil::WindowFactory::create_window(Anvil::WINDOW_PLATFORM_SYSTEM,hWindow);
			VkSurfaceKHR surface = nullptr;
			auto err = glfwCreateWindowSurface(
				m_instancePtr->get_instance_vk(),
				const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()),
				nullptr,
				reinterpret_cast<VkSurfaceKHR*>(&surface)
			);
			InitSwapchain();*/
		}
		else
			; // Terminal error?
	}
	//m_glfwWindow->SwapBuffers();
}

void Context::DrawFrame()
{
	DrawFrame([this](const std::shared_ptr<prosper::PrimaryCommandBuffer>&,uint32_t m_n_swapchain_image) {
		Draw(m_n_swapchain_image);
	});
	EndFrame();
}

void Context::ReleaseSwapchain()
{
	m_renderPass.reset();
	m_swapchainPtr.reset();
	m_renderingSurfacePtr.reset();
	for(auto &fbo : m_fbos)
		fbo.reset();

	m_commandBuffers.clear();
	m_cmdFences.clear();
	m_fbos.clear();
	m_frameSignalSemaphores.clear();
	m_frameWaitSemaphores.clear();
}

void Context::ReloadWindow()
{
	WaitIdle();
	ReleaseSwapchain();
	InitWindow();
	ReloadSwapchain();
}

void Context::ReloadSwapchain()
{
	// Reload swapchain related objects
	WaitIdle();
	ReleaseSwapchain();
	InitSwapchain();
	InitFrameBuffers();
	InitCommandBuffers();
	InitSemaphores();
	InitMainRenderPass();
	OnSwapchainInitialized();

	if(m_shaderManager != nullptr)
	{
		auto &shaderManager = *m_shaderManager;
		for(auto &pair : shaderManager.GetShaders())
		{
			if(pair.second->IsGraphicsShader() == false)
				continue;
			auto &shader = static_cast<prosper::ShaderGraphics&>(*pair.second);
			auto numPipelines = shader.GetPipelineCount();
			auto bHasStaticViewportOrScissor = false;
			for(auto i=decltype(numPipelines){0};i<numPipelines;++i)
			{
				auto *baseInfo = shader.GetPipelineInfo(i);
				if(baseInfo == nullptr)
					continue;
				auto &info = static_cast<const Anvil::GraphicsPipelineCreateInfo&>(*baseInfo);
				if(info.get_n_scissor_boxes() == 0u && info.get_n_viewports() == 0u)
					continue;
				bHasStaticViewportOrScissor = true;
				break;
			}
			if(bHasStaticViewportOrScissor == false)
				continue;
			shader.ReloadPipelines();
		}
	}
}

void Context::ChangeResolution(uint32_t width,uint32_t height)
{
	if(width == m_windowCreationInfo->width && height == m_windowCreationInfo->height)
		return;
	m_windowCreationInfo->width = width;
	m_windowCreationInfo->height = height;
	if(umath::is_flag_set(m_stateFlags,StateFlags::Initialized) == false)
		return;
	WaitIdle();
	m_glfwWindow->SetSize(Vector2i(width,height));
	ReloadSwapchain();
	OnResolutionChanged(width,height);
}

void Context::OnResolutionChanged(uint32_t width,uint32_t height) {}
void Context::OnWindowInitialized() {}
void Context::OnSwapchainInitialized() {}

void Context::SetPresentMode(Anvil::PresentModeKHR presentMode)
{
	if(IsPresentationModeSupported(presentMode) == false)
	{
		switch(presentMode)
		{
			case Anvil::PresentModeKHR::IMMEDIATE_KHR:
				throw std::runtime_error("Immediate present mode not supported!");
			default:
				return SetPresentMode(Anvil::PresentModeKHR::FIFO_KHR);
				
		}
	}
	m_presentMode = presentMode;
}
Anvil::PresentModeKHR Context::GetPresentMode() const {return m_presentMode;}
void Context::ChangePresentMode(Anvil::PresentModeKHR presentMode)
{
	auto oldPresentMode = GetPresentMode();
	SetPresentMode(presentMode);
	if(oldPresentMode == GetPresentMode() || umath::is_flag_set(m_stateFlags,StateFlags::Initialized) == false)
		return;
	WaitIdle();
	ReloadSwapchain();
}

bool Context::IsPresentationModeSupported(Anvil::PresentModeKHR presentMode) const
{
	if(m_renderingSurfacePtr == nullptr)
		return true;
	auto res = false;
	m_renderingSurfacePtr->supports_presentation_mode(m_physicalDevicePtr,presentMode,&res);
	return res;
}

Vendor Context::GetPhysicalDeviceVendor() const
{
	return static_cast<Vendor>(const_cast<Context*>(this)->GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->vendor_id);
}

bool Context::GetSurfaceCapabilities(Anvil::SurfaceCapabilities &caps) const
{
	return const_cast<Context*>(this)->GetDevice().get_physical_device_surface_capabilities(m_renderingSurfacePtr.get(),&caps);
}

void Context::GetScissorViewportInfo(VkRect2D *out_scissors,VkViewport *out_viewports)
{
	auto width = GetWindowWidth();
	auto height = GetWindowHeight();
	auto min_size = (height > width) ? width : height;
	auto x_delta = (width -min_size) /2;
	auto y_delta = (height -min_size) /2;

	if(out_scissors != nullptr)
	{
		/* Top-left region */
		out_scissors[0].extent.height = height /2 -y_delta;
		out_scissors[0].extent.width = width /2 -x_delta;
		out_scissors[0].offset.x = 0;
		out_scissors[0].offset.y = 0;

		/* Top-right region */
		out_scissors[1] = out_scissors[0];
		out_scissors[1].offset.x = width /2 +x_delta;

		/* Bottom-left region */
		out_scissors[2] = out_scissors[0];
		out_scissors[2].offset.y = height /2 +y_delta;

		/* Bottom-right region */
		out_scissors[3] = out_scissors[2];
		out_scissors[3].offset.x = width /2 +x_delta;
	}

	if(out_viewports != nullptr)
	{
		/* Top-left region */
		out_viewports[0].height = float(height /2 -y_delta);
		out_viewports[0].maxDepth = 1.0f;
		out_viewports[0].minDepth = 0.0f;
		out_viewports[0].width = float(width /2 -x_delta);
		out_viewports[0].x = 0.0f;
		out_viewports[0].y = 0.0f;

		/* Top-right region */
		out_viewports[1] = out_viewports[0];
		out_viewports[1].x = float(width /2 +x_delta);

		/* Bottom-left region */
		out_viewports[2] = out_viewports[0];
		out_viewports[2].y = float(height /2 +y_delta);

		/* Bottom-right region */
		out_viewports[3] = out_viewports[2];
		out_viewports[3].x = float(width /2 +x_delta);
	}
}

const std::shared_ptr<prosper::PrimaryCommandBuffer> &Context::GetSetupCommandBuffer()
{
	if(m_setupCmdBuffer != nullptr)
		return m_setupCmdBuffer;
	auto &dev = GetDevice();
	m_setupCmdBuffer = prosper::PrimaryCommandBuffer::Create(*this,dev.get_command_pool_for_queue_family_index(dev.get_universal_queue(0)->get_queue_family_index())->alloc_primary_level_command_buffer(),Anvil::QueueFamilyType::UNIVERSAL);
	if(static_cast<Anvil::PrimaryCommandBuffer&>(m_setupCmdBuffer->GetAnvilCommandBuffer()).start_recording(true,false) == false)
		throw std::runtime_error("Unable to start recording for primary level command buffer!");
	return m_setupCmdBuffer;
}

const std::shared_ptr<prosper::PrimaryCommandBuffer> &Context::GetDrawCommandBuffer() const {return m_commandBuffers.at(m_n_swapchain_image);}

const std::shared_ptr<prosper::PrimaryCommandBuffer> &Context::GetDrawCommandBuffer(uint32_t swapchainIdx) const
{
	static std::shared_ptr<prosper::PrimaryCommandBuffer> nptr = nullptr;
	return (swapchainIdx < m_commandBuffers.size()) ? m_commandBuffers.at(swapchainIdx) : nptr;
}

void Context::FlushSetupCommandBuffer()
{
	if(m_setupCmdBuffer == nullptr)
		return;
	auto bSuccess = static_cast<Anvil::PrimaryCommandBuffer&>(m_setupCmdBuffer->GetAnvilCommandBuffer()).stop_recording();
	auto &dev = GetDevice();
	dev.get_universal_queue(0)->submit(Anvil::SubmitInfo::create(
		&m_setupCmdBuffer->GetAnvilCommandBuffer(),0u,nullptr,
		0u,nullptr,nullptr,true
	));
	m_setupCmdBuffer = nullptr;
}

void Context::ClearKeepAliveResources(uint32_t n)
{
	if(m_n_swapchain_image >= m_keepAliveResources.size())
		return;
	auto &resources = m_keepAliveResources.at(m_n_swapchain_image);
	n = umath::min(static_cast<size_t>(n),resources.size());
	while(n-- > 0u)
		resources.erase(resources.begin());
}
void Context::ClearKeepAliveResources()
{
	if(m_n_swapchain_image >= m_keepAliveResources.size())
		return;
	auto &resources = m_keepAliveResources.at(m_n_swapchain_image);
	resources.clear();
}
void Context::KeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::Idle))
		return; // No need to keep resource around if device is currently idling (i.e. nothing is in progress)
	m_keepAliveResources.at(m_n_swapchain_image).push_back(resource);
}

void Context::WaitIdle()
{
	FlushSetupCommandBuffer();
	auto &dev = GetDevice();
	dev.wait_idle();
	umath::set_flag(m_stateFlags,StateFlags::Idle);
	ClearKeepAliveResources();
}

void Context::Initialize(const CreateInfo &createInfo)
{
	// TODO: Check if resolution is supported
	m_windowCreationInfo->width = createInfo.width;
	m_windowCreationInfo->height = createInfo.height;
	InitVulkan(createInfo);
	InitWindow();
	ReloadSwapchain();
	InitBuffers();
	InitGfxPipelines();
	InitDummyTextures();
	InitDummyBuffer();
	umath::set_flag(m_stateFlags,StateFlags::Initialized);
	auto &dev = GetDevice();
	s_vertexBuffer = prosper::util::get_square_vertex_buffer(dev);
	s_uvBuffer = prosper::util::get_square_uv_buffer(dev);
	s_vertexUvBuffer = prosper::util::get_square_vertex_uv_buffer(dev);

	auto &shaderManager = GetShaderManager();
	shaderManager.RegisterShader("copy_image",[](prosper::Context &context,const std::string &identifier) {return new ShaderCopyImage(context,identifier);});
	shaderManager.RegisterShader("blur_horizontal",[](prosper::Context &context,const std::string &identifier) {return new ShaderBlurH(context,identifier);});
	shaderManager.RegisterShader("blur_vertical",[](prosper::Context &context,const std::string &identifier) {return new ShaderBlurV(context,identifier);});
}

bool Context::IsImageFormatSupported(Anvil::Format format,Anvil::ImageUsageFlags usageFlags,Anvil::ImageType type,Anvil::ImageTiling tiling) const
{
	return m_devicePtr->get_physical_device_image_format_properties(
		Anvil::ImageFormatPropertiesQuery{
			format,type,tiling,
			usageFlags,{}
		}
	);
}

void Context::InitBuffers() {}

void Context::DrawFrame(prosper::PrimaryCommandBuffer &cmd_buffer_ptr,uint32_t n_current_swapchain_image) {}

ShaderManager &Context::GetShaderManager() const {return *m_shaderManager;}

::util::WeakHandle<Shader> Context::RegisterShader(const std::string &identifier,const std::function<Shader*(Context&,const std::string&)> &fFactory) {return m_shaderManager->RegisterShader(identifier,fFactory);}
::util::WeakHandle<Shader> Context::GetShader(const std::string &identifier) const {return m_shaderManager->GetShader(identifier);}

const std::shared_ptr<Texture> &Context::GetDummyTexture() const {return m_dummyTexture;}
const std::shared_ptr<Texture> &Context::GetDummyCubemapTexture() const {return m_dummyCubemapTexture;}
const std::shared_ptr<Buffer> &Context::GetDummyBuffer() const {return m_dummyBuffer;}
void Context::InitDummyBuffer()
{
	auto &dev = GetDevice();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.size = 1ull;
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT | Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
	m_dummyBuffer = prosper::util::create_buffer(dev,createInfo);
	m_dummyBuffer->SetDebugName("context_dummy_buf");
}
void Context::InitDummyTextures()
{
	auto &dev = GetDevice();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.format = Anvil::Format::R8G8B8A8_UNORM;
	createInfo.height = 1u;
	createInfo.width = 1u;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.postCreateLayout = Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
	createInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT;
	auto img = prosper::util::create_image(dev,createInfo);
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	prosper::util::SamplerCreateInfo samplerCreateInfo {};
	m_dummyTexture = prosper::util::create_texture(dev,{},std::move(img),&imgViewCreateInfo,&samplerCreateInfo);
	m_dummyTexture->SetDebugName("context_dummy_tex");

	createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;
	createInfo.layers = 6u;
	auto imgCubemap = prosper::util::create_image(dev,createInfo);
	m_dummyCubemapTexture = prosper::util::create_texture(dev,{},std::move(imgCubemap),&imgViewCreateInfo,&samplerCreateInfo);
	m_dummyCubemapTexture->SetDebugName("context_dummy_cubemap_tex");
}

void Context::SubmitCommandBuffer(prosper::CommandBuffer &cmd,bool shouldBlock,Anvil::Fence *fence) {SubmitCommandBuffer(cmd.GetAnvilCommandBuffer(),cmd.GetQueueFamilyType(),shouldBlock,fence);}
void Context::SubmitCommandBuffer(Anvil::CommandBufferBase &cmd,Anvil::QueueFamilyType queueFamilyType,bool shouldBlock,Anvil::Fence *fence)
{
	switch(queueFamilyType)
	{
		case Anvil::QueueFamilyType::UNIVERSAL:
			m_devicePtr->get_universal_queue(0u)->submit(Anvil::SubmitInfo::create(
				&cmd,0u,nullptr,0u,nullptr,nullptr,shouldBlock,fence
			));
			break;
		case Anvil::QueueFamilyType::COMPUTE:
			m_devicePtr->get_compute_queue(0u)->submit(Anvil::SubmitInfo::create(
				&cmd,0u,nullptr,0u,nullptr,nullptr,shouldBlock,fence
			));
			break;
		default:
			throw std::invalid_argument("No device queue exists for queue family " +std::to_string(umath::to_integral(queueFamilyType)) +"!");
	}
}

bool Context::IsRecording() const {return umath::is_flag_set(m_stateFlags,StateFlags::IsRecording);}
bool Context::ScheduleRecordUpdateBuffer(const std::shared_ptr<Buffer> &buffer,uint64_t offset,uint64_t size,const void *data,Anvil::PipelineStageFlags srcStageMask,Anvil::AccessFlags srcAccessMask)
{
	if(size == 0u)
		return true;
	if((buffer->GetBaseAnvilBuffer().get_create_info_ptr()->get_usage_flags() &Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT) == Anvil::BufferUsageFlagBits::NONE)
		throw std::logic_error("Buffer has to have been created with VK_BUFFER_USAGE_TRANSFER_DST_BIT flag to allow buffer updates on command buffer!");
	const auto fRecordSrcBarrier = [this,buffer,srcStageMask,srcAccessMask,offset,size](prosper::CommandBuffer &cmdBuffer) {
		if(srcStageMask != Anvil::PipelineStageFlagBits::NONE)
		{
			prosper::util::record_buffer_barrier(
				*cmdBuffer,*buffer,
				srcStageMask,Anvil::PipelineStageFlagBits::TRANSFER_BIT,
				srcAccessMask,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
				offset,size
			);
		}
	};
	const auto fUpdateBuffer = [](prosper::CommandBuffer &cmdBuffer,prosper::Buffer &buffer,const uint8_t *data,uint64_t offset,uint64_t size) {
		auto dataOffset = 0ull;
		const auto maxUpdateSize = 65'536u; // Maximum size allowed per vkCmdUpdateBuffer-call (see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html)
		do
		{
			auto updateSize = umath::min(static_cast<uint64_t>(maxUpdateSize),size);
			if(prosper::util::record_update_buffer(cmdBuffer.GetAnvilCommandBuffer(),buffer,offset,updateSize,data +dataOffset) == false)
				return false;
			dataOffset += maxUpdateSize;
			offset += maxUpdateSize;
			size -= updateSize;
		}
		while(size > 0ull);
		return true;
	};

#ifdef _DEBUG
	auto fCheckMask = [](vk::AccessFlags srcAccessMask,vk::PipelineStageFlags srcStageMask,vk::AccessFlagBits accessFlag,vk::PipelineStageFlags validStageMask) {
		if(!(srcAccessMask &accessFlag))
			return true;
		return (srcStageMask &validStageMask) != vk::PipelineStageFlagBits{};
	};
	auto fCheckMasks = [&bufferUpdateInfo,&fCheckMask](vk::AccessFlags srcAccessMask,vk::PipelineStageFlags srcStageMask) {
		const auto shaderStageMask = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader | vk::PipelineStageFlagBits::eGeometryShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
		if(
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eIndirectCommandRead,vk::PipelineStageFlagBits::eDrawIndirect) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eIndexRead,vk::PipelineStageFlagBits::eVertexInput) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eVertexAttributeRead,vk::PipelineStageFlagBits::eVertexInput) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eUniformRead,shaderStageMask) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eInputAttachmentRead,vk::PipelineStageFlagBits::eFragmentShader) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eShaderRead,shaderStageMask) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eShaderWrite,shaderStageMask) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eColorAttachmentRead,vk::PipelineStageFlagBits::eColorAttachmentOutput) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eColorAttachmentWrite,vk::PipelineStageFlagBits::eColorAttachmentOutput) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eDepthStencilAttachmentRead,vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eDepthStencilAttachmentWrite,vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eTransferRead,vk::PipelineStageFlagBits::eTransfer) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eTransferWrite,vk::PipelineStageFlagBits::eTransfer) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eHostRead,vk::PipelineStageFlagBits::eHost) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eHostWrite,vk::PipelineStageFlagBits::eHost) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eMemoryRead,vk::PipelineStageFlagBits{}) ||
			!fCheckMask(srcAccessMask,srcStageMask,vk::AccessFlagBits::eMemoryWrite,vk::PipelineStageFlagBits{})
		)
			throw std::logic_error("Barrier stage mask is not compatible with barrier access mask!");
	};
	fCheckMasks(bufferUpdateInfo.srcAccessMask,bufferUpdateInfo.srcStageMask);
	fCheckMasks(bufferUpdateInfo.dstAccessMask,bufferUpdateInfo.dstStageMask);
#endif

	if(IsRecording())
	{
		auto &drawCmd = GetDrawCommandBuffer();
		if(prosper::util::get_current_render_pass_target(**drawCmd) == nullptr) // Buffer updates are not allowed while a render pass is active! (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html)
		{
			fRecordSrcBarrier(*drawCmd);
			if(fUpdateBuffer(*drawCmd,*buffer,static_cast<const uint8_t*>(data),offset,size) == false)
				return false;
			return true;
		}
	}
	// We'll have to make a copy of the data for later
	auto ptrData = std::make_shared<std::vector<uint8_t>>(size);
	memcpy(ptrData->data(),data,size);
	m_scheduledBufferUpdates.push([buffer,fRecordSrcBarrier,fUpdateBuffer,offset,size,ptrData](prosper::CommandBuffer &cmdBuffer) {
		fRecordSrcBarrier(cmdBuffer);
		fUpdateBuffer(cmdBuffer,*buffer,ptrData->data(),offset,size);
	});
	return true;
}

bool Context::GetUniversalQueueFamilyIndex(Anvil::QueueFamilyType queueFamilyType,uint32_t &queueFamilyIndex) const
{
    auto n_universal_queue_family_indices = 0u;
    const uint32_t *universal_queue_family_indices = nullptr;
    if(m_devicePtr->get_queue_family_indices_for_queue_family_type(
		queueFamilyType,
		&n_universal_queue_family_indices,
		&universal_queue_family_indices
	) == false || n_universal_queue_family_indices == 0u)
		return false;
	queueFamilyIndex = universal_queue_family_indices[0];
	return true;
}
std::shared_ptr<prosper::PrimaryCommandBuffer> Context::AllocatePrimaryLevelCommandBuffer(Anvil::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)
{
	if(GetUniversalQueueFamilyIndex(queueFamilyType,universalQueueFamilyIndex) == false)
	{
		if(queueFamilyType != Anvil::QueueFamilyType::UNIVERSAL)
			return AllocatePrimaryLevelCommandBuffer(Anvil::QueueFamilyType::UNIVERSAL,universalQueueFamilyIndex);
		return nullptr;
	}
	return prosper::PrimaryCommandBuffer::Create(*this,GetDevice().get_command_pool_for_queue_family_index(universalQueueFamilyIndex)->alloc_primary_level_command_buffer(),queueFamilyType);
}
std::shared_ptr<prosper::SecondaryCommandBuffer> Context::AllocateSecondaryLevelCommandBuffer(Anvil::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)
{
	if(GetUniversalQueueFamilyIndex(queueFamilyType,universalQueueFamilyIndex) == false)
	{
		if(queueFamilyType != Anvil::QueueFamilyType::UNIVERSAL)
			return AllocateSecondaryLevelCommandBuffer(Anvil::QueueFamilyType::UNIVERSAL,universalQueueFamilyIndex);
		return nullptr;
	}
	return prosper::SecondaryCommandBuffer::Create(*this,GetDevice().get_command_pool_for_queue_family_index(universalQueueFamilyIndex)->alloc_secondary_level_command_buffer(),queueFamilyType);
}

void Context::InitMainRenderPass()
{
	Anvil::RenderPassAttachmentID render_pass_color_attachment_id;
	VkRect2D scissors[4];

	GetScissorViewportInfo(scissors,nullptr); /* viewports */

	std::unique_ptr<Anvil::RenderPassCreateInfo> render_pass_info_ptr(new Anvil::RenderPassCreateInfo(m_devicePtr.get()));

	render_pass_info_ptr->add_color_attachment(m_swapchainPtr->get_create_info_ptr()->get_format(),
		Anvil::SampleCountFlagBits::_1_BIT,
		Anvil::AttachmentLoadOp::CLEAR,
		Anvil::AttachmentStoreOp::STORE,
		Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
		Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
		false, /* may_alias */
		&render_pass_color_attachment_id
	);
	render_pass_info_ptr->add_subpass(&m_mainSubPass);
	render_pass_info_ptr->add_subpass_color_attachment(
		m_mainSubPass,
		Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
		render_pass_color_attachment_id,
		0, /* in_location */
		nullptr
	); /* in_opt_attachment_resolve_id_ptr */

	m_renderPass = Anvil::RenderPass::create(std::move(render_pass_info_ptr),m_swapchainPtr.get()); // TODO: Deprecated? Remove main render pass entirely

	m_renderPass->set_name("Main renderpass");
}

void Context::Draw(uint32_t n_swapchain_image)
{
	{
#if 0
		/* Switch the swap-chain image layout to renderable */
		{
			Anvil::ImageBarrier image_barrier(
				0, /* source_access_mask */
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				false,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				universal_queue_ptr->get_queue_family_index(),
				universal_queue_ptr->get_queue_family_index(),
				m_swapchainPtr->get_image(n_swapchain_image),
				subresource_range
			);

			cmd_buffer_ptr->record_pipeline_barrier(
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_FALSE, /* in_by_region */
				0, /* in_memory_barrier_count */
				nullptr, /* in_memory_barrier_ptrs */
				0, /* in_buffer_memory_barrier_count */
				nullptr, /* in_buffer_memory_barrier_ptrs */
				1, /* in_image_memory_barrier_count  */
				&image_barrier
			);
        }
#endif // TODO
		auto &cmd_buffer_ptr = m_commandBuffers.at(n_swapchain_image);
		DrawFrame(*cmd_buffer_ptr,n_swapchain_image);

#if 0
		/* Change the swap-chain image's layout to presentable */
		{
			Anvil::ImageBarrier image_barrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, /* source_access_mask */
				VK_ACCESS_MEMORY_READ_BIT, /* destination_access_mask */
				false,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /* old_image_layout */
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, /* new_image_layout */
				universal_queue_ptr->get_queue_family_index(),
				universal_queue_ptr->get_queue_family_index(),
				m_swapchainPtr->get_image(n_swapchain_image),
				subresource_range
			);

			cmd_buffer_ptr->record_pipeline_barrier(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_FALSE, /* in_by_region */
				0, /* in_memory_barrier_count */
				nullptr, /* in_memory_barrier_ptrs */
				0, /* in_buffer_memory_barrier_count */
				nullptr, /* in_buffer_memory_barrier_ptrs */
				1, /* in_image_memory_barrier_count */
				&image_barrier
			);
		}
#endif // TODO
	}
}

void Context::InitCommandBuffers()
{
	VkImageSubresourceRange subresource_range;
	auto *universal_queue_ptr = m_devicePtr->get_universal_queue(0);

	subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseArrayLayer = 0;
	subresource_range.baseMipLevel = 0;
	subresource_range.layerCount = 1;
	subresource_range.levelCount = 1;

    /* Set up rendering command buffers. We need one per swap-chain image. */
    uint32_t        n_universal_queue_family_indices = 0;
    const uint32_t* universal_queue_family_indices   = nullptr;

    m_devicePtr->get_queue_family_indices_for_queue_family_type(
		Anvil::QueueFamilyType::UNIVERSAL,
		&n_universal_queue_family_indices,
		&universal_queue_family_indices
	);

    /* Set up rendering command buffers. We need one per swap-chain image. */
	for(auto n_current_swapchain_image=0u;n_current_swapchain_image < m_commandBuffers.size();++n_current_swapchain_image)
	{
		auto cmd_buffer_ptr = prosper::PrimaryCommandBuffer::Create(*this,m_devicePtr->get_command_pool_for_queue_family_index(universal_queue_family_indices[0])->alloc_primary_level_command_buffer(),Anvil::QueueFamilyType::UNIVERSAL);
		cmd_buffer_ptr->SetDebugName("swapchain_cmd" +std::to_string(n_current_swapchain_image));
		//m_devicePtr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();

		m_commandBuffers[n_current_swapchain_image] = cmd_buffer_ptr;
		m_cmdFences[n_current_swapchain_image] = Anvil::Fence::create(Anvil::FenceCreateInfo::create(m_devicePtr.get(),true));
	}
}

void Context::InitFrameBuffers()
{
	bool result;
	for(uint32_t n_swapchain_image=0;n_swapchain_image<m_numSwapchainImages;++n_swapchain_image)
	{
		auto createinfo = Anvil::FramebufferCreateInfo::create(
			m_devicePtr.get(),
			GetWindowWidth(),
			GetWindowHeight(),
			1
		);
		result = createinfo->add_attachment(m_swapchainPtr->get_image_view(n_swapchain_image),nullptr /* out_opt_attachment_id_ptrs */);
		anvil_assert(result);
		m_fbos[n_swapchain_image] = Anvil::Framebuffer::create(std::move(createinfo));

		m_fbos[n_swapchain_image]->set_name_formatted("Framebuffer used to render to swapchain image [%d]",n_swapchain_image);
	}
}

void Context::InitGfxPipelines() {}

void Context::InitSemaphores()
{
	for(auto n_semaphore=0u;n_semaphore < m_numSwapchainImages;++n_semaphore)
	{
		auto new_signal_semaphore_ptr = Anvil::Semaphore::create(Anvil::SemaphoreCreateInfo::create(m_devicePtr.get()));
		auto new_wait_semaphore_ptr = Anvil::Semaphore::create(Anvil::SemaphoreCreateInfo::create(m_devicePtr.get()));

		new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",n_semaphore);
		new_wait_semaphore_ptr->set_name_formatted("Wait semaphore [%d]",n_semaphore);

		m_frameSignalSemaphores.push_back(std::move(new_signal_semaphore_ptr));
		m_frameWaitSemaphores.push_back(std::move(new_wait_semaphore_ptr));
	}
}

void Context::InitSwapchain()
{
	m_renderingSurfacePtr = Anvil::RenderingSurface::create(Anvil::RenderingSurfaceCreateInfo::create(m_instancePtr.get(),m_devicePtr.get(),m_windowPtr.get()));

	m_renderingSurfacePtr->set_name("Main rendering surface");
	SetPresentMode(GetPresentMode()); // Update present mode to make sure it's supported by out surface

	auto presentMode = GetPresentMode();
	switch(presentMode)
	{
		case Anvil::PresentModeKHR::MAILBOX_KHR:
			m_numSwapchainImages = 3u;
			break;
		case Anvil::PresentModeKHR::FIFO_KHR:
			m_numSwapchainImages = 2u;
			break;
		default:
			m_numSwapchainImages = 1u;
	}
	m_keepAliveResources.resize(m_numSwapchainImages);
	m_commandBuffers.resize(m_keepAliveResources.size());
	m_cmdFences.resize(m_commandBuffers.size());
	m_fbos.resize(GetSwapchainImageCount());

	m_swapchainPtr = m_pGpuDevice->create_swapchain(
		m_renderingSurfacePtr.get(),m_windowPtr.get(),
		Anvil::Format::B8G8R8A8_UNORM,Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,presentMode,
		Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT,m_numSwapchainImages
	);

	m_swapchainPtr->set_name("Main swapchain");

	/* Cache the queue we are going to use for presentation */
	const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

	if(!m_renderingSurfacePtr->get_queue_families_with_present_support(m_physicalDevicePtr,&present_queue_fams_ptr))
		anvil_assert_fail();

	m_presentQueuePtr = m_devicePtr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),0);
}

GLFW::WindowCreationInfo &Context::GetWindowCreationInfo() {return *m_windowCreationInfo;}

void Context::InitWindow()
{
#ifdef _WIN32
	const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_SYSTEM;
#else
	const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_XCB;
#endif
	auto oldSize = (m_glfwWindow != nullptr) ? m_glfwWindow->GetSize() : Vector2i();
	auto &appName = GetAppName();
	/* Create a window */
	//m_windowPtr = Anvil::WindowFactory::create_window(platform,appName,width,height,true,std::bind(&Context::DrawFrame,this));

	// TODO: Clean this up
	try
	{
		m_glfwWindow = nullptr;
		GLFW::poll_events();
		m_glfwWindow = GLFW::Window::Create(*m_windowCreationInfo); // TODO: Release
#ifdef _WIN32
		auto hWindow = glfwGetWin32Window(const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()));
#else
		auto hWindow = glfwGetX11Window(const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()));
#endif

		// GLFW does not guarantee to actually use the size which was specified,
		// in some cases it may change, so we have to retrieve it again here
		auto actualWindowSize = m_glfwWindow->GetSize();
		m_windowCreationInfo->width = actualWindowSize.x;
		m_windowCreationInfo->height = actualWindowSize.y;

		m_windowPtr = Anvil::WindowFactory::create_window(platform,hWindow);
		auto err = glfwCreateWindowSurface(
			m_instancePtr->get_instance_vk(),
			const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()),
			nullptr,
			reinterpret_cast<VkSurfaceKHR*>(&m_surface)
		);
		// TODO: Check error
		
		m_glfwWindow->SetResizeCallback([](GLFW::Window &window,Vector2i size) {
			std::cout<<"Resizing..."<<std::endl; // TODO
		});

		//glfwDestroyWindow

		//m_window = window.GetHandle();
		//auto *allocatorCallbacks = GetAllocatorCallbacks();
		//glfwCreateWindowSurface(s_info.instance,const_cast<GLFWwindow*>(window.GetGLFWWindow()),reinterpret_cast<const VkAllocationCallbacks*>(allocatorCallbacks),reinterpret_cast<VkSurfaceKHR*>(&m_surface));

		//m_window->SetKeyCallback(std::bind(&RenderState::KeyCallback,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,std::placeholders::_5));
		//m_window->SetRefereshCallback(std::bind(&RenderState::RefreshCallback,this,std::placeholders::_1));
		//m_window->SetResizeCallback(std::bind(&RenderState::ResizeCallback,this,std::placeholders::_1,std::placeholders::_2));
		//m_context->AttachWindow(*m_window.get(),features);
	}
	catch(const std::exception &e)
	{

	}
	//catch(Vulkan::Exception &e)
	//{
	//	throw e;
	//}
	//catch(std::exception &e)
	//{
	//	throw Vulkan::Exception{e.what(),vk::Result::eErrorInitializationFailed};
	//}
	OnWindowInitialized();
	if(m_glfwWindow != nullptr && umath::is_flag_set(m_stateFlags,StateFlags::Initialized) == true)
	{
		auto newSize = m_glfwWindow->GetSize();
		if(newSize != oldSize)
			OnResolutionChanged(newSize.x,newSize.y);
	}
}

const Anvil::Instance &Context::GetAnvilInstance() const {return const_cast<Context*>(this)->GetAnvilInstance();}
Anvil::Instance &Context::GetAnvilInstance() {return *m_instancePtr;}

void Context::InitVulkan(const CreateInfo &createInfo)
{
	auto &appName = GetAppName();
	/* Create a Vulkan instance */
	m_instancePtr = Anvil::Instance::create(Anvil::InstanceCreateInfo::create(
		appName, /* app_name */
		appName, /* engine_name */
		(umath::is_flag_set(m_stateFlags,StateFlags::ValidationEnabled) == true) ? [this](
			Anvil::DebugMessageSeverityFlags severityFlags,
			const char *message
		) -> VkBool32 {
			return this->ValidationCallback(severityFlags,message);
		} :  Anvil::DebugCallbackFunction(),
		false
	)); /* in_mt_safe */

	if(umath::is_flag_set(m_stateFlags,StateFlags::ValidationEnabled) == true)
		prosper::debug::set_debug_mode_enabled(true);

	if(createInfo.device.has_value())
	{
		auto numDevices = m_instancePtr->get_n_physical_devices();
		for(auto i=decltype(numDevices){0u};i<numDevices;++i)
		{
			auto *pDevice = m_instancePtr->get_physical_device(i);
			auto *props = (pDevice != nullptr) ? pDevice->get_device_properties().core_vk1_0_properties_ptr : nullptr;
			if(props == nullptr || static_cast<Vendor>(props->vendor_id) != createInfo.device->vendorId || props->device_id != createInfo.device->deviceId)
				continue;
			m_physicalDevicePtr = m_instancePtr->get_physical_device(i);
		}
	}
	if(m_physicalDevicePtr == nullptr)
		m_physicalDevicePtr = m_instancePtr->get_physical_device(0);

	m_shaderManager = std::make_unique<ShaderManager>(*this);

    /* Create a Vulkan device */
    m_devicePtr = Anvil::SGPUDevice::create(
		Anvil::DeviceCreateInfo::create_sgpu(
			m_physicalDevicePtr,
			true, /* in_enable_shader_module_cache */
			Anvil::DeviceExtensionConfiguration(),
			std::vector<std::string>(),
			Anvil::CommandPoolCreateFlagBits::CREATE_RESET_COMMAND_BUFFER_BIT,
			false /* in_mt_safe */
		)
	);
	/*[](Anvil::BaseDevice &dev) -> Anvil::PipelineCacheUniquePtr {
		prosper::PipelineCache::LoadError loadErr {};
		auto pipelineCache = PipelineCache::Load(dev,PIPELINE_CACHE_PATH,loadErr);
		if(pipelineCache == nullptr)
			pipelineCache = PipelineCache::Create(dev);
		if(pipelineCache == nullptr)
			throw std::runtime_error("Unable to create pipeline cache!");
		return pipelineCache;
	}*/

	m_pGpuDevice = static_cast<Anvil::SGPUDevice*>(m_devicePtr.get());
	s_devToContext[m_devicePtr.get()] = this;
}

bool Context::SavePipelineCache()
{
	auto *pPipelineCache = m_devicePtr->get_pipeline_cache();
	if(pPipelineCache == nullptr)
		return false;
	return PipelineCache::Save(*pPipelineCache,PIPELINE_CACHE_PATH);
}

VkBool32 Context::ValidationCallback(
	Anvil::DebugMessageSeverityFlags severityFlags,
	const char *message
)
{
    return false;
}

void Context::Run()
{
	auto t = std::chrono::high_resolution_clock::now();
	while(true) // TODO
	{
		auto tNow = std::chrono::high_resolution_clock::now();
		auto tDelta = tNow -t;
		auto bResChanged = false;
		if(std::chrono::duration_cast<std::chrono::seconds>(tDelta).count() > 5 && bResChanged == false)
		{
			bResChanged = true;
			//ChangeResolution(1024,768);
		}
		DrawFrame();
		GLFW::poll_events();
	}
}
#pragma optimize("",on)
