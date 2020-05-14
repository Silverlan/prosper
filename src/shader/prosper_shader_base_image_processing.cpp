/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader_base_image_processing.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_util_square_shape.hpp"
#include <wrappers/graphics_pipeline_manager.h>

using namespace prosper;

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX) ShaderBaseImageProcessing::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};

decltype(ShaderBaseImageProcessing::VERTEX_BINDING_UV) ShaderBaseImageProcessing::VERTEX_BINDING_UV = {prosper::VertexInputRate::Vertex};
decltype(ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV) ShaderBaseImageProcessing::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_UV,prosper::util::get_square_uv_format()};

decltype(ShaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE) ShaderBaseImageProcessing::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding {
			DescriptorType::CombinedImageSampler,
			ShaderStageFlags::FragmentBit
		}
	}
};
ShaderBaseImageProcessing::ShaderBaseImageProcessing(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader)
	: ShaderGraphics(context,identifier,vsShader,fsShader)
{}

ShaderBaseImageProcessing::ShaderBaseImageProcessing(prosper::IPrContext &context,const std::string &identifier,const std::string &fsShader)
	: ShaderBaseImageProcessing(context,identifier,"screen/vs_screen_uv",fsShader)
{}

void ShaderBaseImageProcessing::AddDefaultVertexAttributes(prosper::GraphicsPipelineCreateInfo &pipelineInfo)
{
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
}

void ShaderBaseImageProcessing::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

uint32_t ShaderBaseImageProcessing::GetTextureDescriptorSetIndex() const {return DESCRIPTOR_SET_TEXTURE.setIndex;}

bool ShaderBaseImageProcessing::Draw()
{
	auto vertBuffer = prosper::util::get_square_vertex_buffer(GetContext());
	auto uvBuffer = prosper::util::get_square_uv_buffer(GetContext());
	if(
		RecordBindVertexBuffers({vertBuffer.get(),uvBuffer.get()}) == false ||
		RecordDraw(prosper::util::get_square_vertex_count()) == false
	)
		return false;
	return true;
}

bool ShaderBaseImageProcessing::Draw(prosper::IDescriptorSet &descSetTexture) {return RecordBindDescriptorSet(descSetTexture) && Draw();}
