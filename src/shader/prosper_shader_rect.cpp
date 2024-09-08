/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
