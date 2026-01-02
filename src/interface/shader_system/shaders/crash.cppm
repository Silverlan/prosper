// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:shader_system.shaders.crash;

export import :shader_system.shader;

export namespace prosper {
	class DLLPROSPER ShaderCrash : public ShaderGraphics {
	  public:
		static VertexBinding VERTEX_BINDING_VERTEX;
		static VertexAttribute VERTEX_ATTRIBUTE_POSITION;

#pragma pack(push, 1)
		struct PushConstants {
			float offset;
		};
#pragma pack(pop)

		ShaderCrash(IPrContext &context, const std::string &identifier);
		virtual ~ShaderCrash() override;
		bool RecordDraw(ICommandBuffer &cmd) const;
	  protected:
		virtual void InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
