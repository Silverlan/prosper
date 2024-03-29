/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shader/prosper_pipeline_manager.hpp"

using namespace prosper;

// TODO: Remove this file/class
IPipelineManager::IPipelineManager(IPrContext &context) : ContextObject {context} {}

#if 0
bool VlkPipelineManager::ClearPipeline(PipelineID id)
{
	return static_cast<prosper::VlkContext&>(GetContext()).GetDevice().get_compute_pipeline_manager()->delete_pipeline(id);
}
PipelineLayout *VlkPipelineManager::GetPipelineLayout(PipelineID id)
{
	return static_cast<prosper::VlkContext&>(GetContext()).GetDevice().get_compute_pipeline_manager()->get_pipeline_layout(id);
}
BasePipelineCreateInfo *VlkPipelineManager::GetPipelineInfo(PipelineID id)
{
	return static_cast<prosper::VlkContext&>(GetContext()).GetDevice().get_compute_pipeline_manager()->get_pipeline_create_info(id);
}
#endif
