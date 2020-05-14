/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_RECT_HPP__
#define __PROSPER_SHADER_RECT_HPP__

#include "shader/prosper_shader_base_image_processing.hpp"

namespace prosper
{
	class RenderTarget;
	class DLLPROSPER ShaderRect
		: public ShaderBaseImageProcessing
	{
	public:
		ShaderRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader);
		ShaderRect(prosper::IPrContext &context,const std::string &identifier);
		bool Draw(const Mat4 &modelMatrix);
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
