// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.shaders.copy_image;

export import :shader_system.shaders.base_image_processing;

export namespace prosper {
	class RenderTarget;
	class DLLPROSPER ShaderCopyImage : public ShaderBaseImageProcessing {
	  public:
		enum class Pipeline : uint32_t {
			R8G8B8A8Unorm,

			Count
		};

		ShaderCopyImage(IPrContext &context, const std::string &identifier);
		bool RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx = Pipeline::R8G8B8A8Unorm) const;
	  protected:
		virtual void InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
	};
};
