/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_pipeline_loader.hpp"
#include "prosper_command_buffer.hpp"

prosper::ShaderCompute::ShaderCompute(prosper::IPrContext &context,const std::string &identifier,const std::string &csShader)
	: Shader(context,identifier,csShader)
{}

bool prosper::ShaderCompute::AddSpecializationConstant(prosper::ComputePipelineCreateInfo &pipelineInfo,uint32_t constantId,uint32_t numBytes,const void *data)
{
	return pipelineInfo.AddSpecializationConstant(constantId,numBytes,data);
}
void prosper::ShaderCompute::InitializeComputePipeline(prosper::ComputePipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) {}
void prosper::ShaderCompute::InitializePipeline()
{
	// Reset pipeline infos (in case the shader is being reloaded)
	auto &pipelineInfos = GetPipelineInfos();
	for(auto &pipelineInfo : pipelineInfos)
		pipelineInfo = {};

	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(ShaderStage::Compute);
	auto firstPipelineId = std::numeric_limits<prosper::PipelineID>::max();
	for(auto pipelineIdx=decltype(pipelineInfos.size()){0};pipelineIdx<pipelineInfos.size();++pipelineIdx)
	{
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		auto basePipelineId = std::numeric_limits<prosper::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<prosper::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false)
		{
			assert(!GetContext().GetPipelineLoader().IsShaderQueued(m_basePipeline.lock()->GetIndex()));
			// Base shader pipeline must have already been loaded (but not necessarily baked) at this point!
			m_basePipeline.lock()->GetPipelineId(basePipelineId);
		}

		prosper::PipelineCreateFlags createFlags = prosper::PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<prosper::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | prosper::PipelineCreateFlags::DerivativeBit;
		auto computePipelineInfo = prosper::ComputePipelineCreateInfo::Create(
			createFlags,
			(modCmp != nullptr) ? *modCmp->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			bIsDerivative ? &basePipelineId : nullptr
		);
		if(computePipelineInfo == nullptr)
			continue;
		InitializeComputePipeline(*computePipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*computePipelineInfo,pipelineIdx);

		auto &pipelineInitInfo = pipelineInfos.at(pipelineIdx);
		for(auto &range : pipelineInitInfo.pushConstantRanges)
			computePipelineInfo->AttachPushConstantRange(range.offset,range.size,range.stages);
		
		auto &pipelineInfo = pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = InitPipelineId(pipelineIdx);
		auto &context = GetContext();
		auto result = context.AddPipeline(*this,pipelineIdx,*computePipelineInfo,*modCmp,basePipelineId);
		pipelineInfo.createInfo = std::move(computePipelineInfo);
		if(result.has_value())
		{
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<prosper::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			OnPipelineInitialized(pipelineIdx);

			GetContext().GetPipelineLoader().Bake(GetIndex(),pipelineInfo.id,GetPipelineBindPoint());
		}
		else
			pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}

bool prosper::ShaderCompute::RecordBeginCompute(ShaderBindState &bindState,uint32_t pipelineIdx) const
{
	return RecordBindPipeline(bindState,pipelineIdx);
}
void prosper::ShaderCompute::RecordCompute(ShaderBindState &bindState) const {}
void prosper::ShaderCompute::RecordEndCompute(ShaderBindState &bindState) const
{
	// Reset bind state
	bindState.pipelineIdx = std::numeric_limits<uint32_t>::max();
}
bool prosper::ShaderCompute::RecordDispatch(ShaderBindState &bindState,uint32_t x,uint32_t y,uint32_t z) const
{
	return bindState.commandBuffer.RecordDispatch(x,y,z);
}
