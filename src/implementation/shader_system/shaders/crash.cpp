// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.pipeline_create_info;
import :shader_system.shaders.crash;

using namespace prosper;

decltype(ShaderCrash::VERTEX_BINDING_VERTEX) ShaderCrash::VERTEX_BINDING_VERTEX = {VertexInputRate::Vertex};
decltype(ShaderCrash::VERTEX_ATTRIBUTE_POSITION) ShaderCrash::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareVertexFormat()};

ShaderCrash::ShaderCrash(IPrContext &context, const std::string &identifier) : ShaderGraphics(context, identifier, "programs/image/noop_uv", "programs/util/crash") {}

ShaderCrash::~ShaderCrash() {}

void ShaderCrash::InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void ShaderCrash::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(PushConstants), ShaderStageFlags::FragmentBit);
}

bool ShaderCrash::RecordDraw(ICommandBuffer &cmd) const
{
	auto &context = GetContext();
	auto vertBuf = GetContext().GetCommonBufferCache().GetSquareVertexBuffer();
	auto vertexBuffer = context.CreateRenderBuffer(*static_cast<const GraphicsPipelineCreateInfo *>(GetPipelineCreateInfo(0u)), {vertBuf.get()});
	if(!vertexBuffer)
		return false;
	ShaderBindState bindState {cmd};
	if(RecordBeginDraw(bindState) == false)
		return false;

	PushConstants pushConstants {};
	pushConstants.offset = -0.1f;
	auto res = RecordPushConstants(bindState, pushConstants) && ShaderGraphics::RecordDraw(bindState);
	res = res && RecordBindRenderBuffer(bindState, *vertexBuffer);
	res = res && ShaderGraphics::RecordDraw(bindState, GetContext().GetCommonBufferCache().GetSquareVertexCount());
	RecordEndDraw(bindState);
	return res;
}
