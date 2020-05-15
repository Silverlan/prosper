/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_context.hpp"
#include "vk_fence.hpp"
#include "vk_command_buffer.hpp"
#include "vk_render_pass.hpp"
#include "buffers/vk_buffer.hpp"
#include "vk_descriptor_set_group.hpp"
#include "prosper_pipeline_cache.hpp"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "image/vk_image.hpp"
#include "image/vk_image_view.hpp"
#include "image/vk_sampler.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <util_image_buffer.hpp>
#include <misc/buffer_create_info.h>
#include <misc/fence_create_info.h>
#include <misc/semaphore_create_info.h>
#include <misc/framebuffer_create_info.h>
#include <misc/swapchain_create_info.h>
#include <misc/rendering_surface_create_info.h>
#include <misc/graphics_pipeline_create_info.h>
#include <misc/render_pass_create_info.h>
#include <misc/image_create_info.h>
#include <misc/window_factory.h>
#include <misc/window.h>
#include <wrappers/fence.h>
#include <wrappers/device.h>
#include <wrappers/queue.h>
#include <wrappers/instance.h>
#include <wrappers/swapchain.h>
#include <wrappers/render_pass.h>
#include <wrappers/command_buffer.h>
#include <wrappers/command_pool.h>
#include <wrappers/framebuffer.h>
#include <wrappers/semaphore.h>
#include <wrappers/descriptor_set_group.h>
#include <wrappers/graphics_pipeline_manager.h>
#include <wrappers/compute_pipeline_manager.h>
#include <iglfw/glfw_window.h>

#include <string>
#include <cmath>
#include <iglfw/glfw_window.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3native.h>

#ifdef __linux__
#define ENABLE_GLFW_ANVIL_COMPATIBILITY
#endif
#ifdef ENABLE_GLFW_ANVIL_COMPATIBILITY
#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#endif

using namespace prosper;

decltype(VlkContext::s_devToContext) VlkContext::s_devToContext = {};

VlkContext::VlkContext(const std::string &appName,bool bEnableValidation)
	: IPrContext{appName,bEnableValidation}
{}

prosper::Result VlkContext::WaitForFence(const IFence &fence,uint64_t timeout) const
{
	auto vkFence = static_cast<const VlkFence&>(fence).GetAnvilFence().get_fence();
	return static_cast<prosper::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),1,&vkFence,true,timeout));
}

prosper::Result VlkContext::WaitForFences(const std::vector<IFence*> &fences,bool waitAll,uint64_t timeout) const
{
	std::vector<VkFence> vkFences {};
	vkFences.reserve(fences.size());
	for(auto &fence : fences)
		vkFences.push_back(static_cast<VlkFence&>(*fence).GetAnvilFence().get_fence());
	return static_cast<prosper::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),vkFences.size(),vkFences.data(),waitAll,timeout));
}

void VlkContext::DrawFrame(const std::function<void(const std::shared_ptr<prosper::IPrimaryCommandBuffer>&,uint32_t)> &drawFrame)
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
	if(errCode == Anvil::SwapchainOperationErrorCode::OUT_OF_DATE)
	{
		InitSwapchain();
		return;
	}

	auto success = (errCode == Anvil::SwapchainOperationErrorCode::SUCCESS);
	std::string errMsg;
	if(success)
	{
		auto waitResult = static_cast<prosper::Result>(vkWaitForFences(m_devicePtr->get_device_vk(),1,m_cmdFences[m_n_swapchain_image]->get_fence_ptr(),true,std::numeric_limits<uint64_t>::max()));
		if(waitResult == prosper::Result::Success)
		{
			if(m_cmdFences[m_n_swapchain_image]->reset() == false)
				errMsg = "Unable to reset swapchain fence!";
		}
		else
			errMsg = "An error has occurred when waiting for swapchain fence: " +prosper::util::to_string(waitResult);
	}
	else
		errMsg = "Unable to acquire next swapchain image: " +std::to_string(umath::to_integral(errCode));
	if(errMsg.empty() == false)
		throw std::runtime_error(errMsg);

	ClearKeepAliveResources();

	//auto &keepAliveResources = m_keepAliveResources.at(m_n_swapchain_image);
	//auto numKeepAliveResources = keepAliveResources.size(); // We can clear the resources from the previous render pass of this swapchain after we've waited for the semaphore (i.e. after the frame rendering is complete)

	auto &cmd_buffer_ptr = m_commandBuffers.at(m_n_swapchain_image);
	/* Start recording commands */
	static_cast<Anvil::PrimaryCommandBuffer&>(static_cast<prosper::VlkPrimaryCommandBuffer&>(*cmd_buffer_ptr).GetAnvilCommandBuffer()).start_recording(false,true);
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
	static_cast<Anvil::PrimaryCommandBuffer&>(static_cast<prosper::VlkPrimaryCommandBuffer&>(*cmd_buffer_ptr).GetAnvilCommandBuffer()).stop_recording();


	/* Submit work chunk and present */
	auto *signalSemaphore = curr_frame_signal_semaphore_ptr;
	auto *waitSemaphore = curr_frame_wait_semaphore_ptr;
	m_devicePtr->get_universal_queue(0)->submit(Anvil::SubmitInfo::create(
		&static_cast<prosper::VlkPrimaryCommandBuffer&>(*m_commandBuffers[m_n_swapchain_image]).GetAnvilCommandBuffer(),
		1, /* n_semaphores_to_signal */
		&signalSemaphore,
		1, /* n_semaphores_to_wait_on */
		&waitSemaphore,
		&wait_stage_mask,
		false, /* should_block  */
		m_cmdFences.at(m_n_swapchain_image).get()
	)); /* opt_fence_ptr */

		//ClearKeepAliveResources(numKeepAliveResources);

	auto bPresentSuccess = present_queue_ptr->present(
		m_swapchainPtr.get(),
		m_n_swapchain_image,
		1, /* n_wait_semaphores */
		&signalSemaphore,
		&errCode
	);
	if(errCode != Anvil::SwapchainOperationErrorCode::SUCCESS || bPresentSuccess == false)
	{
		if(errCode == Anvil::SwapchainOperationErrorCode::OUT_OF_DATE)
		{
			InitSwapchain();
			return;
		}
		else
			; // Terminal error?
	}
	//m_glfwWindow->SwapBuffers();
}

bool VlkContext::Submit(ICommandBuffer &cmdBuf,bool shouldBlock,IFence *optFence)
{
	auto &dev = GetDevice();
	return dev.get_universal_queue(0)->submit(Anvil::SubmitInfo::create(
		&dynamic_cast<VlkCommandBuffer&>(cmdBuf).GetAnvilCommandBuffer(),0u,nullptr,
		0u,nullptr,nullptr,shouldBlock,optFence ? &dynamic_cast<VlkFence*>(optFence)->GetAnvilFence() : nullptr
	));
}

bool VlkContext::GetSurfaceCapabilities(Anvil::SurfaceCapabilities &caps) const
{
	return const_cast<VlkContext*>(this)->GetDevice().get_physical_device_surface_capabilities(m_renderingSurfacePtr.get(),&caps);
}

void VlkContext::ReleaseSwapchain()
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

void VlkContext::ReloadWindow()
{
	WaitIdle();
	ReleaseSwapchain();
	InitWindow();
	ReloadSwapchain();
}

VkBool32 VlkContext::ValidationCallback(
	Anvil::DebugMessageSeverityFlags severityFlags,
	const char *message
)
{
	if(m_callbacks.validationCallback)
		m_callbacks.validationCallback(static_cast<prosper::DebugMessageSeverityFlags>(severityFlags.get_vk()),message);
	return false;
}

const Anvil::Instance &VlkContext::GetAnvilInstance() const {return const_cast<VlkContext*>(this)->GetAnvilInstance();}
Anvil::Instance &VlkContext::GetAnvilInstance() {return *m_instancePtr;}

std::shared_ptr<VlkContext> VlkContext::Create(const std::string &appName,bool bEnableValidation)
{
	return std::shared_ptr<VlkContext>{new VlkContext{appName,bEnableValidation}};
}

IPrContext &VlkContext::GetContext(Anvil::BaseDevice &dev)
{
	auto it = s_devToContext.find(&dev);
	if(it == s_devToContext.end())
		throw std::runtime_error("Invalid context for device!");
	return *it->second;
}

void VlkContext::Release()
{
	if(m_devicePtr == nullptr)
		return;
	ReleaseSwapchain();

	IPrContext::Release();

	auto &dev = GetDevice();
	auto it = s_devToContext.find(&dev);
	if(it != s_devToContext.end())
		s_devToContext.erase(it);

	m_devicePtr.reset(); // All Vulkan resources related to the device have to be cleared at this point!
	vkDestroySurfaceKHR(m_instancePtr->get_instance_vk(),m_surface,nullptr);
	m_instancePtr.reset();
	m_windowPtr.reset();
}

Anvil::SGPUDevice &VlkContext::GetDevice() {return *m_pGpuDevice;}
const std::shared_ptr<Anvil::RenderPass> &VlkContext::GetMainRenderPass() const {return m_renderPass;}
Anvil::SubPassID VlkContext::GetMainSubPassID() const {return m_mainSubPass;}
std::shared_ptr<Anvil::Swapchain> VlkContext::GetSwapchain() {return m_swapchainPtr;}

void VlkContext::DoWaitIdle()
{
	auto &dev = GetDevice();
	dev.wait_idle();
}

void VlkContext::DoFlushSetupCommandBuffer()
{
	auto bSuccess = static_cast<Anvil::PrimaryCommandBuffer&>(static_cast<prosper::VlkPrimaryCommandBuffer&>(*m_setupCmdBuffer).GetAnvilCommandBuffer()).stop_recording();
	auto &dev = GetDevice();
	dev.get_universal_queue(0)->submit(Anvil::SubmitInfo::create(
		&static_cast<prosper::VlkPrimaryCommandBuffer&>(*m_setupCmdBuffer).GetAnvilCommandBuffer(),0u,nullptr,
		0u,nullptr,nullptr,true
	));
}


bool VlkContext::IsImageFormatSupported(prosper::Format format,prosper::ImageUsageFlags usageFlags,prosper::ImageType type,prosper::ImageTiling tiling) const
{
	return m_devicePtr->get_physical_device_image_format_properties(
		Anvil::ImageFormatPropertiesQuery{
			static_cast<Anvil::Format>(format),static_cast<Anvil::ImageType>(type),static_cast<Anvil::ImageTiling>(tiling),
			static_cast<Anvil::ImageUsageFlagBits>(usageFlags),{}
		}
	);
}

const std::string PIPELINE_CACHE_PATH = "cache/shader.cache";
bool VlkContext::SavePipelineCache()
{
	auto *pPipelineCache = m_devicePtr->get_pipeline_cache();
	if(pPipelineCache == nullptr)
		return false;
	return PipelineCache::Save(*pPipelineCache,PIPELINE_CACHE_PATH);
}

std::shared_ptr<prosper::IPrimaryCommandBuffer> VlkContext::AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)
{
	if(GetUniversalQueueFamilyIndex(queueFamilyType,universalQueueFamilyIndex) == false)
	{
		if(queueFamilyType != prosper::QueueFamilyType::Universal)
			return AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType::Universal,universalQueueFamilyIndex);
		return nullptr;
	}
	return prosper::VlkPrimaryCommandBuffer::Create(*this,GetDevice().get_command_pool_for_queue_family_index(universalQueueFamilyIndex)->alloc_primary_level_command_buffer(),queueFamilyType);
}
std::shared_ptr<prosper::ISecondaryCommandBuffer> VlkContext::AllocateSecondaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)
{
	if(GetUniversalQueueFamilyIndex(queueFamilyType,universalQueueFamilyIndex) == false)
	{
		if(queueFamilyType != prosper::QueueFamilyType::Universal)
			return AllocateSecondaryLevelCommandBuffer(prosper::QueueFamilyType::Universal,universalQueueFamilyIndex);
		return nullptr;
	}
	return std::dynamic_pointer_cast<ISecondaryCommandBuffer>(prosper::VlkSecondaryCommandBuffer::Create(
		*this,GetDevice().get_command_pool_for_queue_family_index(universalQueueFamilyIndex)->alloc_secondary_level_command_buffer(),
		queueFamilyType
	));
}

void VlkContext::ReloadSwapchain()
{
	// Reload swapchain related objects
	WaitIdle();
	ReleaseSwapchain();
	InitSwapchain();
	InitFrameBuffers();
	InitCommandBuffers();
	InitSemaphores();
	InitMainRenderPass();
	InitTemporaryBuffer();
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
				auto *baseInfo = shader.GetPipelineCreateInfo(i);
				if(baseInfo == nullptr)
					continue;
				auto &info = static_cast<const prosper::GraphicsPipelineCreateInfo&>(*baseInfo);
				if(info.GetScissorBoxesCount() == 0u && info.GetViewportCount() == 0u)
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

void VlkContext::InitCommandBuffers()
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
		auto cmd_buffer_ptr = prosper::VlkPrimaryCommandBuffer::Create(*this,m_devicePtr->get_command_pool_for_queue_family_index(universal_queue_family_indices[0])->alloc_primary_level_command_buffer(),prosper::QueueFamilyType::Universal);
		cmd_buffer_ptr->SetDebugName("swapchain_cmd" +std::to_string(n_current_swapchain_image));
		//m_devicePtr->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)->alloc_primary_level_command_buffer();

		m_commandBuffers[n_current_swapchain_image] = cmd_buffer_ptr;
		m_cmdFences[n_current_swapchain_image] = Anvil::Fence::create(Anvil::FenceCreateInfo::create(m_devicePtr.get(),true));
	}
}

void VlkContext::InitFrameBuffers()
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

void VlkContext::InitSemaphores()
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

void VlkContext::InitSwapchain()
{
	m_renderingSurfacePtr = Anvil::RenderingSurface::create(Anvil::RenderingSurfaceCreateInfo::create(m_instancePtr.get(),m_devicePtr.get(),m_windowPtr.get()));
	if(m_renderingSurfacePtr->get_width() == 0 || m_renderingSurfacePtr->get_height() == 0)
		return; // Minimized?

	m_n_swapchain_image = 0;
	m_renderingSurfacePtr->set_name("Main rendering surface");
	SetPresentMode(GetPresentMode()); // Update present mode to make sure it's supported by out surface

	auto presentMode = GetPresentMode();
	switch(presentMode)
	{
	case prosper::PresentModeKHR::Mailbox:
		m_numSwapchainImages = 3u;
		break;
	case prosper::PresentModeKHR::Fifo:
		m_numSwapchainImages = 2u;
		break;
	default:
		m_numSwapchainImages = 1u;
	}

	m_swapchainImages.clear();
	m_cmdFences.clear();
	m_commandBuffers.clear();
	m_fbos.clear();

	m_commandBuffers.resize(m_numSwapchainImages);
	m_cmdFences.resize(m_numSwapchainImages);
	m_fbos.resize(m_numSwapchainImages);

	auto createInfo = Anvil::SwapchainCreateInfo::create(
		m_pGpuDevice,
		m_renderingSurfacePtr.get(),m_windowPtr.get(),
		Anvil::Format::B8G8R8A8_UNORM,Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,static_cast<Anvil::PresentModeKHR>(presentMode),
		Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT,m_numSwapchainImages
	);
	createInfo->set_mt_safety(Anvil::MTSafety::ENABLED);

	auto recreateSwapchain = (m_swapchainPtr != nullptr);
	if(recreateSwapchain)
		createInfo->set_old_swapchain(m_swapchainPtr.get());
	m_swapchainPtr = Anvil::Swapchain::create(std::move(createInfo));

	/*
	m_swapchainPtr = m_pGpuDevice->create_swapchain(
	m_renderingSurfacePtr.get(),m_windowPtr.get(),
	Anvil::Format::B8G8R8A8_UNORM,Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,static_cast<Anvil::PresentModeKHR>(presentMode),
	Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT,m_numSwapchainImages
	);*/

	auto nSwapchainImages = m_swapchainPtr->get_n_images();
	m_swapchainImages.resize(nSwapchainImages);
	for(auto i=decltype(nSwapchainImages){0u};i<nSwapchainImages;++i)
	{
		auto &img = *m_swapchainPtr->get_image(i);
		auto *anvCreateInfo = img.get_create_info_ptr();
		prosper::util::ImageCreateInfo createInfo {};
		createInfo.format = static_cast<prosper::Format>(anvCreateInfo->get_format());
		createInfo.width = anvCreateInfo->get_base_mip_width();
		createInfo.height = anvCreateInfo->get_base_mip_height();
		createInfo.layers = anvCreateInfo->get_n_layers();
		createInfo.tiling = static_cast<prosper::ImageTiling>(anvCreateInfo->get_tiling());
		createInfo.samples = static_cast<prosper::SampleCountFlags>(anvCreateInfo->get_sample_count());
		createInfo.usage = static_cast<prosper::ImageUsageFlags>(anvCreateInfo->get_usage_flags().get_vk());
		createInfo.type = static_cast<prosper::ImageType>(anvCreateInfo->get_type());
		createInfo.postCreateLayout = static_cast<prosper::ImageLayout>(anvCreateInfo->get_post_create_image_layout());
		createInfo.queueFamilyMask = static_cast<prosper::QueueFamilyFlags>(anvCreateInfo->get_queue_families().get_vk());
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		m_swapchainImages.at(i) = VlkImage::Create(*this,std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>>{&img,[](Anvil::Image *img) {
			// Don't delete, image will be destroyed by Anvil
		}},createInfo,true);
	}

	m_swapchainPtr->set_name("Main swapchain");

	/* Cache the queue we are going to use for presentation */
	const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

	if(!m_renderingSurfacePtr->get_queue_families_with_present_support(m_physicalDevicePtr,&present_queue_fams_ptr))
		anvil_assert_fail();

	m_presentQueuePtr = m_devicePtr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),0);

	if(recreateSwapchain)
	{
		InitFrameBuffers();
		InitCommandBuffers();
	}
	m_keepAliveResources.clear();
	m_keepAliveResources.resize(m_numSwapchainImages);
}

void VlkContext::InitWindow()
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
#ifdef ENABLE_GLFW_ANVIL_COMPATIBILITY
																	// This is a workaround since GLFW does not expose any functions
																	// for retrieving XCB connection.
		auto hWindow = glfwGetX11Window(const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()));
		auto *pConnection =  XGetXCBConnection(glfwGetX11Display());
#else
#error "Unable to retrieve xcb connection: GLFW does not expose required functions."
		auto hWindow = glfwGetX11Window(const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()));
#endif
#endif
		const char *errDesc;
		auto err = glfwGetError(&errDesc);
		if(err != GLFW_NO_ERROR)
		{
			std::cout<<"Error retrieving GLFW window handle: "<<errDesc<<std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(5));
			exit(EXIT_FAILURE);
		}

		// GLFW does not guarantee to actually use the size which was specified,
		// in some cases it may change, so we have to retrieve it again here
		auto actualWindowSize = m_glfwWindow->GetSize();
		m_windowCreationInfo->width = actualWindowSize.x;
		m_windowCreationInfo->height = actualWindowSize.y;

		m_windowPtr = Anvil::WindowFactory::create_window(platform,hWindow
#ifdef ENABLE_GLFW_ANVIL_COMPATIBILITY
			,pConnection
#endif
		);

		err = glfwCreateWindowSurface(
			m_instancePtr->get_instance_vk(),
			const_cast<GLFWwindow*>(m_glfwWindow->GetGLFWWindow()),
			nullptr,
			reinterpret_cast<VkSurfaceKHR*>(&m_surface)
		);
		if(err != GLFW_NO_ERROR)
		{
			glfwGetError(&errDesc);
			std::cout<<"Error creating GLFW window surface: "<<errDesc<<std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(5));
			exit(EXIT_FAILURE);
		}

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

void VlkContext::InitVulkan(const CreateInfo &createInfo)
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
	Anvil::DeviceExtensionConfiguration devExtConfig {};

	// Note: These are required for VR on Nvidia GPUs!
	devExtConfig.extension_status["VK_NV_dedicated_allocation"] = Anvil::ExtensionAvailability::ENABLE_IF_AVAILABLE;
	devExtConfig.extension_status["VK_NV_external_memory"] = Anvil::ExtensionAvailability::ENABLE_IF_AVAILABLE;
	devExtConfig.extension_status["VK_NV_external_memory_win32"] = Anvil::ExtensionAvailability::ENABLE_IF_AVAILABLE;
	devExtConfig.extension_status["VK_NV_win32_keyed_mutex"] = Anvil::ExtensionAvailability::ENABLE_IF_AVAILABLE;

	auto devCreateInfo = Anvil::DeviceCreateInfo::create_sgpu(
		m_physicalDevicePtr,
		true, /* in_enable_shader_module_cache */
		devExtConfig,
		std::vector<std::string>(),
		Anvil::CommandPoolCreateFlagBits::CREATE_RESET_COMMAND_BUFFER_BIT,
		false /* in_mt_safe */
	);
	// devCreateInfo->set_pipeline_cache_ptr() // TODO
	m_devicePtr = Anvil::SGPUDevice::create(
		std::move(devCreateInfo)
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

Vendor VlkContext::GetPhysicalDeviceVendor() const
{
	return static_cast<Vendor>(const_cast<VlkContext*>(this)->GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->vendor_id);
}

MemoryRequirements VlkContext::GetMemoryRequirements(IImage &img)
{
	static_assert(sizeof(MemoryRequirements) == sizeof(VkMemoryRequirements));
	MemoryRequirements req {};
	vkGetImageMemoryRequirements(GetDevice().get_device_vk(),static_cast<VlkImage&>(img).GetAnvilImage().get_image(),reinterpret_cast<VkMemoryRequirements*>(&req));
	return req;
}

DeviceSize VlkContext::GetBufferAlignment(BufferUsageFlags usageFlags)
{
	return prosper::util::calculate_buffer_alignment(GetDevice(),usageFlags);
}

void VlkContext::DoKeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource)
{
	if(m_cmdFences[m_n_swapchain_image]->is_set())
		return;
	m_keepAliveResources.at(m_n_swapchain_image).push_back(resource);
}

bool VlkContext::IsPresentationModeSupported(prosper::PresentModeKHR presentMode) const
{
	if(m_renderingSurfacePtr == nullptr)
		return true;
	auto res = false;
	m_renderingSurfacePtr->supports_presentation_mode(m_physicalDevicePtr,static_cast<Anvil::PresentModeKHR>(presentMode),&res);
	return res;
}

Anvil::PipelineLayout *VlkContext::GetPipelineLayout(bool graphicsShader,Anvil::PipelineID pipelineId)
{
	auto &dev = GetDevice();
	if(graphicsShader)
		return dev.get_graphics_pipeline_manager()->get_pipeline_layout(pipelineId);
	return dev.get_compute_pipeline_manager()->get_pipeline_layout(pipelineId);
}

void VlkContext::SubmitCommandBuffer(prosper::ICommandBuffer &cmd,prosper::QueueFamilyType queueFamilyType,bool shouldBlock,prosper::IFence *fence)
{
	switch(queueFamilyType)
	{
	case prosper::QueueFamilyType::Universal:
		m_devicePtr->get_universal_queue(0u)->submit(Anvil::SubmitInfo::create(
			&dynamic_cast<VlkCommandBuffer&>(cmd).GetAnvilCommandBuffer(),0u,nullptr,0u,nullptr,nullptr,shouldBlock,fence ? &static_cast<VlkFence*>(fence)->GetAnvilFence() : nullptr
		));
		break;
	case prosper::QueueFamilyType::Compute:
		m_devicePtr->get_compute_queue(0u)->submit(Anvil::SubmitInfo::create(
			&dynamic_cast<VlkCommandBuffer&>(cmd).GetAnvilCommandBuffer(),0u,nullptr,0u,nullptr,nullptr,shouldBlock,fence ? &static_cast<VlkFence*>(fence)->GetAnvilFence() : nullptr
		));
		break;
	default:
		throw std::invalid_argument("No device queue exists for queue family " +std::to_string(umath::to_integral(queueFamilyType)) +"!");
	}
}

bool VlkContext::GetUniversalQueueFamilyIndex(prosper::QueueFamilyType queueFamilyType,uint32_t &queueFamilyIndex) const
{
	auto n_universal_queue_family_indices = 0u;
	const uint32_t *universal_queue_family_indices = nullptr;
	if(m_devicePtr->get_queue_family_indices_for_queue_family_type(
		static_cast<Anvil::QueueFamilyType>(queueFamilyType),
		&n_universal_queue_family_indices,
		&universal_queue_family_indices
	) == false || n_universal_queue_family_indices == 0u)
		return false;
	queueFamilyIndex = universal_queue_family_indices[0];
	return true;
}

void VlkContext::InitAPI(const CreateInfo &createInfo)
{
	InitVulkan(createInfo);
	InitWindow();
	ReloadSwapchain();
}

void VlkContext::InitMainRenderPass()
{
	Anvil::RenderPassAttachmentID render_pass_color_attachment_id;
	prosper::Rect2D scissors[4];

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

std::shared_ptr<prosper::IImage> create_image(prosper::IPrContext &context,std::vector<std::shared_ptr<uimg::ImageBuffer>> &imgBuffers,bool cubemap)
{
	if(imgBuffers.empty() || imgBuffers.size() != (cubemap ? 6 : 1))
		return nullptr;
	auto &img0 = imgBuffers.at(0);
	auto format = img0->GetFormat();
	auto w = img0->GetWidth();
	auto h = img0->GetHeight();
	for(auto &img : imgBuffers)
	{
		if(img->GetFormat() != format || img->GetWidth() != w || img->GetHeight() != h)
			return nullptr;
	}

	std::optional<uimg::ImageBuffer::Format> conversionFormatRequired = {};
	prosper::Format prosperFormat;
	switch(format)
	{
	case uimg::ImageBuffer::Format::RGB8:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA8;
		break;
	case uimg::ImageBuffer::Format::RGB16:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA16;
		break;
	case uimg::ImageBuffer::Format::RGB32:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA32;
		break;
	case uimg::ImageBuffer::Format::RGBA8:
		prosperFormat = prosper::Format::R8G8B8A8_UNorm;
		break;
	case uimg::ImageBuffer::Format::RGBA16:
		prosperFormat = prosper::Format::R16G16B16A16_UNorm;
		break;
	case uimg::ImageBuffer::Format::RGBA32:
		prosperFormat = prosper::Format::R32G32B32A32_SFloat;
		break;
	}
	static_assert(umath::to_integral(uimg::ImageBuffer::Format::Count) == 7);
	if(conversionFormatRequired.has_value())
	{
		std::vector<std::shared_ptr<uimg::ImageBuffer>> converted {};
		converted.reserve(imgBuffers.size());
		for(auto &img : imgBuffers)
		{
			auto copy = img->Copy(*conversionFormatRequired);
			converted.push_back(copy);
		}
		return create_image(context,converted,cubemap);
	}
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosperFormat;
	imgCreateInfo.width = w;
	imgCreateInfo.height = h;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
	imgCreateInfo.layers = imgBuffers.size();
	if(cubemap)
		imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;

	auto byteSize = prosper::util::get_byte_size(imgCreateInfo.format);
	std::vector<Anvil::MipmapRawData> layers {};
	layers.reserve(imgBuffers.size());
	for(auto iLayer=decltype(imgBuffers.size()){0u};iLayer<imgBuffers.size();++iLayer)
	{
		auto &imgBuf = imgBuffers.at(iLayer);
		if(cubemap)
		{
			layers.push_back(Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(
				Anvil::ImageAspectFlagBits::COLOR_BIT,iLayer,0,
				static_cast<uint8_t*>(imgBuf->GetData()),imgCreateInfo.width *imgCreateInfo.height *byteSize,imgCreateInfo.width *byteSize
			));
		}
		else
		{
			layers.push_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(
				Anvil::ImageAspectFlagBits::COLOR_BIT,0u,
				static_cast<uint8_t*>(imgBuf->GetData()),imgCreateInfo.width *imgCreateInfo.height *byteSize,imgCreateInfo.width *byteSize
			));
		}
	}
	return static_cast<VlkContext&>(context).CreateImage(imgCreateInfo,layers);
}
extern std::shared_ptr<prosper::IImage> create_image(prosper::IPrContext &context,const prosper::util::ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> *data);
std::shared_ptr<prosper::IImage> prosper::VlkContext::CreateImage(const util::ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> &data)
{
	return ::create_image(*this,createInfo,&data);
}
std::shared_ptr<prosper::IRenderPass> prosper::VlkContext::CreateRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo,std::unique_ptr<Anvil::RenderPassCreateInfo> anvRenderPassInfo)
{
	return prosper::VlkRenderPass::Create(*this,renderPassInfo,Anvil::RenderPass::create(std::move(anvRenderPassInfo),GetSwapchain().get()));
}
static void init_default_dsg_bindings(Anvil::BaseDevice &dev,Anvil::DescriptorSetGroup &dsg)
{
	// Initialize image sampler bindings with dummy texture
	auto &context = prosper::VlkContext::GetContext(dev);
	auto &dummyTex = context.GetDummyTexture();
	auto &dummyBuf = context.GetDummyBuffer();
	auto numSets = dsg.get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
	{
		auto *descSet = dsg.get_descriptor_set(i);
		auto *descSetLayout = dsg.get_descriptor_set_layout(i);
		auto &info = *descSetLayout->get_create_info();
		auto numBindings = info.get_n_bindings();
		auto bindingIndex = 0u;
		for(auto j=decltype(numBindings){0};j<numBindings;++j)
		{
			Anvil::DescriptorType descType;
			uint32_t arraySize;
			info.get_binding_properties_by_index_number(j,nullptr,&descType,&arraySize,nullptr,nullptr);
			switch(descType)
			{
			case Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER:
			{
				std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
					Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					&static_cast<prosper::VlkImageView&>(*dummyTex->GetImageView()).GetAnvilImageView(),&static_cast<prosper::VlkSampler&>(*dummyTex->GetSampler()).GetAnvilSampler()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::UNIFORM_BUFFER:
			{
				std::vector<Anvil::DescriptorSet::UniformBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::UniformBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC:
			{
				std::vector<Anvil::DescriptorSet::DynamicUniformBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::DynamicUniformBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::STORAGE_BUFFER:
			{
				std::vector<Anvil::DescriptorSet::StorageBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::StorageBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC:
			{
				std::vector<Anvil::DescriptorSet::DynamicStorageBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::DynamicStorageBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			}
			//bindingIndex += arraySize;
			++bindingIndex;
		}
	}
}
std::shared_ptr<prosper::IDescriptorSetGroup> prosper::VlkContext::CreateDescriptorSetGroup(const DescriptorSetCreateInfo &descSetCreateInfo,std::unique_ptr<Anvil::DescriptorSetCreateInfo> descSetInfo)
{
	std::vector<std::unique_ptr<Anvil::DescriptorSetCreateInfo>> descSetInfos = {};
	descSetInfos.push_back(std::move(descSetInfo));
	auto dsg = Anvil::DescriptorSetGroup::create(&static_cast<VlkContext&>(*this).GetDevice(),descSetInfos,Anvil::DescriptorPoolCreateFlagBits::FREE_DESCRIPTOR_SET_BIT);
	init_default_dsg_bindings(static_cast<VlkContext&>(*this).GetDevice(),*dsg);
	return prosper::VlkDescriptorSetGroup::Create(*this,descSetCreateInfo,std::move(dsg));
}
