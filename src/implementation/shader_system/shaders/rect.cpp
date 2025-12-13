// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.shaders.rect;

using namespace prosper;

ShaderRect::ShaderRect(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader) : ShaderBaseImageProcessing(context, identifier, vsShader, fsShader) {}
ShaderRect::ShaderRect(IPrContext &context, const std::string &identifier) : ShaderRect(context, identifier, "programs/image/noop_uv_cheap", "programs/image/noop") {}
void ShaderRect::InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }
bool ShaderRect::RecordDraw(ShaderBindState &bindState, const Mat4 &modelMatrix) const { return RecordPushConstants(bindState, modelMatrix) && ShaderBaseImageProcessing::RecordDraw(bindState); }
void ShaderRect::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();
	AddDefaultVertexAttributes();
	AttachPushConstantRange(0u, sizeof(Mat4), ShaderStageFlags::VertexBit);
}
