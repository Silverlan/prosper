// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :shader_system.pipeline_create_info;
import :shader_system.pipeline_loader;
import :shader_system.shader;

prosper::ShaderRaytracing::ShaderRaytracing(IPrContext &context, const std::string &identifier, const std::string &csShader) : Shader(context, identifier, csShader) {}

void prosper::ShaderRaytracing::InitializeRaytracingPipeline(RayTracingPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) {}
void prosper::ShaderRaytracing::InitializePipeline()
{
	OnInitializePipelines();

	// Reset pipeline infos (in case the shader is being reloaded)
	auto &pipelineInfos = GetPipelineInfos();
	for(auto &pipelineInfo : pipelineInfos)
		pipelineInfo = {};

	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(ShaderStage::Compute);
	auto firstPipelineId = std::numeric_limits<PipelineID>::max();
	for(auto pipelineIdx = decltype(pipelineInfos.size()) {0}; pipelineIdx < pipelineInfos.size(); ++pipelineIdx) {
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		auto basePipelineId = std::numeric_limits<PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false) {
			assert(!GetContext().GetPipelineLoader().IsShaderQueued(m_basePipeline.lock()->GetIndex()));
			// Base shader pipeline must have already been loaded (but not necessarily baked) at this point!
			m_basePipeline.lock()->GetPipelineId(basePipelineId);
		}

		PipelineCreateFlags createFlags = PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | PipelineCreateFlags::DerivativeBit;
		auto rtPipelineInfo = RayTracingPipelineCreateInfo::Create(createFlags, (modCmp != nullptr) ? *modCmp->entryPoint : ShaderModuleStageEntryPoint(), bIsDerivative ? &basePipelineId : nullptr);
		if(rtPipelineInfo == nullptr)
			continue;
		InitializeRaytracingPipeline(*rtPipelineInfo, pipelineIdx);
		InitializeDescriptorSetGroups(*rtPipelineInfo);

		for(auto &range : m_shaderResources.pushConstantRanges)
			rtPipelineInfo->AttachPushConstantRange(range.offset, range.size, range.stages);

		auto &pipelineInfo = pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = InitPipelineId(pipelineIdx);
		auto &context = GetContext();
		auto result = context.AddPipeline(*this, pipelineIdx, *rtPipelineInfo, *modCmp, basePipelineId);
		pipelineInfo.createInfo = std::move(rtPipelineInfo);
		if(result.has_value()) {
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			OnPipelineInitialized(pipelineIdx);

			GetContext().GetPipelineLoader().Bake(GetIndex(), pipelineInfo.id, GetPipelineBindPoint());
		}
		else
			pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}
