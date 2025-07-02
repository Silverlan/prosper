// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_SHADER_COPY_IMAGE_HPP__
#define __PROSPER_SHADER_COPY_IMAGE_HPP__

#include "prosper_shader_base_image_processing.hpp"

namespace prosper {
	class RenderTarget;
	class DLLPROSPER ShaderCopyImage : public ShaderBaseImageProcessing {
	  public:
		enum class Pipeline : uint32_t {
			R8G8B8A8Unorm,

			Count
		};

		ShaderCopyImage(prosper::IPrContext &context, const std::string &identifier);
		bool RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx = Pipeline::R8G8B8A8Unorm) const;
	  protected:
		virtual void InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
	};
};

#endif
