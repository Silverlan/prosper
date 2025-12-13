// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.pipeline_loader;

export import :types;

#undef max

export {
	namespace prosper {
		class IPrContext;
		class DLLPROSPER ShaderPipelineLoader {
		  public:
			ShaderPipelineLoader(IPrContext &context);
			~ShaderPipelineLoader();
			void Flush();
			void Stop();
			void Init(ShaderIndex shaderIndex, const std::function<bool()> &job);
			void Bake(ShaderIndex shaderIndex, PipelineID id, PipelineBindPoint pipelineType);
			bool IsShaderQueued(ShaderIndex shaderIndex) const;
		  private:
			struct InitItem {
				ShaderIndex shaderIndex;
				std::function<bool()> init;
			};
			struct BakeItem {
				ShaderIndex shaderIndex;
				PipelineID pipelineId;
				PipelineBindPoint pipelineBindPoint;
			};
			void ProcessInitQueue();
			void ProcessBakeQueue();
			IPrContext &m_context;
			void AddPendingShaderJobCount(ShaderIndex shaderIndex, int32_t i);

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
			std::unordered_map<ShaderIndex, uint32_t> m_pendingShaderJobs;

			std::unordered_set<ShaderIndex> m_shadersPendingForFinalization;
		};
	};
}
