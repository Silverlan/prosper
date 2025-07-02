// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "shader/prosper_shader_rect.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_shader_t.hpp"

using namespace prosper;

ShaderRect::ShaderRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader) : ShaderBaseImageProcessing(context, identifier, vsShader, fsShader) {}
ShaderRect::ShaderRect(prosper::IPrContext &context, const std::string &identifier) : ShaderRect(context, identifier, "programs/image/noop_uv_cheap", "programs/image/noop") {}
void ShaderRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }
bool ShaderRect::RecordDraw(ShaderBindState &bindState, const Mat4 &modelMatrix) const { return RecordPushConstants(bindState, modelMatrix) && ShaderBaseImageProcessing::RecordDraw(bindState); }
void ShaderRect::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();
	AddDefaultVertexAttributes();
	AttachPushConstantRange(0u, sizeof(Mat4), prosper::ShaderStageFlags::VertexBit);
}
