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

namespace prosper
{
	class IPrContext;
	class IPrimaryCommandBuffer;
	class IRenderPass;
	class IFramebuffer;
	class ISecondaryCommandBuffer;
	class ICommandBufferPool;

	using RenderThreadDrawCall = std::function<void(prosper::ISecondaryCommandBuffer&)>;
	class SwapCommandBufferGroup;
	class DLLPROSPER ISwapCommandBufferGroup
	{
	public:
		ISwapCommandBufferGroup(prosper::IPrContext &context);
		virtual ~ISwapCommandBufferGroup();
		virtual void Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw)=0;
		virtual bool ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf);
		virtual bool IsPending() const=0;
		void SetOneTimeSubmit(bool oneTimeSubmit) {m_oneTimeSubmit = oneTimeSubmit;}
		bool GetOneTimeSubmit() const {return m_oneTimeSubmit;}

		void Reuse();
		prosper::IPrContext &GetContext() const {return m_context;}
	protected:
		void Initialize(uint32_t swapchainIdx);
		virtual void Wait()=0;;
		prosper::IPrContext &m_context;
		std::vector<std::shared_ptr<prosper::ISecondaryCommandBuffer>> m_commandBuffers;
		std::shared_ptr<prosper::ICommandBufferPool> m_cmdPool = nullptr;
		prosper::ISecondaryCommandBuffer *m_curCommandBuffer = nullptr;
		RenderThreadDrawCall m_drawCall;
		bool m_oneTimeSubmit = true;
	};

	class DLLPROSPER MtSwapCommandBufferGroup
		: public ISwapCommandBufferGroup
	{
	public:
		MtSwapCommandBufferGroup(prosper::IPrContext &context);
		virtual ~MtSwapCommandBufferGroup() override;
		virtual bool IsPending() const override {return m_pending;}
	private:
		virtual void Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw) override;
		virtual void Wait() override;;

		std::thread m_thread;
		std::condition_variable m_conditionVar;
		std::mutex m_waitMutex;
		std::atomic<bool> m_close = false;
		std::atomic<bool> m_pending = false;
	};

	class DLLPROSPER StSwapCommandBufferGroup
		: public ISwapCommandBufferGroup
	{
	public:
		StSwapCommandBufferGroup(prosper::IPrContext &context);
		virtual bool ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf) override;
		virtual bool IsPending() const override {return false;}
	private:
		virtual void Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw) override;
		virtual void Wait() override;;
	};
};

#endif
