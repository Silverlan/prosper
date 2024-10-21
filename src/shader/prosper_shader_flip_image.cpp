/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2023 Silverlan
 */

#include "shader/prosper_shader_flip_image.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <shader/prosper_shader_copy_image.hpp>
#include <shader/prosper_shader_t.hpp>
#include <prosper_util.hpp>
#include <image/prosper_render_target.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_texture.hpp>
#include <random>

using namespace prosper;

ShaderFlipImage::ShaderFlipImage(prosper::IPrContext &context, const std::string &identifier) : prosper::ShaderBaseImageProcessing(context, identifier, "programs/util/flip") {}

ShaderFlipImage::~ShaderFlipImage() {}

void ShaderFlipImage::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderBaseImageProcessing::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void ShaderFlipImage::InitializeShaderResources()
{
	ShaderBaseImageProcessing::InitializeShaderResources();

	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}

bool ShaderFlipImage::RecordDraw(prosper::ICommandBuffer &cmd, prosper::IDescriptorSet &descSetTexture, bool flipHorizontally, bool flipVertically) const
{
	prosper::ShaderBindState bindState {cmd};
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

bool ShaderFlipImage::RecordDraw(prosper::ShaderBindState &bindState, prosper::IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const { return RecordPushConstants(bindState, pushConstants) && prosper::ShaderBaseImageProcessing::RecordDraw(bindState, descSetTexture); }
