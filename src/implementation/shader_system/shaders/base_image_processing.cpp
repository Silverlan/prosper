// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.shaders.base_image_processing;

using namespace prosper;

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX) ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX = {VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareVertexFormat()};

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_UV) ShaderBaseImageProcessing::VERTEX_BINDING_UV = {VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV, CommonBufferCache::GetSquareUvFormat()};

decltype(shaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE) shaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE = {"TEXTURE", {DescriptorSetInfo::Binding {"TEXTURE", DescriptorType::CombinedImageSampler, ShaderStageFlags::FragmentBit}}};

DescriptorSetInfo shaderBaseImageProcessing::get_descriptor_set_texture() { return DESCRIPTOR_SET_TEXTURE; }

ShaderBaseImageProcessing::ShaderBaseImageProcessing(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader) : ShaderGraphics(context, identifier, vsShader, fsShader) {}

ShaderBaseImageProcessing::ShaderBaseImageProcessing(IPrContext &context, const std::string &identifier, const std::string &fsShader) : ShaderBaseImageProcessing(context, identifier, "programs/image/noop_uv", fsShader) {}

void ShaderBaseImageProcessing::AddDefaultVertexAttributes()
{
	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(VERTEX_ATTRIBUTE_UV);
}

void ShaderBaseImageProcessing::InitializeShaderResources()
{
	AddDescriptorSetGroup(shaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE);
	AddDefaultVertexAttributes();
}

uint32_t ShaderBaseImageProcessing::GetTextureDescriptorSetIndex() const { return shaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE.setIndex; }

bool ShaderBaseImageProcessing::RecordDraw(ShaderBindState &bindState) const
{
	return RecordBindRenderBuffer(bindState, *GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) && ShaderGraphics::RecordDraw(bindState, GetContext().GetCommonBufferCache().GetSquareVertexCount());
}

bool ShaderBaseImageProcessing::RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture) const { return RecordBindDescriptorSet(bindState, descSetTexture) && RecordDraw(bindState); }
