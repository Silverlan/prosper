// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:shader_system.shaders.base_image_processing;

export import :shader_system.shader;

export namespace prosper {
	class RenderTarget;
	namespace shaderBaseImageProcessing {
		CLASS_ENUM_COMPAT DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;
		// WINDOWS_CLANG_COMPILER_FIX
		DLLPROSPER DescriptorSetInfo get_descriptor_set_texture();
	}
	class DLLPROSPER ShaderBaseImageProcessing : public ShaderGraphics {
	  public:
		static VertexBinding VERTEX_BINDING_VERTEX;
		static VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static VertexBinding VERTEX_BINDING_UV;
		static VertexAttribute VERTEX_ATTRIBUTE_UV;

		ShaderBaseImageProcessing(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader);
		ShaderBaseImageProcessing(IPrContext &context, const std::string &identifier, const std::string &fsShader);
		bool RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture) const;
		virtual bool RecordDraw(ShaderBindState &bindState) const override;
	  protected:
		void AddDefaultVertexAttributes();
		virtual void InitializeShaderResources() override;
		virtual uint32_t GetTextureDescriptorSetIndex() const;
	};
};
