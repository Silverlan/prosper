/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "prosper_command_buffer.hpp"

prosper::ShaderRaytracing::ShaderRaytracing(prosper::IPrContext &context,const std::string &identifier,const std::string &csShader)
	: Shader(context,identifier,csShader)
{}

void prosper::ShaderRaytracing::InitializeRaytracingPipeline(prosper::RayTracingPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) {}
void prosper::ShaderRaytracing::InitializePipeline()
{
	// Reset pipeline infos (in case the shader is being reloaded)
	for(auto &pipelineInfo : m_pipelineInfos)
		pipelineInfo = {};

	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(ShaderStage::Compute);
	auto firstPipelineId = std::numeric_limits<prosper::PipelineID>::max();
	for(auto pipelineIdx=decltype(m_pipelineInfos.size()){0};pipelineIdx<m_pipelineInfos.size();++pipelineIdx)
	{
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		auto basePipelineId = std::numeric_limits<prosper::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<prosper::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false)
			m_basePipeline.lock()->GetPipelineId(basePipelineId);

		prosper::PipelineCreateFlags createFlags = prosper::PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<prosper::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | prosper::PipelineCreateFlags::DerivativeBit;
		auto rtPipelineInfo = prosper::RayTracingPipelineCreateInfo::Create(
			createFlags,
			(modCmp != nullptr) ? *modCmp->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			bIsDerivative ? &basePipelineId : nullptr
		);
		if(rtPipelineInfo == nullptr)
			continue;
		m_currentPipelineIdx = pipelineIdx;
		InitializeRaytracingPipeline(*rtPipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*rtPipelineInfo);

		auto &pipelineInitInfo = m_pipelineInfos.at(pipelineIdx);
		for(auto &range : pipelineInitInfo.pushConstantRanges)
			rtPipelineInfo->AttachPushConstantRange(range.offset,range.size,range.stages);

		m_currentPipelineIdx = std::numeric_limits<decltype(m_currentPipelineIdx)>::max();
		
		auto &pipelineInfo = m_pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = InitPipelineId(pipelineIdx);
		auto &context = GetContext();
		auto result = context.AddPipeline(*this,pipelineIdx,*rtPipelineInfo,*modCmp,basePipelineId);
		pipelineInfo.createInfo = std::move(rtPipelineInfo);
		if(result.has_value())
		{
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<prosper::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			OnPipelineInitialized(pipelineIdx);
		}
		else
			pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}
