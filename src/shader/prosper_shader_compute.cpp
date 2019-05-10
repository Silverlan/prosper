/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "prosper_context.hpp"
#include "prosper_command_buffer.hpp"
#include <wrappers/command_buffer.h>
#include <misc/descriptor_set_create_info.h>
#include <misc/compute_pipeline_create_info.h>
#include <wrappers/compute_pipeline_manager.h>

prosper::ShaderCompute::ShaderCompute(prosper::Context &context,const std::string &identifier,const std::string &csShader)
	: Shader(context,identifier,csShader)
{}

Anvil::BasePipelineManager *prosper::ShaderCompute::GetPipelineManager() const
{
	auto &context = const_cast<const ShaderCompute*>(this)->GetContext();
	auto &dev = context.GetDevice();
	return dev.get_compute_pipeline_manager();
}
bool prosper::ShaderCompute::AddSpecializationConstant(Anvil::ComputePipelineCreateInfo &pipelineInfo,uint32_t constantId,uint32_t numBytes,const void *data)
{
	return pipelineInfo.add_specialization_constant(constantId,numBytes,data);
}
void prosper::ShaderCompute::InitializeComputePipeline(Anvil::ComputePipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) {}
void prosper::ShaderCompute::InitializePipeline()
{
	auto &context = GetContext();
	auto &dev = context.GetDevice();
	auto swapchain = context.GetSwapchain();
	auto *computePipelineManager = dev.get_compute_pipeline_manager();

	/* Configure the graphics pipeline */
	auto *modCmp = GetStage(Anvil::ShaderStage::COMPUTE);
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

		Anvil::PipelineCreateFlags createFlags = Anvil::PipelineCreateFlagBits::ALLOW_DERIVATIVES_BIT;
		auto bIsDerivative = basePipelineId != std::numeric_limits<Anvil::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | Anvil::PipelineCreateFlagBits::DERIVATIVE_BIT;
		auto computePipelineInfo = Anvil::ComputePipelineCreateInfo::create(
			createFlags,
			(modCmp != nullptr) ? *modCmp->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			bIsDerivative ? &basePipelineId : nullptr
		);
		if(computePipelineInfo == nullptr)
			continue;
		InitializeComputePipeline(*computePipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*computePipelineInfo);

		auto &pipelineInfo = m_pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = std::numeric_limits<decltype(pipelineInfo.id)>::max();
		auto r = computePipelineManager->add_pipeline(std::move(computePipelineInfo),&pipelineInfo.id);
		if(r == true)
		{
			if(firstPipelineId == std::numeric_limits<Anvil::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			computePipelineManager->bake();
			OnPipelineInitialized(pipelineIdx);
		}
	}
}

bool prosper::ShaderCompute::BeginCompute(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx)
{
	auto b = BindPipeline(cmdBuffer->GetAnvilCommandBuffer(),pipelineIdx);
	if(b == true)
		SetCurrentDrawCommandBuffer(cmdBuffer,pipelineIdx);
	return b;
}
void prosper::ShaderCompute::Compute() {}
void prosper::ShaderCompute::EndCompute() {UnbindPipeline(); SetCurrentDrawCommandBuffer(nullptr);}
bool prosper::ShaderCompute::RecordDispatch(uint32_t x,uint32_t y,uint32_t z)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && (*cmdBuffer)->record_dispatch(x,y,z);
}
