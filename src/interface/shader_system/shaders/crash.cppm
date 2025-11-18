// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.shaders.crash;

export import :shader_system.shader;

export namespace prosper {
	class DLLPROSPER ShaderCrash : public prosper::ShaderGraphics {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

#pragma pack(push, 1)
		struct PushConstants {
			float offset;
		};
#pragma pack(pop)

		ShaderCrash(prosper::IPrContext &context, const std::string &identifier);
		virtual ~ShaderCrash() override;
		bool RecordDraw(prosper::ICommandBuffer &cmd) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
