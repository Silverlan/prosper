// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_SHADER_T_HPP__
#define __PROSPER_SHADER_T_HPP__

#include "prosper_definitions.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"

#pragma warning(push)
#pragma warning(disable : 4251)
template<class TShader>
void prosper::Shader::SetBaseShader()
{
	auto &shaderManager = GetContext().GetShaderManager();
	auto *pShader = shaderManager.template FindShader<TShader>();
	if(pShader == nullptr)
		throw std::logic_error("Cannot derive from base shader which hasn't been registered!");
	SetBaseShader(*pShader);
}

template<class TShader>
const std::shared_ptr<prosper::IRenderPass> &prosper::ShaderGraphics::GetRenderPass(prosper::IPrContext &context, uint32_t pipelineIdx)
{
	return GetRenderPass(context, typeid(TShader).hash_code(), pipelineIdx);
}

template<class TShader>
void prosper::ShaderGraphics::CreateCachedRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo, std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	return CreateCachedRenderPass(typeid(TShader).hash_code(), renderPassInfo, outRenderPass, pipelineIdx, "shader_" + std::string(typeid(TShader).name()) + std::string("_rp"));
}

template<class T>
bool prosper::Shader::RecordPushConstants(ShaderBindState &bindState, const T &data, uint32_t offset) const
{
	return RecordPushConstants(bindState, sizeof(data), &data, offset);
}
#pragma warning(pop)

#endif
