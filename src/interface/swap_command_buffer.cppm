// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:swap_command_buffer;

export import :types;

export namespace prosper {
	class IPrContext;
	class IPrimaryCommandBuffer;
	class IRenderPass;
	class IFramebuffer;
	class ISecondaryCommandBuffer;
	class ICommandBufferPool;

	using RenderThreadRecordCall = std::function<void(ISecondaryCommandBuffer &)>;
	class SwapCommandBufferGroup;
	class DLLPROSPER ISwapCommandBufferGroup {
	  public:
		ISwapCommandBufferGroup(Window &window, const std::string &debugName = {});
		virtual ~ISwapCommandBufferGroup();
		virtual void Record(const RenderThreadRecordCall &record) = 0;
		virtual bool ExecuteCommands(IPrimaryCommandBuffer &cmdBuf);
		virtual bool IsPending() const = 0;
		void StartRecording(IRenderPass &rp, IFramebuffer &fb);
		void EndRecording();
		void SetOneTimeSubmit(bool oneTimeSubmit) { m_oneTimeSubmit = oneTimeSubmit; }
		bool GetOneTimeSubmit() const { return m_oneTimeSubmit; }
		virtual void Wait() = 0;

		void Reuse();
		IPrContext &GetContext() const;
	  protected:
		void Initialize(uint32_t swapchainIdx);

		std::vector<std::shared_ptr<ISecondaryCommandBuffer>> m_commandBuffers;
		std::shared_ptr<ICommandBufferPool> m_cmdPool = nullptr;
		ISecondaryCommandBuffer *m_curCommandBuffer = nullptr;
		std::queue<RenderThreadRecordCall> m_recordCalls;
		bool m_oneTimeSubmit = true;
		std::weak_ptr<Window> m_window {};
		Window *m_windowPtr = nullptr;
		std::string m_debugName;
	};

	class DLLPROSPER MtSwapCommandBufferGroup : public ISwapCommandBufferGroup {
	  public:
		MtSwapCommandBufferGroup(Window &window, const std::string &debugName = {});
		virtual ~MtSwapCommandBufferGroup() override;
		virtual bool IsPending() const override { return m_pending; }
		virtual void Wait() override;
	  private:
		virtual void Record(const RenderThreadRecordCall &record) override;

		std::thread m_thread;
		std::condition_variable m_conditionVar;
		std::mutex m_waitMutex;
		std::atomic<bool> m_close = false;
		std::atomic<bool> m_pending = false;
	};

	class DLLPROSPER StSwapCommandBufferGroup : public ISwapCommandBufferGroup {
	  public:
		StSwapCommandBufferGroup(Window &window, const std::string &debugName = {});
		virtual bool ExecuteCommands(IPrimaryCommandBuffer &cmdBuf) override;
		virtual bool IsPending() const override { return false; }
		virtual void Wait() override;
	  private:
		virtual void Record(const RenderThreadRecordCall &record) override;
		;
	};
};
