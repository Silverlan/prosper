/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_swap_command_buffer.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;
#pragma optimize("",off)
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
void ISwapCommandBufferGroup::Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw)
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
			m_conditionVar.wait(lock,[this]{return m_drawCall != nullptr || m_close;});

			if(m_close)
				break;
			m_drawCall(*m_curCommandBuffer);
			m_drawCall = nullptr;

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

void MtSwapCommandBufferGroup::Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw)
{
	ISwapCommandBufferGroup::Draw(rp,fb,draw);
	m_waitMutex.lock();
		m_pending = true;
		m_drawCall = [draw,&rp,&fb,this](prosper::ISecondaryCommandBuffer &drawCmd) {
			m_curCommandBuffer->Reset(false);
			m_curCommandBuffer->StartRecording(rp,fb);

			draw(drawCmd);

			m_curCommandBuffer->StopRecording();	
		};
		m_conditionVar.notify_one();
	m_waitMutex.unlock();
}

void MtSwapCommandBufferGroup::Wait() {while(IsPending());}

/////////////

StSwapCommandBufferGroup::StSwapCommandBufferGroup(prosper::IPrContext &context)
	: ISwapCommandBufferGroup{context}
{}

void StSwapCommandBufferGroup::Draw(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const RenderThreadDrawCall &draw)
{
	ISwapCommandBufferGroup::Draw(rp,fb,draw);
	m_drawCall = [draw,&rp,&fb,this](prosper::ISecondaryCommandBuffer &drawCmd) {
		m_curCommandBuffer->Reset(false);
		m_curCommandBuffer->StartRecording(rp,fb);

		draw(drawCmd);

		m_curCommandBuffer->StopRecording();	
	};
}

void StSwapCommandBufferGroup::Wait() {while(IsPending());}

bool StSwapCommandBufferGroup::ExecuteCommands(prosper::IPrimaryCommandBuffer &cmdBuf)
{
	if(m_drawCall)
	{
		m_drawCall(*m_curCommandBuffer);
		m_drawCall = nullptr;
	}
	return ISwapCommandBufferGroup::ExecuteCommands(cmdBuf);
}
#pragma optimize("",on)
