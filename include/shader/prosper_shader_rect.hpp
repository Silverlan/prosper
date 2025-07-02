// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_SHADER_RECT_HPP__
#define __PROSPER_SHADER_RECT_HPP__

#include "shader/prosper_shader_base_image_processing.hpp"

namespace prosper {
	class RenderTarget;
	class DLLPROSPER ShaderRect : public ShaderBaseImageProcessing {
	  public:
		ShaderRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader);
		ShaderRect(prosper::IPrContext &context, const std::string &identifier);
		bool RecordDraw(ShaderBindState &bindState, const Mat4 &modelMatrix) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};

#endif
