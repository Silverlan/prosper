/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_pipeline_loader.hpp"
#include "shader/prosper_shader.hpp"

using namespace prosper;
#pragma optimize("",off)
ShaderPipelineLoader::ShaderPipelineLoader(prosper::IPrContext &context)
	: m_context{context},m_multiThreaded{context.IsMultiThreadedRenderingEnabled()}
{
	if(m_multiThreaded)
	{
		m_initThread = std::thread{[this]() {
			while(m_running)
				ProcessInitQueue();
		}};
		m_bakeThread = std::thread{[this]() {
			while(m_running)
				ProcessBakeQueue();
		}};
	}
}
ShaderPipelineLoader::~ShaderPipelineLoader()
{
	m_running = false;
	if(!m_multiThreaded)
		return;
	m_initPending.notify_one();
	m_bakePending.notify_one();
	m_initThread.join();
	m_bakeThread.join();
}

void ShaderPipelineLoader::ProcessInitQueue()
{
	std::queue<InitItem> initQueue;
	if(m_multiThreaded)
	{
		std::unique_lock<std::mutex> lock {m_initQueueMutex};
		m_initPending.wait(lock,[this]() {
			return !m_initQueue.empty() || !m_running;
		});
		if(!m_running)
			return;
		initQueue = std::move(m_initQueue);
		m_initQueue = {};
	}
	else
	{
		initQueue = std::move(m_initQueue);
		m_initQueue = {};
	}
	auto n = initQueue.size();
	while(!initQueue.empty())
	{
		auto &item = initQueue.front();
		auto res = item.init();
		if(res)
			m_shadersPendingForFinalization.insert(item.shaderIndex);
		AddPendingShaderJobCount(item.shaderIndex,-1);
		initQueue.pop();
	}
	m_pendingWorkMutex.lock();
		m_pendingWork -= n;
		m_pendingWorkCondition.notify_one();
	m_pendingWorkMutex.unlock();
}
void ShaderPipelineLoader::ProcessBakeQueue()
{
	std::queue<BakeItem> bakeQueue;
	if(m_multiThreaded)
	{
		std::unique_lock<std::mutex> lock {m_bakeQueueMutex};
		m_bakePending.wait(lock,[this]() {
			return !m_bakeQueue.empty() || !m_running;
		});
		if(!m_running)
			return;
		bakeQueue = std::move(m_bakeQueue);
		m_bakeQueue = {};
	}
	else
	{
		bakeQueue = std::move(m_bakeQueue);
		m_bakeQueue = {};
	}
	auto n = bakeQueue.size();
	while(!bakeQueue.empty())
	{
		auto &item = bakeQueue.front();
		m_context.BakeShaderPipeline(item.pipelineId,item.pipelineBindPoint);
		AddPendingShaderJobCount(item.shaderIndex,-1);
		bakeQueue.pop();
	}
	m_pendingWorkMutex.lock();
		m_pendingWork -= n;
		m_pendingWorkCondition.notify_one();
	m_pendingWorkMutex.unlock();
}
bool ShaderPipelineLoader::IsShaderQueued(ShaderIndex shaderIndex) const
{
	std::scoped_lock lock {m_pendingShaderJobsMutex};
	auto it = m_pendingShaderJobs.find(shaderIndex);
	return it != m_pendingShaderJobs.end() && it->second > 0;
		
}
void ShaderPipelineLoader::Flush()
{
	if(m_pendingWork > 0)
	{
		if(std::this_thread::get_id() != m_context.GetMainThreadId())
			throw std::runtime_error{"Pipeline loader must not be flushed from non-main thread!"};
		std::unique_lock<std::mutex> lock {m_pendingWorkMutex};
		m_pendingWorkCondition.wait(lock,[this]() {
			return m_pendingWork == 0;
		});
	}

	// No need for a mutex, since all threaded work has been completed
	if(!m_shadersPendingForFinalization.empty())
	{
		auto shadersPendingForFinalization = std::move(m_shadersPendingForFinalization);
		m_shadersPendingForFinalization.clear();
		for(auto idx : shadersPendingForFinalization)
		{
			auto *shader = m_context.GetShaderManager().GetShader(idx);
			if(!shader)
				continue;
			shader->FinalizeInitialization();
		}
	}
}
void ShaderPipelineLoader::AddPendingShaderJobCount(ShaderIndex shaderIndex,int32_t i)
{
	m_pendingShaderJobsMutex.lock();
		auto it = m_pendingShaderJobs.find(shaderIndex);
		if(it == m_pendingShaderJobs.end())
			it = m_pendingShaderJobs.insert(std::make_pair(shaderIndex,0)).first;
		it->second += i;
		if(it->second == 0)
			m_pendingShaderJobs.erase(it);
	m_pendingShaderJobsMutex.unlock();
}
void ShaderPipelineLoader::Init(ShaderIndex shaderIndex,const std::function<bool()> &job)
{
	AddPendingShaderJobCount(shaderIndex,1);
	m_pendingWorkMutex.lock();
		++m_pendingWork;
	m_pendingWorkMutex.unlock();

	m_initQueueMutex.lock();
		m_initQueue.push(InitItem{shaderIndex,job});
	m_initQueueMutex.unlock();
	m_initPending.notify_one();

	if(!m_multiThreaded)
	{
		ProcessInitQueue();
		ProcessBakeQueue();
		Flush();
	}
}
void ShaderPipelineLoader::Bake(ShaderIndex shaderIndex,prosper::PipelineID id,prosper::PipelineBindPoint pipelineType)
{
	AddPendingShaderJobCount(shaderIndex,1);
	m_pendingWorkMutex.lock();
		++m_pendingWork;
	m_pendingWorkMutex.unlock();

	m_bakeQueueMutex.lock();
		m_bakeQueue.push(BakeItem{shaderIndex,id,pipelineType});
	m_bakeQueueMutex.unlock();
	m_bakePending.notify_one();

	if(!m_multiThreaded)
		ProcessBakeQueue();
}
#pragma optimize("",on)
