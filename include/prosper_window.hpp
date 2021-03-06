/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_WINDOW_HPP__
#define __PROSPER_WINDOW_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"
#include <iglfw/glfw_window.h>

namespace prosper
{
	class DLLPROSPER Window
		: public ContextObject,
		public std::enable_shared_from_this<Window>
	{
	public:
		Window(const Window&)=delete;
		Window &operator=(const Window&)=delete;
		virtual ~Window() override;

		GLFW::Window *operator->();
		const GLFW::Window *operator->() const {return const_cast<Window*>(this)->operator->();}

		GLFW::Window &operator*();
		const GLFW::Window &operator*() const {return const_cast<Window*>(this)->operator*();}

		uint32_t GetSwapchainImageCount() const {return m_swapchainImages.size();}
		virtual uint32_t GetLastAcquiredSwapchainImageIndex() const=0;
		prosper::IImage *GetSwapchainImage(uint32_t idx);
		prosper::IFramebuffer *GetSwapchainFramebuffer(uint32_t idx);
		GLFW::Window &GetGlfwWindow() {return *m_glfwWindow;}
		
		std::shared_ptr<prosper::RenderTarget> &GetStagingRenderTarget();
		const std::shared_ptr<prosper::RenderTarget> &GetStagingRenderTarget() const {return const_cast<Window*>(this)->GetStagingRenderTarget();}
		void ReloadStagingRenderTarget();

		void Close();
		bool IsValid() const;
		void UpdateWindow();

		const WindowSettings &GetWindowSettings() const {return m_settings;}
		void SetWindowedMode(bool b);
		void SetRefreshRate(uint32_t rate);
		void SetNoBorder(bool b);
		void SetResolution(const Vector2i &sz);
		void SetResolutionWidth(uint32_t w);
		void SetResolutionHeight(uint32_t h);
		void SetMonitor(GLFW::Monitor &monitor);
		void SetPresentMode(prosper::PresentModeKHR presentMode);
		float GetAspectRatio() const;
		void ReloadSwapchain();

		void SetInitCallback(const std::function<void()> &callback) {m_initCallback = callback;}
		const std::function<void()> &GetInitCallback() const {return m_initCallback;}
		void SetCloseCallback(const std::function<void()> &callback) {m_closeCallback = callback;}
		const std::function<void()> &GetCloseCallback() const {return m_closeCallback;}

		prosper::ISwapCommandBufferGroup &GetSwapCommandBufferGroup() {return *m_guiCommandBufferGroup;}
	protected:
		struct WindowChangeInfo
		{
			std::optional<bool> windowedMode = {};
			std::optional<uint32_t> refreshRate = {};
			std::optional<bool> decorated = {};
			std::optional<uint32_t> width = {};
			std::optional<uint32_t> height = {};
			std::optional<GLFW::Monitor> monitor = {};
			std::optional<prosper::PresentModeKHR> presentMode = {};
		};
		Window(IPrContext &context,const WindowSettings &windowCreationInfo);
		virtual void InitWindow()=0;
		virtual void ReleaseWindow()=0;
		virtual void InitSwapchain()=0;
		virtual void ReleaseSwapchain()=0;
		virtual void Release();
		virtual void OnWindowInitialized();
		void ReloadWindow();

		WindowChangeInfo &ScheduleWindowReload();
		std::unique_ptr<WindowChangeInfo> m_scheduledWindowReloadInfo = nullptr;

		std::function<void()> m_initCallback = nullptr;
		std::function<void()> m_closeCallback = nullptr;
		WindowSettings m_settings {};
		std::unique_ptr<GLFW::Window> m_glfwWindow = nullptr;
		std::shared_ptr<prosper::ISwapCommandBufferGroup> m_guiCommandBufferGroup = nullptr;
		
		float m_aspectRatio = 1.f;
		std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
		std::vector<std::shared_ptr<prosper::IImage>> m_swapchainImages {};
		std::vector<std::shared_ptr<prosper::IFramebuffer>> m_swapchainFramebuffers {};
		uint32_t m_lastSemaporeUsed = 0u;
	};
};

#endif
