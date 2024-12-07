/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2024 Silverlan
 */

#include "shader/prosper_shader_crash.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <shader/prosper_shader_t.hpp>
#include <prosper_util.hpp>
#include <image/prosper_render_target.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_texture.hpp>
#include <random>

using namespace prosper;

decltype(ShaderCrash::VERTEX_BINDING_VERTEX) ShaderCrash::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderCrash::VERTEX_ATTRIBUTE_POSITION) ShaderCrash::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareVertexFormat()};

ShaderCrash::ShaderCrash(prosper::IPrContext &context, const std::string &identifier) : prosper::ShaderGraphics(context, identifier, "programs/image/noop_uv", "programs/util/crash") {}

ShaderCrash::~ShaderCrash() {}

void ShaderCrash::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void ShaderCrash::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}

bool ShaderCrash::RecordDraw(prosper::ICommandBuffer &cmd) const
{
	auto &context = GetContext();
	auto vertBuf = GetContext().GetCommonBufferCache().GetSquareVertexBuffer();
	auto vertexBuffer = context.CreateRenderBuffer(*static_cast<const prosper::GraphicsPipelineCreateInfo *>(GetPipelineCreateInfo(0u)), {vertBuf.get()});
	if(!vertexBuffer)
		return false;
	prosper::ShaderBindState bindState {cmd};
	if(RecordBeginDraw(bindState) == false)
		return false;

	PushConstants pushConstants {};
	pushConstants.offset = -0.1f;
	auto res = RecordPushConstants(bindState, pushConstants) && prosper::ShaderGraphics::RecordDraw(bindState);
	res = res && RecordBindRenderBuffer(bindState, *vertexBuffer);
	res = res && ShaderGraphics::RecordDraw(bindState, GetContext().GetCommonBufferCache().GetSquareVertexCount());
	RecordEndDraw(bindState);
	return res;
}
