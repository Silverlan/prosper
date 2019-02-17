/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader_rect.hpp"
#include "prosper_util_square_shape.hpp"

using namespace prosper;

ShaderRect::ShaderRect(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader)
	: ShaderBaseImageProcessing(context,identifier,vsShader,fsShader)
{}
ShaderRect::ShaderRect(prosper::Context &context,const std::string &identifier)
	: ShaderRect(context,identifier,"screen/vs_screen_uv_cheap","screen/fs_screen")
{}
void ShaderRect::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddDefaultVertexAttributes(pipelineInfo);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(Mat4),Anvil::ShaderStageFlagBits::VERTEX_BIT);
}
bool ShaderRect::Draw(const Mat4 &modelMatrix)
{
	return RecordPushConstants(modelMatrix) && ShaderBaseImageProcessing::Draw();
}
