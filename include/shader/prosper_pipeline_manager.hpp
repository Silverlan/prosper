/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PIPELINE_MANAGER_HPP__
#define __PROSPER_PIPELINE_MANAGER_HPP__

#include "prosper_structs.hpp"
#include "prosper_context_object.hpp"

namespace prosper
{
	struct PipelineLayout;
	class BasePipelineCreateInfo;
	class DLLPROSPER IPipelineManager
		: public ContextObject
	{
	public:
		IPipelineManager(IPrContext &context);
		IPipelineManager(const IPipelineManager&)=delete;
		IPipelineManager &operator=(const IPipelineManager&)=delete;
		virtual ~IPipelineManager() override=default;

		virtual bool ClearPipeline(PipelineID id)=0;
		virtual PipelineLayout *GetPipelineLayout(PipelineID id)=0;
		virtual BasePipelineCreateInfo *GetPipelineInfo(PipelineID id)=0;
	};

	class DLLPROSPER VlkPipelineManager
		: public IPipelineManager
	{
	public:
		virtual bool ClearPipeline(PipelineID id) override;
		virtual PipelineLayout *GetPipelineLayout(PipelineID id) override;
		virtual BasePipelineCreateInfo *GetPipelineInfo(PipelineID id) override;
	};
};

#endif
