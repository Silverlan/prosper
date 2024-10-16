/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shader/prosper_shader_base_image_processing.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "buffers/prosper_buffer.hpp"

using namespace prosper;

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX) ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareVertexFormat()};

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_UV) ShaderBaseImageProcessing::VERTEX_BINDING_UV = {prosper::VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV, CommonBufferCache::GetSquareUvFormat()};

decltype(ShaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE) ShaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE = {"TEXTURE", {prosper::DescriptorSetInfo::Binding {"TEXTURE", DescriptorType::CombinedImageSampler, ShaderStageFlags::FragmentBit}}};
ShaderBaseImageProcessing::ShaderBaseImageProcessing(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader) : ShaderGraphics(context, identifier, vsShader, fsShader) {}

ShaderBaseImageProcessing::ShaderBaseImageProcessing(prosper::IPrContext &context, const std::string &identifier, const std::string &fsShader) : ShaderBaseImageProcessing(context, identifier, "programs/image/noop_uv", fsShader) {}

void ShaderBaseImageProcessing::AddDefaultVertexAttributes()
{
	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(VERTEX_ATTRIBUTE_UV);
}

void ShaderBaseImageProcessing::InitializeShaderResources()
{
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	AddDefaultVertexAttributes();
}

uint32_t ShaderBaseImageProcessing::GetTextureDescriptorSetIndex() const { return DESCRIPTOR_SET_TEXTURE.setIndex; }

bool ShaderBaseImageProcessing::RecordDraw(ShaderBindState &bindState) const
{
	return RecordBindRenderBuffer(bindState, *GetContext().GetCommonBufferCache().GetSquareVertexUvRenderBuffer()) && ShaderGraphics::RecordDraw(bindState, GetContext().GetCommonBufferCache().GetSquareVertexCount());
}

bool ShaderBaseImageProcessing::RecordDraw(ShaderBindState &bindState, prosper::IDescriptorSet &descSetTexture) const { return RecordBindDescriptorSet(bindState, descSetTexture) && RecordDraw(bindState); }
