/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PIPELINE_LOADER_HPP__
#define __PROSPER_PIPELINE_LOADER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include <mutex>
#include <queue>
#include <functional>
#include <unordered_set>
#include <condition_variable>

#undef max

namespace prosper
{
	class IPrContext;
	class DLLPROSPER ShaderPipelineLoader
	{
	public:
		ShaderPipelineLoader(prosper::IPrContext &context);
		~ShaderPipelineLoader();
		void Flush();
		void Stop();
		void Init(ShaderIndex shaderIndex,const std::function<bool()> &job);
		void Bake(ShaderIndex shaderIndex,prosper::PipelineID id,prosper::PipelineBindPoint pipelineType);
		bool IsShaderQueued(ShaderIndex shaderIndex) const;
	private:
		struct InitItem
		{
			ShaderIndex shaderIndex;
			std::function<bool()> init;
		};
		struct BakeItem
		{
			ShaderIndex shaderIndex;
			prosper::PipelineID pipelineId;
			prosper::PipelineBindPoint pipelineBindPoint;
		};
		void ProcessInitQueue();
		void ProcessBakeQueue();
		prosper::IPrContext &m_context;
		void AddPendingShaderJobCount(ShaderIndex shaderIndex,int32_t i);

		bool m_multiThreaded = true;
		std::mutex m_initQueueMutex;
		std::queue<InitItem> m_initQueue;
		std::thread m_initThread;
		std::condition_variable m_initPending;

		std::thread m_bakeThread;
		std::mutex m_bakeQueueMutex;
		std::queue<BakeItem> m_bakeQueue;
		std::condition_variable m_bakePending;
		std::atomic<bool> m_running = true;

		std::atomic<uint32_t> m_pendingWork = 0;
		std::condition_variable m_pendingWorkCondition;
		std::mutex m_pendingWorkMutex;

		mutable std::mutex m_pendingShaderJobsMutex;
		std::unordered_map<ShaderIndex,uint32_t> m_pendingShaderJobs;

		std::unordered_set<ShaderIndex> m_shadersPendingForFinalization;
	};
};

#endif
