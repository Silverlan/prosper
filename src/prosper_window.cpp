/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_window.hpp"
#include "image/prosper_render_target.hpp"

using namespace prosper;
#pragma optimize("",off)
prosper::Window::Window(IPrContext &context,const WindowSettings &windowCreationInfo)
	: ContextObject{context},m_settings{windowCreationInfo}
{
	m_guiCommandBufferGroup = context.CreateSwapCommandBufferGroup();
}
Window::~Window() {}

void Window::OnWindowInitialized()
{
	m_scheduledWindowReloadInfo = nullptr;
	if(m_initCallback)
		m_initCallback();
}

void Window::Release()
{
	GetContext().WaitIdle();
	ReleaseWindow();
	m_swapchainImages.clear();

	m_glfwWindow = nullptr;
	m_guiCommandBufferGroup = nullptr;
		
	m_stagingRenderTarget = nullptr;
	m_lastSemaporeUsed = 0u;
}

void Window::Close()
{
	if(m_closed)
		return;
	m_closed = true;

	for(auto &cb : m_closeListeners)
		cb();
	m_closeListeners.clear();

	Release();

	for(auto &cb : m_closedListeners)
		cb();
	m_closedListeners.clear();
}

bool Window::IsValid() const {return m_glfwWindow != nullptr;}

void Window::SetMonitor(GLFW::Monitor &monitor)
{
	auto &creationInfo = m_settings;
	if(creationInfo.monitor.has_value() && &monitor == &*creationInfo.monitor)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.monitor = monitor;
}
void Window::SetPresentMode(prosper::PresentModeKHR presentMode)
{
	auto &creationInfo = m_settings;
	if(presentMode == creationInfo.presentMode)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.presentMode = presentMode;
}
void Window::SetWindowedMode(bool b)
{
	auto &creationInfo = m_settings;
	if(b == creationInfo.windowedMode)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.windowedMode = b;
}
void Window::SetRefreshRate(uint32_t rate)
{
	auto &creationInfo = m_settings;
	if(rate == creationInfo.refreshRate)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.refreshRate = rate;
}
void Window::SetNoBorder(bool b)
{
	auto &creationInfo = m_settings;
	if(!b == creationInfo.decorated)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.decorated = !b;
}
void Window::SetResolution(const Vector2i &sz)
{
	auto &creationInfo = m_settings;
	if(sz.x == creationInfo.width && sz.y == creationInfo.height)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.width = sz.x;
	changeInfo.height = sz.y;
}
void Window::SetResolutionWidth(uint32_t w)
{
	auto &creationInfo = m_settings;
	if(w == creationInfo.width)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.width = w;
}
void Window::SetResolutionHeight(uint32_t h)
{
	auto &creationInfo = m_settings;
	if(h == creationInfo.height)
		return;
	auto &changeInfo = ScheduleWindowReload();
	changeInfo.width = h;
}
float Window::GetAspectRatio() const {return m_aspectRatio;}

void Window::UpdateWindow()
{
	if(m_scheduledWindowReloadInfo == nullptr)
		return;
	GetContext().WaitIdle();
	auto &creationInfo = m_settings;
	if(m_scheduledWindowReloadInfo->windowedMode.has_value())
		creationInfo.windowedMode = *m_scheduledWindowReloadInfo->windowedMode;
	if(m_scheduledWindowReloadInfo->refreshRate.has_value())
		creationInfo.refreshRate = (*m_scheduledWindowReloadInfo->refreshRate != 0u) ? static_cast<int32_t>(*m_scheduledWindowReloadInfo->refreshRate) : GLFW_DONT_CARE;
	if(m_scheduledWindowReloadInfo->decorated.has_value())
		creationInfo.decorated = *m_scheduledWindowReloadInfo->decorated;
	if(m_scheduledWindowReloadInfo->width.has_value())
		creationInfo.width = *m_scheduledWindowReloadInfo->width;
	if(m_scheduledWindowReloadInfo->height.has_value())
		creationInfo.height = *m_scheduledWindowReloadInfo->height;
	m_aspectRatio = CFloat(creationInfo.width) /CFloat(creationInfo.height);
	if(m_scheduledWindowReloadInfo->monitor.has_value())
		creationInfo.monitor = m_scheduledWindowReloadInfo->monitor;
	if(m_scheduledWindowReloadInfo->presentMode.has_value())
		creationInfo.presentMode = *m_scheduledWindowReloadInfo->presentMode;
	m_scheduledWindowReloadInfo = nullptr;
	if(creationInfo.windowedMode == false)
	{
		if(!creationInfo.monitor.has_value())
			creationInfo.monitor = GLFW::get_primary_monitor();
	}
	else
		creationInfo.monitor = nullptr;
	ReloadWindow();
}

void Window::ReloadSwapchain()
{
	ReleaseSwapchain();
	InitSwapchain();
}

void Window::ReloadWindow()
{
	std::optional<GLFW::CallbackInterface> callbacks {};
	std::optional<Vector2> cursorPosOverride {};
	if(m_glfwWindow)
	{
		callbacks = m_glfwWindow->GetCallbacks();
		cursorPosOverride = m_glfwWindow->GetCursorPosOverride();
	}
	ReleaseWindow();
	InitWindow();
	ReloadSwapchain();
	// Restore callbacks and override position
	if(m_glfwWindow && callbacks.has_value())
	{
		m_glfwWindow->SetCallbacks(*callbacks);
		if(cursorPosOverride.has_value())
			m_glfwWindow->SetCursorPosOverride(*cursorPosOverride);
	}
}

Window::WindowChangeInfo &Window::ScheduleWindowReload()
{
	if(m_scheduledWindowReloadInfo == nullptr)
		m_scheduledWindowReloadInfo = std::unique_ptr<WindowChangeInfo>(new WindowChangeInfo{});
	return *m_scheduledWindowReloadInfo;
}

void Window::ReloadStagingRenderTarget()
{
	auto &context = GetContext();
	context.WaitIdle();
	m_stagingRenderTarget = nullptr;

	auto resolution = (*this)->GetSize();
	resolution.x = umath::max(resolution.x,1);
	resolution.y = umath::max(resolution.y,1);
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.usage = prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::SampledBit;
	auto colorFormat = createInfo.format = STAGING_RENDER_TARGET_COLOR_FORMAT;
	createInfo.width = resolution.x;
	createInfo.height = resolution.y;
	createInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
	auto stagingImg = context.CreateImage(createInfo);
	
	createInfo.usage = prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::DepthStencilAttachmentBit | prosper::ImageUsageFlags::SampledBit;
	auto depthStencilFormat = createInfo.format = STAGING_RENDER_TARGET_DEPTH_STENCIL_FORMAT;
	createInfo.postCreateLayout = prosper::ImageLayout::DepthStencilAttachmentOptimal;
	auto depthStencilImg = context.CreateImage(createInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto stagingTex = context.CreateTexture({},*stagingImg,imgViewCreateInfo);
	imgViewCreateInfo.aspectFlags = prosper::ImageAspectFlags::StencilBit;
	auto depthStencilTex = context.CreateTexture({},*depthStencilImg,imgViewCreateInfo);

	auto rp = context.CreateRenderPass(
		prosper::util::RenderPassCreateInfo{
			{
				prosper::util::RenderPassCreateInfo::AttachmentInfo{
					colorFormat,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
					prosper::AttachmentStoreOp::Store,prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::ColorAttachmentOptimal
				},
				prosper::util::RenderPassCreateInfo::AttachmentInfo{
					depthStencilFormat,prosper::ImageLayout::DepthStencilAttachmentOptimal,
					prosper::AttachmentLoadOp::DontCare,prosper::AttachmentStoreOp::DontCare,
					prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::DepthStencilAttachmentOptimal,
					prosper::AttachmentLoadOp::Clear,prosper::AttachmentStoreOp::Store
				}
			}
	});
	
	m_stagingRenderTarget = context.CreateRenderTarget({stagingTex,depthStencilTex},rp);//,finalDepthTex},rp);
	m_stagingRenderTarget->SetDebugName("engine_staging_rt");
	// Vulkan TODO: Resize when window resolution was changed
}

GLFW::Window *Window::operator->() {return m_glfwWindow.get();}
GLFW::Window &Window::operator*() {return *operator->();}

prosper::IRenderPass &Window::GetStagingRenderPass() const
{
	auto rt = GetStagingRenderTarget();
	return rt->GetRenderPass();
}

std::shared_ptr<prosper::RenderTarget> &Window::GetStagingRenderTarget()
{
	if(!m_stagingRenderTarget)
		const_cast<Window*>(this)->ReloadStagingRenderTarget();
	return m_stagingRenderTarget;
}

prosper::IImage *Window::GetSwapchainImage(uint32_t idx)
{
	return (idx < m_swapchainImages.size()) ? m_swapchainImages.at(idx).get() : nullptr;
}

prosper::IFramebuffer *Window::GetSwapchainFramebuffer(uint32_t idx)
{
	return (idx < m_swapchainFramebuffers.size()) ? m_swapchainFramebuffers.at(idx).get() : nullptr;
}
#pragma optimize("",on)
