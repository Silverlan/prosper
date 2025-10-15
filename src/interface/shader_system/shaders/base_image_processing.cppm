// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"
#include <string>

export module pragma.prosper:shader_system.shaders.base_image_processing;

export import :shader_system.shader;

export namespace prosper {
	class RenderTarget;
	class DLLPROSPER ShaderBaseImageProcessing : public ShaderGraphics {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_UV;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		ShaderBaseImageProcessing(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader);
		ShaderBaseImageProcessing(prosper::IPrContext &context, const std::string &identifier, const std::string &fsShader);
		bool RecordDraw(ShaderBindState &bindState, prosper::IDescriptorSet &descSetTexture) const;
		virtual bool RecordDraw(ShaderBindState &bindState) const override;
	  protected:
		void AddDefaultVertexAttributes();
		virtual void InitializeShaderResources() override;
		virtual uint32_t GetTextureDescriptorSetIndex() const;
	};
};
