/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SWAP_COMMAND_BUFFER_HPP__
#define __PROSPER_SWAP_COMMAND_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace prosper {
	class IPrContext;
	class IPrimaryCommandBuffer;
	class IRenderPass;
	class IFramebuffer;
	class ISecondaryCommandBuffer;
	class ICommandBufferPool;

	using RenderThreadRecordCall = std::function<void(prosper::ISecondaryCommandBuffer &)>;
	class SwapCommandBufferGroup;
	class DLLPROSPER ISwapCommandBufferGroup {
	  public:
		ISwapCommandBufferGroup(Window &window);
		virtual ~ISwapCommandBufferGroup();
		virtual void Record(const RenderThreadRecordCall &record) = 0;
		virtual bool ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf);
		virtual bool IsPending() const = 0;
		void StartRecording(prosper::IRenderPass &rp, prosper::IFramebuffer &fb);
		void EndRecording();
		void SetOneTimeSubmit(bool oneTimeSubmit) { m_oneTimeSubmit = oneTimeSubmit; }
		bool GetOneTimeSubmit() const { return m_oneTimeSubmit; }

		void Reuse();
		prosper::IPrContext &GetContext() const;
	  protected:
		void Initialize(uint32_t swapchainIdx);
		virtual void Wait() = 0;
		;
		std::vector<std::shared_ptr<prosper::ISecondaryCommandBuffer>> m_commandBuffers;
		std::shared_ptr<prosper::ICommandBufferPool> m_cmdPool = nullptr;
		prosper::ISecondaryCommandBuffer *m_curCommandBuffer = nullptr;
		std::queue<RenderThreadRecordCall> m_recordCalls;
		bool m_oneTimeSubmit = true;
		std::weak_ptr<Window> m_window {};
		Window *m_windowPtr = nullptr;
	};

	class DLLPROSPER MtSwapCommandBufferGroup : public ISwapCommandBufferGroup {
	  public:
		MtSwapCommandBufferGroup(Window &window);
		virtual ~MtSwapCommandBufferGroup() override;
		virtual bool IsPending() const override { return m_pending; }
	  private:
		virtual void Record(const RenderThreadRecordCall &record) override;
		virtual void Wait() override;
		;

		std::thread m_thread;
		std::condition_variable m_conditionVar;
		std::mutex m_waitMutex;
		std::atomic<bool> m_close = false;
		std::atomic<bool> m_pending = false;
	};

	class DLLPROSPER StSwapCommandBufferGroup : public ISwapCommandBufferGroup {
	  public:
		StSwapCommandBufferGroup(Window &window);
		virtual bool ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf) override;
		virtual bool IsPending() const override { return false; }
	  private:
		virtual void Record(const RenderThreadRecordCall &record) override;
		virtual void Wait() override;
		;
	};
};

#endif
