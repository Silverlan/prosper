/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_pipeline_loader.hpp"
#include "prosper_context.hpp"
#include "prosper_command_buffer.hpp"
#include <cassert>

prosper::ShaderRaytracing::ShaderRaytracing(prosper::IPrContext &context, const std::string &identifier, const std::string &csShader) : Shader(context, identifier, csShader) {}

void prosper::ShaderRaytracing::InitializeRaytracingPipeline(prosper::RayTracingPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) {}
void prosper::ShaderRaytracing::InitializePipeline()
{
	OnInitializePipelines();

	// Reset pipeline infos (in case the shader is being reloaded)
	auto &pipelineInfos = GetPipelineInfos();
	for(auto &pipelineInfo : pipelineInfos)
		pipelineInfo = {};

	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(ShaderStage::Compute);
	auto firstPipelineId = std::numeric_limits<prosper::PipelineID>::max();
	for(auto pipelineIdx = decltype(pipelineInfos.size()) {0}; pipelineIdx < pipelineInfos.size(); ++pipelineIdx) {
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		auto basePipelineId = std::numeric_limits<prosper::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<prosper::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false) {
			assert(!GetContext().GetPipelineLoader().IsShaderQueued(m_basePipeline.lock()->GetIndex()));
			// Base shader pipeline must have already been loaded (but not necessarily baked) at this point!
			m_basePipeline.lock()->GetPipelineId(basePipelineId);
		}

		prosper::PipelineCreateFlags createFlags = prosper::PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<prosper::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | prosper::PipelineCreateFlags::DerivativeBit;
		auto rtPipelineInfo = prosper::RayTracingPipelineCreateInfo::Create(createFlags, (modCmp != nullptr) ? *modCmp->entryPoint : prosper::ShaderModuleStageEntryPoint(), bIsDerivative ? &basePipelineId : nullptr);
		if(rtPipelineInfo == nullptr)
			continue;
		InitializeRaytracingPipeline(*rtPipelineInfo, pipelineIdx);
		InitializeDescriptorSetGroup(*rtPipelineInfo, pipelineIdx);

		auto &pipelineInitInfo = pipelineInfos.at(pipelineIdx);
		for(auto &range : pipelineInitInfo.pushConstantRanges)
			rtPipelineInfo->AttachPushConstantRange(range.offset, range.size, range.stages);

		auto &pipelineInfo = pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = InitPipelineId(pipelineIdx);
		auto &context = GetContext();
		auto result = context.AddPipeline(*this, pipelineIdx, *rtPipelineInfo, *modCmp, basePipelineId);
		pipelineInfo.createInfo = std::move(rtPipelineInfo);
		if(result.has_value()) {
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<prosper::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			OnPipelineInitialized(pipelineIdx);

			GetContext().GetPipelineLoader().Bake(GetIndex(), pipelineInfo.id, GetPipelineBindPoint());
		}
		else
			pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}
