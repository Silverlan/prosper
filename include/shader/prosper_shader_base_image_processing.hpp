/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_BASE_IMAGE_PROCESSING_HPP__
#define __PROSPER_SHADER_BASE_IMAGE_PROCESSING_HPP__

#include "shader/prosper_shader.hpp"
#include <mathutil/uvec.h>

namespace prosper
{
	class RenderTarget;
	class DLLPROSPER ShaderBaseImageProcessing
		: public ShaderGraphics
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_UV;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		ShaderBaseImageProcessing(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader);
		ShaderBaseImageProcessing(prosper::IPrContext &context,const std::string &identifier,const std::string &fsShader);
		bool Draw(prosper::IDescriptorSet &descSetTexture);
		virtual bool Draw() override;
	protected:
		void AddDefaultVertexAttributes(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual uint32_t GetTextureDescriptorSetIndex() const;
	};
};

#endif
