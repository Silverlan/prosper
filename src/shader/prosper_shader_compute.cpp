/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "vk_context.hpp"
#include "prosper_command_buffer.hpp"
#include "vk_command_buffer.hpp"
#include <wrappers/command_buffer.h>
#include <misc/descriptor_set_create_info.h>
#include <misc/compute_pipeline_create_info.h>
#include <wrappers/device.h>
#include <wrappers/compute_pipeline_manager.h>

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
	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(ShaderStage::Compute);
	auto firstPipelineId = std::numeric_limits<Anvil::PipelineID>::max();
	for(auto pipelineIdx=decltype(m_pipelineInfos.size()){0};pipelineIdx<m_pipelineInfos.size();++pipelineIdx)
	{
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		auto basePipelineId = std::numeric_limits<Anvil::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<Anvil::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false)
			m_basePipeline.lock()->GetPipelineId(basePipelineId);

		prosper::PipelineCreateFlags createFlags = prosper::PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<Anvil::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | prosper::PipelineCreateFlags::DerivativeBit;
		auto computePipelineInfo = prosper::ComputePipelineCreateInfo::Create(
			createFlags,
			(modCmp != nullptr) ? *modCmp->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			bIsDerivative ? &basePipelineId : nullptr
		);
		if(computePipelineInfo == nullptr)
			continue;
		m_currentPipelineIdx = pipelineIdx;
		InitializeComputePipeline(*computePipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*computePipelineInfo);
		m_currentPipelineIdx = std::numeric_limits<decltype(m_currentPipelineIdx)>::max();
		
		auto &pipelineInfo = m_pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = std::numeric_limits<decltype(pipelineInfo.id)>::max();
		auto &context = GetContext();
		auto result = context.AddPipeline(*computePipelineInfo,*modCmp,basePipelineId);
		pipelineInfo.createInfo = std::move(computePipelineInfo);
		if(result.has_value())
		{
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<Anvil::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			OnPipelineInitialized(pipelineIdx);
		}
	}
}

bool prosper::ShaderCompute::BeginCompute(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx)
{
	auto b = BindPipeline(*cmdBuffer,pipelineIdx);
	if(b == true)
		SetCurrentDrawCommandBuffer(cmdBuffer,pipelineIdx);
	return b;
}
void prosper::ShaderCompute::Compute() {}
void prosper::ShaderCompute::EndCompute() {UnbindPipeline(); SetCurrentDrawCommandBuffer(nullptr);}
bool prosper::ShaderCompute::RecordDispatch(uint32_t x,uint32_t y,uint32_t z)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && (dynamic_cast<VlkPrimaryCommandBuffer&>(*cmdBuffer))->record_dispatch(x,y,z);
}
