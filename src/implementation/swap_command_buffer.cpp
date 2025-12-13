// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :swap_command_buffer;

using namespace prosper;

ISwapCommandBufferGroup::ISwapCommandBufferGroup(Window &window, const std::string &debugName) : m_window {window.shared_from_this()}, m_debugName {debugName}, m_windowPtr {&window} { m_cmdPool = window.GetContext().CreateCommandBufferPool(QueueFamilyType::Universal); }

ISwapCommandBufferGroup::~ISwapCommandBufferGroup()
{
	m_commandBuffers.clear();
	m_cmdPool = nullptr;
}

IPrContext &ISwapCommandBufferGroup::GetContext() const
{
	if(m_window.expired())
		throw std::runtime_error {"Invalid swap command buffer group window!"};
	return m_windowPtr->GetContext();
}

void ISwapCommandBufferGroup::Initialize(uint32_t swapchainIdx)
{
	auto numSwapchains = swapchainIdx + 1;
	if(numSwapchains <= m_commandBuffers.size())
		return;
	m_commandBuffers.reserve(numSwapchains);
	for(auto i = decltype(m_commandBuffers.size()) {m_commandBuffers.size()}; i < numSwapchains; ++i) {
		auto cmdBuf = m_cmdPool->AllocateSecondaryCommandBuffer();
		cmdBuf->SetDebugName(m_debugName + "_" + std::to_string(i));
		m_commandBuffers.push_back(cmdBuf);
	}
}
void ISwapCommandBufferGroup::StartRecording(IRenderPass &rp, IFramebuffer &fb)
{
	if(m_window.expired())
		return;
	Wait();
	auto swapchainIdx = m_windowPtr->GetLastAcquiredSwapchainImageIndex();
	if(swapchainIdx == std::numeric_limits<decltype(swapchainIdx)>::max())
		return; // TODO: This should be unreachable
	Initialize(swapchainIdx);

	auto *instance = m_commandBuffers[swapchainIdx].get();
	m_curCommandBuffer = instance;

	Record([this, &rp, &fb](ISecondaryCommandBuffer &cmd) {
		cmd.Reset(false);
		cmd.StartRecording(rp, fb, GetOneTimeSubmit());
	});
}
void ISwapCommandBufferGroup::EndRecording()
{
	Record([](ISecondaryCommandBuffer &cmd) { cmd.StopRecording(); });
}
void ISwapCommandBufferGroup::Reuse()
{
	if(m_window.expired())
		return;
	Wait();
	auto swapchainIdx = m_windowPtr->GetLastAcquiredSwapchainImageIndex();
	Initialize(swapchainIdx);

	auto *instance = m_commandBuffers[swapchainIdx].get();
	m_curCommandBuffer = instance;
}
bool ISwapCommandBufferGroup::ExecuteCommands(IPrimaryCommandBuffer &cmdBuf)
{
	Wait();
	if(m_curCommandBuffer == nullptr)
		return false;
	auto result = cmdBuf.ExecuteCommands(*m_curCommandBuffer);
	m_curCommandBuffer = nullptr;
	return result;
}

/////////////

MtSwapCommandBufferGroup::MtSwapCommandBufferGroup(Window &window, const std::string &debugName) : ISwapCommandBufferGroup {window, debugName}
{
	m_thread = std::thread {[this]() {
		while(!m_close) {
			std::unique_lock<std::mutex> lock {m_waitMutex};
			if(m_recordCalls.empty() && m_pending) {
				m_pending = false;
				m_conditionVar.notify_all();
			}
			m_conditionVar.wait(lock, [this] { return !m_recordCalls.empty() || m_close; });

			if(m_close)
				break;
			if(!m_curCommandBuffer) {
				while(!m_recordCalls.empty())
					m_recordCalls.pop();
			}
			else {
				auto drawCall = std::move(m_recordCalls.front());
				m_recordCalls.pop();
				lock.unlock();

				drawCall(*m_curCommandBuffer);
			}
		}
	}};
	pragma::util::set_thread_name(m_thread, debugName);
}

MtSwapCommandBufferGroup::~MtSwapCommandBufferGroup()
{
	m_close = true;
	m_waitMutex.lock();
	m_conditionVar.notify_one();
	m_waitMutex.unlock();
	m_thread.join();
}

void MtSwapCommandBufferGroup::Record(const RenderThreadRecordCall &record)
{
	m_waitMutex.lock();

	m_pending = true;
	m_recordCalls.push(record);
	m_conditionVar.notify_one();
	m_waitMutex.unlock();
	if(GetContext().IsMultiThreadedRenderingEnabled() == false)
		Wait();
}

void MtSwapCommandBufferGroup::Wait()
{
	std::unique_lock<std::mutex> lock {m_waitMutex};
	m_conditionVar.wait(lock, [this] { return !m_pending; });
}

/////////////

StSwapCommandBufferGroup::StSwapCommandBufferGroup(Window &window, const std::string &debugName) : ISwapCommandBufferGroup {window, debugName} {}

void StSwapCommandBufferGroup::Record(const RenderThreadRecordCall &record) { m_recordCalls.push(record); }

void StSwapCommandBufferGroup::Wait() { assert(IsPending() == false); }

bool StSwapCommandBufferGroup::ExecuteCommands(IPrimaryCommandBuffer &cmdBuf)
{
	if(m_curCommandBuffer == nullptr)
		return false;
	while(m_recordCalls.empty() == false) {
		auto &drawCall = m_recordCalls.front();
		drawCall(*m_curCommandBuffer);
		m_recordCalls.pop();
	}
	return ISwapCommandBufferGroup::ExecuteCommands(cmdBuf);
}
