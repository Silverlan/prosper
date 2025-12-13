// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.shaders.rect;

export import :shader_system.shaders.base_image_processing;

export namespace prosper {
	class RenderTarget;
	class DLLPROSPER ShaderRect : public ShaderBaseImageProcessing {
	  public:
		ShaderRect(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader);
		ShaderRect(IPrContext &context, const std::string &identifier);
		bool RecordDraw(ShaderBindState &bindState, const Mat4 &modelMatrix) const;
	  protected:
		virtual void InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
