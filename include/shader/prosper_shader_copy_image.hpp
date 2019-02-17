/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_COPY_IMAGE_HPP__
#define __PROSPER_SHADER_COPY_IMAGE_HPP__

#include "prosper_shader_base_image_processing.hpp"

namespace prosper
{
	class RenderTarget;
	class DLLPROSPER ShaderCopyImage
		: public ShaderBaseImageProcessing
	{
	public:
		enum class Pipeline : uint32_t
		{
			R8G8B8A8Unorm,

			Count
		};

		ShaderCopyImage(prosper::Context &context,const std::string &identifier);
		bool BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx=Pipeline::R8G8B8A8Unorm);
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};
};

#endif
