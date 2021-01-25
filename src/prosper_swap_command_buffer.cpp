/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_swap_command_buffer.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

ISwapCommandBufferGroup::ISwapCommandBufferGroup(prosper::IPrContext &context)
	: m_context{context}
{
	m_cmdPool = m_context.CreateCommandBufferPool(prosper::QueueFamilyType::Universal);
}

ISwapCommandBufferGroup::~ISwapCommandBufferGroup()
{
	m_commandBuffers.clear();
	m_cmdPool = nullptr;
}

void ISwapCommandBufferGroup::Initialize(uint32_t swapchainIdx)
{
	auto numSwapchains = swapchainIdx +1;
	if(numSwapchains <= m_commandBuffers.size())
		return;
	m_commandBuffers.reserve(numSwapchains);
	for(auto i=decltype(m_commandBuffers.size()){m_commandBuffers.size()};i<numSwapchains;++i)
	{
		auto cmdBuf = m_cmdPool->AllocateSecondaryCommandBuffer();
		m_commandBuffers.push_back(cmdBuf);
	}
}
void ISwapCommandBufferGroup::StartRecording(prosper::IRenderPass &rp,prosper::IFramebuffer &fb)
{
	auto swapchainIdx = m_context.GetLastAcquiredSwapchainImageIndex();
	Initialize(swapchainIdx);

	auto *instance = m_commandBuffers[swapchainIdx].get();
	m_curCommandBuffer = instance;

	Record([this,&rp,&fb](prosper::ISecondaryCommandBuffer &cmd) {
		cmd.Reset(false);
		cmd.StartRecording(rp,fb,GetOneTimeSubmit());
	});
}
void ISwapCommandBufferGroup::EndRecording()
{
	Record([](prosper::ISecondaryCommandBuffer &cmd) {
		cmd.StopRecording();	
	});
}
void ISwapCommandBufferGroup::Reuse()
{
	auto swapchainIdx = m_context.GetLastAcquiredSwapchainImageIndex();
	Initialize(swapchainIdx);

	auto *instance = m_commandBuffers[swapchainIdx].get();
	m_curCommandBuffer = instance;
}
bool ISwapCommandBufferGroup::ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf)
{
	Wait();
	assert(m_curInstance);
	if(m_curCommandBuffer == nullptr)
		return false;
	auto result = cmdBuf.ExecuteCommands(*m_curCommandBuffer);
	m_curCommandBuffer = nullptr;
	return result;
}

/////////////

MtSwapCommandBufferGroup::MtSwapCommandBufferGroup(prosper::IPrContext &context)
	: ISwapCommandBufferGroup{context}
{
	m_thread = std::thread{[this]() {
		while(!m_close)
		{
			std::unique_lock<std::mutex> lock {m_waitMutex};
			m_conditionVar.wait(lock,[this]{return !m_recordCalls.empty() || m_close;});

			if(m_close)
				break;
			while(m_recordCalls.empty() == false)
			{
				auto &drawCall = m_recordCalls.front();
				drawCall(*m_curCommandBuffer);
				m_recordCalls.pop();
			}

			m_pending = false;
		}
	}};
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

void MtSwapCommandBufferGroup::Wait() {while(IsPending());}

/////////////

StSwapCommandBufferGroup::StSwapCommandBufferGroup(prosper::IPrContext &context)
	: ISwapCommandBufferGroup{context}
{}

void StSwapCommandBufferGroup::Record(const RenderThreadRecordCall &record)
{
	m_recordCalls.push(record);
}

void StSwapCommandBufferGroup::Wait() {while(IsPending());}

bool StSwapCommandBufferGroup::ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf)
{
	while(m_recordCalls.empty() == false)
	{
		auto &drawCall = m_recordCalls.front();
		drawCall(*m_curCommandBuffer);
		m_recordCalls.pop();
	}
	return ISwapCommandBufferGroup::ExecuteCommands(cmdBuf);
}
