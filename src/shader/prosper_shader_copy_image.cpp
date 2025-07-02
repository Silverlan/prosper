// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "shader/prosper_shader_copy_image.hpp"
#include "shader/prosper_shader_t.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "buffers/prosper_buffer.hpp"

using namespace prosper;

ShaderCopyImage::ShaderCopyImage(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing(context, identifier, "programs/image/noop") { SetPipelineCount(umath::to_integral(Pipeline::Count)); }
bool ShaderCopyImage::RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx) const { return ShaderGraphics::RecordBeginDraw(bindState, umath::to_integral(pipelineIdx)); }
void ShaderCopyImage::InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	if(pipelineIdx == 0u) {
		ShaderBaseImageProcessing::InitializeRenderPass(outRenderPass, pipelineIdx);
		return;
	}
	CreateCachedRenderPass<ShaderCopyImage>({{}}, outRenderPass, pipelineIdx);
}
