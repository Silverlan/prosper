// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.shaders.flip_image;

using namespace prosper;

ShaderFlipImage::ShaderFlipImage(IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing(context, identifier, "programs/util/flip") {}

ShaderFlipImage::~ShaderFlipImage() {}

void ShaderFlipImage::InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderBaseImageProcessing::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void ShaderFlipImage::InitializeShaderResources()
{
	ShaderBaseImageProcessing::InitializeShaderResources();

	AttachPushConstantRange(0u, sizeof(PushConstants), ShaderStageFlags::FragmentBit);
}

bool ShaderFlipImage::RecordDraw(ICommandBuffer &cmd, IDescriptorSet &descSetTexture, bool flipHorizontally, bool flipVertically) const
{
	ShaderBindState bindState {cmd};
	if(RecordBeginDraw(bindState) == false)
		return false;

	PushConstants pushConstants {};
	pushConstants.flipHorizontally = static_cast<uint32_t>(flipHorizontally);
	pushConstants.flipVertically = static_cast<uint32_t>(flipVertically);
	auto res = RecordDraw(bindState, descSetTexture, pushConstants);
	RecordEndDraw(bindState);
	return res;
}

bool ShaderFlipImage::RecordDraw(ShaderBindState &bindState, bool flipHorizontally, bool flipVertically) const
{
	PushConstants pushConstants {};
	pushConstants.flipHorizontally = static_cast<uint32_t>(flipHorizontally);
	pushConstants.flipVertically = static_cast<uint32_t>(flipVertically);
	return RecordPushConstants(bindState, pushConstants) && ShaderBaseImageProcessing::RecordDraw(bindState);
}

bool ShaderFlipImage::RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const { return RecordPushConstants(bindState, pushConstants) && ShaderBaseImageProcessing::RecordDraw(bindState, descSetTexture); }
