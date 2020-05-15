/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sharedutils/util.h>
#include "stdafx_prosper.h"
#include "prosper_context.hpp"
#include "image/prosper_texture.hpp"
#include "prosper_util.hpp"
#include "shader/prosper_shader_blur.hpp"
#include "shader/prosper_shader_copy_image.hpp"
#include "prosper_util_square_shape.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "buffers/prosper_buffer.hpp"
#include "buffers/prosper_dynamic_resizable_buffer.hpp"
#include "buffers/vk_buffer.hpp"
#include "buffers/vk_dynamic_resizable_buffer.hpp"
#include "vk_command_buffer.hpp"
#include "image/prosper_sampler.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_fence.hpp"
#include "prosper_pipeline_cache.hpp"
#include <wrappers/command_buffer.h>
#include <iglfw/glfw_window.h>
#include <sharedutils/util_clock.hpp>
#include <thread>

/* Uncomment the #define below to enable off-screen rendering */
// #define ENABLE_OFFSCREEN_RENDERING

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

#pragma optimize("",off)
static std::shared_ptr<prosper::IBuffer> s_vertexBuffer = nullptr;
static std::shared_ptr<prosper::IBuffer> s_uvBuffer = nullptr;
static std::shared_ptr<prosper::IBuffer> s_vertexUvBuffer = nullptr;
IPrContext::IPrContext(const std::string &appName,bool bEnableValidation)
	: m_lastSemaporeUsed(0),m_appName(appName),
	m_windowCreationInfo(std::make_unique<GLFW::WindowCreationInfo>())
{
	umath::set_flag(m_stateFlags,StateFlags::ValidationEnabled,bEnableValidation);
	m_windowCreationInfo->title = appName;
}

IPrContext::~IPrContext()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::Closed) == false)
		throw std::logic_error{"Prosper context has to be closed before being destroyed!"};
}

void IPrContext::SetValidationEnabled(bool b) {umath::set_flag(m_stateFlags,StateFlags::ValidationEnabled,b);}
bool IPrContext::IsValidationEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::ValidationEnabled);}

const std::string &IPrContext::GetAppName() const {return m_appName;}
uint32_t IPrContext::GetSwapchainImageCount() const {return m_numSwapchainImages;}

prosper::IImage *IPrContext::GetSwapchainImage(uint32_t idx)
{
	return (idx < m_swapchainImages.size()) ? m_swapchainImages.at(idx).get() : nullptr;
}

GLFW::Window &IPrContext::GetWindow() {return *m_glfwWindow;}
std::array<uint32_t,2> IPrContext::GetWindowSize() const {return {m_windowCreationInfo->width,m_windowCreationInfo->height};}
uint32_t IPrContext::GetWindowWidth() const {return m_windowCreationInfo->width;}
uint32_t IPrContext::GetWindowHeight() const {return m_windowCreationInfo->height;}

void IPrContext::Close()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::Closed))
		return;
	WaitIdle();
	OnClose();
}
void IPrContext::OnClose()
{
	if(m_callbacks.onClose)
		m_callbacks.onClose();
	Release();
}
void IPrContext::Release()
{
	s_vertexBuffer = nullptr;
	s_uvBuffer = nullptr;
	s_vertexUvBuffer = nullptr;

	m_shaderManager = nullptr;
	m_dummyTexture = nullptr;
	m_dummyCubemapTexture = nullptr;
	m_dummyBuffer = nullptr;
	m_swapchainImages.clear();

	m_tmpBuffer = nullptr;
	m_deviceImgBuffers.clear();

	m_setupCmdBuffer = nullptr;
	m_keepAliveResources.clear();
	while(m_scheduledBufferUpdates.empty() == false)
		m_scheduledBufferUpdates.pop();

	m_glfwWindow = nullptr;
	m_stateFlags |= StateFlags::Closed;
}

uint64_t IPrContext::GetLastFrameId() const {return m_frameId;}

void IPrContext::EndFrame() {++m_frameId;}

void IPrContext::DrawFrame()
{
	DrawFrame([this](const std::shared_ptr<prosper::IPrimaryCommandBuffer>&,uint32_t m_n_swapchain_image) {
		Draw(m_n_swapchain_image);
	});
	EndFrame();
}

void IPrContext::AllocateDeviceImageBuffer(prosper::IImage &img,const void *data)
{
	auto req = GetMemoryRequirements(img);
	auto buf = AllocateDeviceImageBuffer(req.size,req.alignment,data);
	img.SetMemoryBuffer(*buf);
}
std::shared_ptr<IBuffer> IPrContext::AllocateDeviceImageBuffer(vk::DeviceSize size,uint32_t alignment,const void *data)
{
	static uint64_t totalAllocated = 0;
	totalAllocated += size;
	auto fAllocateImgBuf = [this,size,alignment,data](prosper::IDynamicResizableBuffer &deviceImgBuf) -> std::shared_ptr<IBuffer> {
		return deviceImgBuf.AllocateBuffer(size,alignment,data);
	};
	for(auto &deviceImgBuf : m_deviceImgBuffers)
	{
		auto buf = fAllocateImgBuf(*deviceImgBuf);
		if(buf)
			return buf;
	}
	
	// Allocate new image buffer with device memory
	uint64_t bufferSize = 512 *1'024 *1'024; // 512 MiB
	auto maxTotalPercent = 0.5f;

	if(m_deviceImgBuffers.empty() == false)
	{
		auto &imgBuf = m_deviceImgBuffers.back();
		std::cout<<"Fragmentation of previous image buffer: "<<imgBuf->GetFragmentationPercent()<<std::endl;
		auto numAllocated = 0;
		for(auto &subBuf : imgBuf->GetAllocatedSubBuffers())
		{
			numAllocated += subBuf->GetSize();
		}
		std::cout<<"Allocated: "<<numAllocated<<" / "<<imgBuf->GetSize()<<std::endl;
	}

	std::cout<<"Image of buffer size "<<::util::get_pretty_bytes(size)<<" (alignment "<<alignment<<") was requested, but does not fit into previous buffer(s) ("<<m_deviceImgBuffers.size()<<")! Allocating new buffer of size "<<::util::get_pretty_bytes(bufferSize)<<"..."<<std::endl;
	std::cout<<"Total allocated: "<<::util::get_pretty_bytes(totalAllocated)<<std::endl;
	util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = MemoryFeatureFlags::GPUBulk;
	createInfo.size = bufferSize;
	createInfo.usageFlags = BufferUsageFlags::None;
	auto deviceImgBuffer = prosper::util::create_dynamic_resizable_buffer(*this,createInfo,bufferSize,maxTotalPercent);
	assert(m_deviceImgBuffer);
	std::cout<<"Device image buffer size: "<<bufferSize<<std::endl;
	if(deviceImgBuffer == nullptr)
		return nullptr;
	m_deviceImgBuffers.push_back(deviceImgBuffer);
	auto buf = fAllocateImgBuf(*deviceImgBuffer);
	return buf;
}

std::shared_ptr<IBuffer> IPrContext::AllocateTemporaryBuffer(vk::DeviceSize size,uint32_t alignment,const void *data)
{
	if(m_tmpBuffer == nullptr)
		return nullptr;
	auto allocatedBuffer = m_tmpBuffer->AllocateBuffer(size,alignment,data);
	if(allocatedBuffer)
		return allocatedBuffer;
	// Couldn't allocate from temp buffer, request size was most likely too large. Fall back to creating a completely new buffer.
	util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = MemoryFeatureFlags::CPUToGPU;
	createInfo.size = size;
	createInfo.usageFlags = BufferUsageFlags::IndexBufferBit | BufferUsageFlags::StorageBufferBit | 
		BufferUsageFlags::TransferDstBit | BufferUsageFlags::TransferSrcBit |
		BufferUsageFlags::UniformBufferBit | BufferUsageFlags::VertexBufferBit;
	return CreateBuffer(createInfo,data);
}
void IPrContext::AllocateTemporaryBuffer(prosper::IImage &img,const void *data)
{
	auto req = GetMemoryRequirements(img);
	auto buf = AllocateTemporaryBuffer(req.size,req.alignment,data);
	img.SetMemoryBuffer(*buf);
}

void IPrContext::ChangeResolution(uint32_t width,uint32_t height)
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

void IPrContext::OnResolutionChanged(uint32_t width,uint32_t height)
{
	if(m_callbacks.onResolutionChanged)
		m_callbacks.onResolutionChanged(width,height);
}
void IPrContext::OnWindowInitialized()
{
	if(m_callbacks.onWindowInitialized)
		m_callbacks.onWindowInitialized();
}
void IPrContext::OnSwapchainInitialized() {}

void IPrContext::SetPresentMode(prosper::PresentModeKHR presentMode)
{
	if(IsPresentationModeSupported(presentMode) == false)
	{
		switch(presentMode)
		{
			case prosper::PresentModeKHR::Immediate:
				throw std::runtime_error("Immediate present mode not supported!");
			default:
				return SetPresentMode(prosper::PresentModeKHR::Fifo);
				
		}
	}
	m_presentMode = presentMode;
}
prosper::PresentModeKHR IPrContext::GetPresentMode() const {return m_presentMode;}
void IPrContext::ChangePresentMode(prosper::PresentModeKHR presentMode)
{
	auto oldPresentMode = GetPresentMode();
	SetPresentMode(presentMode);
	if(oldPresentMode == GetPresentMode() || umath::is_flag_set(m_stateFlags,StateFlags::Initialized) == false)
		return;
	WaitIdle();
	ReloadSwapchain();
}

void IPrContext::GetScissorViewportInfo(Rect2D *out_scissors,Viewport *out_viewports)
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

const std::shared_ptr<prosper::IPrimaryCommandBuffer> &IPrContext::GetSetupCommandBuffer()
{
	if(m_setupCmdBuffer != nullptr)
		return m_setupCmdBuffer;
	uint32_t queueFamilyIndex;
	m_setupCmdBuffer = AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType::Universal,queueFamilyIndex);
	if(m_setupCmdBuffer == nullptr)
		throw std::runtime_error{"Unable to allocate setup command buffer!"};
	if(m_setupCmdBuffer->StartRecording(true,false) == false)
		throw std::runtime_error("Unable to start recording for primary level command buffer!");
	return m_setupCmdBuffer;
}

const std::shared_ptr<prosper::IPrimaryCommandBuffer> &IPrContext::GetDrawCommandBuffer() const {return m_commandBuffers.at(m_n_swapchain_image);}

const std::shared_ptr<prosper::IPrimaryCommandBuffer> &IPrContext::GetDrawCommandBuffer(uint32_t swapchainIdx) const
{
	static std::shared_ptr<prosper::IPrimaryCommandBuffer> nptr = nullptr;
	return (swapchainIdx < m_commandBuffers.size()) ? m_commandBuffers.at(swapchainIdx) : nptr;
}

void IPrContext::FlushSetupCommandBuffer()
{
	if(m_setupCmdBuffer == nullptr)
		return;
	DoFlushSetupCommandBuffer();
	m_setupCmdBuffer = nullptr;
}

void IPrContext::ClearKeepAliveResources(uint32_t n)
{
	if(m_n_swapchain_image >= m_keepAliveResources.size())
		return;
	auto &resources = m_keepAliveResources.at(m_n_swapchain_image);
	n = umath::min(static_cast<size_t>(n),resources.size());
	while(n-- > 0u)
		resources.erase(resources.begin());
}
void IPrContext::ClearKeepAliveResources()
{
	if(m_n_swapchain_image >= m_keepAliveResources.size())
		return;
	if(umath::is_flag_set(m_stateFlags,StateFlags::ClearingKeepAliveResources))
		throw std::logic_error("ClearKeepAliveResources mustn't be called by a resource destructor!");
	auto &resources = m_keepAliveResources.at(m_n_swapchain_image);
	umath::set_flag(m_stateFlags,StateFlags::ClearingKeepAliveResources);
	resources.clear();
	umath::set_flag(m_stateFlags,StateFlags::ClearingKeepAliveResources,false);
}
void IPrContext::KeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::Idle) || umath::is_flag_set(m_stateFlags,StateFlags::ClearingKeepAliveResources))
		return; // No need to keep resource around if device is currently idling (i.e. nothing is in progress)
	DoKeepResourceAliveUntilPresentationComplete(resource);
}

void IPrContext::WaitIdle()
{
	FlushSetupCommandBuffer();
	DoWaitIdle();
	umath::set_flag(m_stateFlags,StateFlags::Idle);
	ClearKeepAliveResources();
}

void IPrContext::Initialize(const CreateInfo &createInfo)
{
	// TODO: Check if resolution is supported
	m_windowCreationInfo->width = createInfo.width;
	m_windowCreationInfo->height = createInfo.height;
	ChangePresentMode(createInfo.presentMode);
	InitAPI(createInfo);
	InitBuffers();
	InitGfxPipelines();
	InitDummyTextures();
	InitDummyBuffer();
	umath::set_flag(m_stateFlags,StateFlags::Initialized);
	s_vertexBuffer = prosper::util::get_square_vertex_buffer(*this);
	s_uvBuffer = prosper::util::get_square_uv_buffer(*this);
	s_vertexUvBuffer = prosper::util::get_square_vertex_uv_buffer(*this);

	auto &shaderManager = GetShaderManager();
	shaderManager.RegisterShader("copy_image",[](prosper::IPrContext &context,const std::string &identifier) {return new ShaderCopyImage(context,identifier);});
	shaderManager.RegisterShader("blur_horizontal",[](prosper::IPrContext &context,const std::string &identifier) {return new ShaderBlurH(context,identifier);});
	shaderManager.RegisterShader("blur_vertical",[](prosper::IPrContext &context,const std::string &identifier) {return new ShaderBlurV(context,identifier);});
}

void IPrContext::InitBuffers() {}

void IPrContext::DrawFrame(prosper::IPrimaryCommandBuffer &cmd_buffer_ptr,uint32_t n_current_swapchain_image)
{
	if(m_callbacks.drawFrame)
		m_callbacks.drawFrame(cmd_buffer_ptr,n_current_swapchain_image);
}

ShaderManager &IPrContext::GetShaderManager() const {return *m_shaderManager;}

::util::WeakHandle<Shader> IPrContext::RegisterShader(const std::string &identifier,const std::function<Shader*(IPrContext&,const std::string&)> &fFactory) {return m_shaderManager->RegisterShader(identifier,fFactory);}
::util::WeakHandle<Shader> IPrContext::GetShader(const std::string &identifier) const {return m_shaderManager->GetShader(identifier);}

const std::shared_ptr<Texture> &IPrContext::GetDummyTexture() const {return m_dummyTexture;}
const std::shared_ptr<Texture> &IPrContext::GetDummyCubemapTexture() const {return m_dummyCubemapTexture;}
const std::shared_ptr<IBuffer> &IPrContext::GetDummyBuffer() const {return m_dummyBuffer;}
const std::shared_ptr<IDynamicResizableBuffer> &IPrContext::GetTemporaryBuffer() const {return m_tmpBuffer;}
const std::vector<std::shared_ptr<IDynamicResizableBuffer>> &IPrContext::GetDeviceImageBuffers() const {return m_deviceImgBuffers;}
void IPrContext::InitDummyBuffer()
{
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	createInfo.size = 1ull;
	createInfo.usageFlags = BufferUsageFlags::UniformBufferBit | BufferUsageFlags::StorageBufferBit | BufferUsageFlags::VertexBufferBit;
	m_dummyBuffer = CreateBuffer(createInfo);
	assert(m_dummyBuffer);
	m_dummyBuffer->SetDebugName("context_dummy_buf");
}
void IPrContext::InitDummyTextures()
{
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.format = Format::R8G8B8A8_UNorm;
	createInfo.height = 1u;
	createInfo.width = 1u;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	createInfo.postCreateLayout = ImageLayout::ShaderReadOnlyOptimal;
	createInfo.usage = ImageUsageFlags::SampledBit;
	auto img = CreateImage(createInfo);
	assert(img);
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	prosper::util::SamplerCreateInfo samplerCreateInfo {};
	m_dummyTexture = CreateTexture({},*img,imgViewCreateInfo,samplerCreateInfo);
	assert(m_dummyTexture);
	m_dummyTexture->SetDebugName("context_dummy_tex");

	createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;
	createInfo.layers = 6u;
	auto imgCubemap = CreateImage(createInfo);
	m_dummyCubemapTexture = CreateTexture({},*imgCubemap,imgViewCreateInfo,samplerCreateInfo);
	assert(m_dummyCubemapTexture);
	m_dummyCubemapTexture->SetDebugName("context_dummy_cubemap_tex");
}

void IPrContext::SubmitCommandBuffer(prosper::ICommandBuffer &cmd,bool shouldBlock,prosper::IFence *fence) {SubmitCommandBuffer(cmd,cmd.GetQueueFamilyType(),shouldBlock,fence);}

bool IPrContext::IsRecording() const {return umath::is_flag_set(m_stateFlags,StateFlags::IsRecording);}
bool IPrContext::ScheduleRecordUpdateBuffer(
	const std::shared_ptr<IBuffer> &buffer,uint64_t offset,uint64_t size,const void *data,const BufferUpdateInfo &updateInfo
)
{
	if(size == 0u)
		return true;
	if((buffer->GetUsageFlags() &prosper::BufferUsageFlags::TransferDstBit) == prosper::BufferUsageFlags::None)
		throw std::logic_error("Buffer has to have been created with VK_BUFFER_USAGE_TRANSFER_DST_BIT flag to allow buffer updates on command buffer!");
	const auto fRecordSrcBarrier = [this,buffer,updateInfo,offset,size](prosper::ICommandBuffer &cmdBuffer) {
		if(updateInfo.srcAccessMask.has_value())
		{
			cmdBuffer.RecordBufferBarrier(
				*buffer,
				*updateInfo.srcStageMask,PipelineStageFlags::TransferBit,
				*updateInfo.srcAccessMask,AccessFlags::TransferWriteBit,
				offset,size
			);
		}
	};
	const auto fUpdateBuffer = [](prosper::ICommandBuffer &cmdBuffer,prosper::IBuffer &buffer,const uint8_t *data,uint64_t offset,uint64_t size) {
		auto dataOffset = 0ull;
		const auto maxUpdateSize = 65'536u; // Maximum size allowed per vkCmdUpdateBuffer-call (see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html)
		do
		{
			auto updateSize = umath::min(static_cast<uint64_t>(maxUpdateSize),size);
			if(cmdBuffer.RecordUpdateBuffer(buffer,offset,updateSize,data +dataOffset) == false)
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
		if(prosper::util::get_current_render_pass_target(*drawCmd) == false) // Buffer updates are not allowed while a render pass is active! (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html)
		{
			fRecordSrcBarrier(*drawCmd);
			if(fUpdateBuffer(*drawCmd,*buffer,static_cast<const uint8_t*>(data),offset,size) == false)
				return false;
			if(updateInfo.postUpdateBarrierStageMask.has_value() && updateInfo.postUpdateBarrierAccessMask.has_value())
			{
				drawCmd->RecordBufferBarrier(
					*buffer,
					PipelineStageFlags::TransferBit,*updateInfo.postUpdateBarrierStageMask,
					AccessFlags::TransferWriteBit,*updateInfo.postUpdateBarrierAccessMask,
					offset,size
				);
			}
			return true;
		}
	}
	// We'll have to make a copy of the data for later
	auto ptrData = std::make_shared<std::vector<uint8_t>>(size);
	memcpy(ptrData->data(),data,size);
	m_scheduledBufferUpdates.push([buffer,fRecordSrcBarrier,fUpdateBuffer,offset,size,ptrData,updateInfo](prosper::ICommandBuffer &cmdBuffer) {
		fRecordSrcBarrier(cmdBuffer);
		fUpdateBuffer(cmdBuffer,*buffer,ptrData->data(),offset,size);

		if(updateInfo.postUpdateBarrierStageMask.has_value() && updateInfo.postUpdateBarrierAccessMask.has_value())
		{
			cmdBuffer.RecordBufferBarrier(
				*buffer,
				PipelineStageFlags::TransferBit,*updateInfo.postUpdateBarrierStageMask,
				AccessFlags::TransferWriteBit,*updateInfo.postUpdateBarrierAccessMask,
				offset,size
			);
		}
	});
	return true;
}

void IPrContext::InitTemporaryBuffer()
{
	auto bufferSize = 512ull *1'024ull *1'024ull; // 512 MiB
	auto maxBufferSize = 1ull *1'024ull *1'024ull *1'024ull; // 1 GiB
	util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = MemoryFeatureFlags::CPUToGPU;
	createInfo.size = bufferSize;
	createInfo.usageFlags = BufferUsageFlags::IndexBufferBit | BufferUsageFlags::StorageBufferBit | 
		BufferUsageFlags::TransferDstBit | BufferUsageFlags::TransferSrcBit |
		BufferUsageFlags::UniformBufferBit | BufferUsageFlags::VertexBufferBit;
	m_tmpBuffer = prosper::util::create_dynamic_resizable_buffer(*this,createInfo,maxBufferSize);
	assert(m_tmpBuffer);
	m_tmpBuffer->SetPermanentlyMapped(true);
}

void IPrContext::Draw(uint32_t n_swapchain_image)
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

void IPrContext::InitGfxPipelines() {}

const GLFW::WindowCreationInfo &IPrContext::GetWindowCreationInfo() const {return const_cast<IPrContext*>(this)->GetWindowCreationInfo();}
GLFW::WindowCreationInfo &IPrContext::GetWindowCreationInfo() {return *m_windowCreationInfo;}

void IPrContext::SetCallbacks(const Callbacks &callbacks) {m_callbacks = callbacks;}

void IPrContext::Run()
{
	auto t = ::util::Clock::now();
	while(true) // TODO
	{
		auto tNow = ::util::Clock::now();
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

static Anvil::ImageSubresourceRange to_anvil_subresource_range(const prosper::util::ImageSubresourceRange &range,prosper::IImage &img)
{
	Anvil::ImageSubresourceRange anvRange {};
	anvRange.base_array_layer = range.baseArrayLayer;
	anvRange.base_mip_level = range.baseMipLevel;
	anvRange.layer_count = range.layerCount;
	anvRange.level_count = range.levelCount;
	anvRange.aspect_mask = static_cast<Anvil::ImageAspectFlagBits>(prosper::util::get_aspect_mask(img));
	return anvRange;
}
#pragma optimize("",on)
