/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shader/prosper_shader_rect.hpp"
#include "shader/prosper_pipeline_create_info.hpp"

using namespace prosper;

ShaderRect::ShaderRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader)
	: ShaderBaseImageProcessing(context,identifier,vsShader,fsShader)
{}
ShaderRect::ShaderRect(prosper::IPrContext &context,const std::string &identifier)
	: ShaderRect(context,identifier,"screen/vs_screen_uv_cheap","screen/fs_screen")
{}
void ShaderRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddDefaultVertexAttributes(pipelineInfo);
	AttachPushConstantRange(pipelineInfo,pipelineIdx,0u,sizeof(Mat4),prosper::ShaderStageFlags::VertexBit);
}
bool ShaderRect::RecordDraw(ShaderBindState &bindState,const Mat4 &modelMatrix) const
{
	return RecordPushConstants(bindState,modelMatrix) && ShaderBaseImageProcessing::RecordDraw(bindState);
}
