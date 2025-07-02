// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_PIPELINE_MANAGER_HPP__
#define __PROSPER_PIPELINE_MANAGER_HPP__

#include "prosper_structs.hpp"
#include "prosper_context_object.hpp"

namespace prosper {
	struct PipelineLayout;
	class BasePipelineCreateInfo;
	class DLLPROSPER IPipelineManager : public ContextObject {
	  public:
		IPipelineManager(IPrContext &context);
		IPipelineManager(const IPipelineManager &) = delete;
		IPipelineManager &operator=(const IPipelineManager &) = delete;
		virtual ~IPipelineManager() override = default;

		virtual bool ClearPipeline(PipelineID id) = 0;
		virtual PipelineLayout *GetPipelineLayout(PipelineID id) = 0;
		virtual BasePipelineCreateInfo *GetPipelineInfo(PipelineID id) = 0;
	};

	class DLLPROSPER VlkPipelineManager : public IPipelineManager {
	  public:
		virtual bool ClearPipeline(PipelineID id) override;
		virtual PipelineLayout *GetPipelineLayout(PipelineID id) override;
		virtual BasePipelineCreateInfo *GetPipelineInfo(PipelineID id) override;
	};
};

#endif
