// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.shaders.copy_image;

using namespace prosper;

ShaderCopyImage::ShaderCopyImage(IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing(context, identifier, "programs/image/noop") { SetPipelineCount(pragma::math::to_integral(Pipeline::Count)); }
bool ShaderCopyImage::RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx) const { return ShaderGraphics::RecordBeginDraw(bindState, pragma::math::to_integral(pipelineIdx)); }
void ShaderCopyImage::InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	if(pipelineIdx == 0u) {
		ShaderBaseImageProcessing::InitializeRenderPass(outRenderPass, pipelineIdx);
		return;
	}
	CreateCachedRenderPass<ShaderCopyImage>({{}}, outRenderPass, pipelineIdx);
}
