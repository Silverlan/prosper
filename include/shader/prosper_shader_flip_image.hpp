// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_SHADER_FLIP_IMAGE_HPP__
#define __PROSPER_SHADER_FLIP_IMAGE_HPP__

#include <shader/prosper_shader_base_image_processing.hpp>

namespace prosper {
	class DLLPROSPER ShaderFlipImage : public prosper::ShaderBaseImageProcessing {
	  public:
#pragma pack(push, 1)
		struct PushConstants {
			uint32_t flipHorizontally;
			uint32_t flipVertically;
		};
#pragma pack(pop)

		ShaderFlipImage(prosper::IPrContext &context, const std::string &identifier);
		virtual ~ShaderFlipImage() override;
		bool RecordDraw(prosper::ICommandBuffer &cmd, prosper::IDescriptorSet &descSetTexture, bool flipHorizontally, bool flipVertically) const;
		// This overload assumes that the texture has already been bound!
		bool RecordDraw(ShaderBindState &bindState, bool flipHorizontally, bool flipVertically) const;
	  protected:
		bool RecordDraw(prosper::ShaderBindState &bindState, prosper::IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};

#endif
