/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader_copy_image.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_util_square_shape.hpp"

using namespace prosper;

ShaderCopyImage::ShaderCopyImage(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing(context,identifier,"screen/fs_screen")
{
	SetPipelineCount(umath::to_integral(Pipeline::Count));
}
bool ShaderCopyImage::BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx) {return ShaderGraphics::BeginDraw(cmdBuffer,umath::to_integral(pipelineIdx));}
void ShaderCopyImage::InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	if(pipelineIdx == 0u)
	{
		ShaderBaseImageProcessing::InitializeRenderPass(outRenderPass,pipelineIdx);
		return;
	}
	CreateCachedRenderPass<ShaderCopyImage>({{}},outRenderPass,pipelineIdx);
}
