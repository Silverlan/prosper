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

namespace prosper {
	class DLLPROSPER Window : public ContextObject, public std::enable_shared_from_this<Window> {
	  public:
		static constexpr auto STAGING_RENDER_TARGET_COLOR_FORMAT = prosper::Format::R8G8B8A8_UNorm;
		static constexpr auto STAGING_RENDER_TARGET_DEPTH_STENCIL_FORMAT = prosper::Format::D32_SFloat_S8_UInt;

		enum class State : uint8_t { Active = 0, Inactive };

		Window(const Window &) = delete;
		Window &operator=(const Window &) = delete;
		virtual ~Window() override;

		GLFW::Window *operator->();
		const GLFW::Window *operator->() const { return const_cast<Window *>(this)->operator->(); }

		GLFW::Window &operator*();
		const GLFW::Window &operator*() const { return const_cast<Window *>(this)->operator*(); }

		uint32_t GetSwapchainImageCount() const { return m_swapchainImages.size(); }
		virtual uint32_t GetLastAcquiredSwapchainImageIndex() const = 0;
		prosper::IImage *GetSwapchainImage(uint32_t idx);
		prosper::IFramebuffer *GetSwapchainFramebuffer(uint32_t idx);
		GLFW::Window &GetGlfwWindow() { return *m_glfwWindow; }

		prosper::IRenderPass &GetStagingRenderPass() const;
		std::shared_ptr<prosper::RenderTarget> &GetStagingRenderTarget();
		const std::shared_ptr<prosper::RenderTarget> &GetStagingRenderTarget() const { return const_cast<Window *>(this)->GetStagingRenderTarget(); }
		void ReloadStagingRenderTarget();

		void Close();
		bool IsValid() const;
		void UpdateWindow();

		const WindowSettings &GetWindowSettings() const { return m_settings; }
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

		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetDrawCommandBuffer() const;
		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetDrawCommandBuffer(uint32_t swapchainIdx) const;

		State GetState() const { return m_state; }
		void SetState(State state) { m_state = state; }

		void SetInitCallback(const std::function<void()> &callback) { m_initCallback = callback; }
		const std::function<void()> &GetInitCallback() const { return m_initCallback; }

		void AddCloseListener(const std::function<void()> &callback) { m_closeListeners.push_back(callback); }
		const std::vector<std::function<void()>> &GetCloseListeners() const { return m_closeListeners; }

		void AddClosedListener(const std::function<void()> &callback) { m_closedListeners.push_back(callback); }
		const std::vector<std::function<void()>> &GetClosedListeners() const { return m_closedListeners; }

		prosper::ISwapCommandBufferGroup &GetSwapCommandBufferGroup() { return *m_guiCommandBufferGroup; }
		bool ScheduledForClose() const;
	  protected:
		struct WindowChangeInfo {
			std::optional<bool> windowedMode = {};
			std::optional<uint32_t> refreshRate = {};
			std::optional<bool> decorated = {};
			std::optional<uint32_t> width = {};
			std::optional<uint32_t> height = {};
			std::optional<GLFW::Monitor> monitor = {};
			std::optional<prosper::PresentModeKHR> presentMode = {};
		};
		friend IPrContext;
		Window(IPrContext &context, const WindowSettings &windowCreationInfo);
		void DoClose();
		virtual void InitWindow() = 0;
		virtual void ReleaseWindow() = 0;
		virtual void InitCommandBuffers() = 0;
		void InitSwapchain();
		void ReleaseSwapchain();
		virtual void OnSwapchainInitialized();
		virtual void DoInitSwapchain() = 0;
		virtual void DoReleaseSwapchain() = 0;
		virtual void Release();
		virtual void OnWindowInitialized();
		void ReloadWindow();

		WindowChangeInfo &ScheduleWindowReload();
		std::unique_ptr<WindowChangeInfo> m_scheduledWindowReloadInfo = nullptr;
		std::vector<std::shared_ptr<prosper::IPrimaryCommandBuffer>> m_commandBuffers;

		std::function<void()> m_initCallback = nullptr;
		std::vector<std::function<void()>> m_closeListeners;
		std::vector<std::function<void()>> m_closedListeners;
		WindowSettings m_settings {};
		std::unique_ptr<GLFW::Window> m_glfwWindow = nullptr;
		std::shared_ptr<prosper::ISwapCommandBufferGroup> m_guiCommandBufferGroup = nullptr;

		State m_state = State::Active;
		float m_aspectRatio = 1.f;
		std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
		std::vector<std::shared_ptr<prosper::IImage>> m_swapchainImages {};
		std::vector<std::shared_ptr<prosper::IFramebuffer>> m_swapchainFramebuffers {};
		uint32_t m_lastSemaporeUsed = 0u;
		bool m_closed = false;
	};
};

#endif
