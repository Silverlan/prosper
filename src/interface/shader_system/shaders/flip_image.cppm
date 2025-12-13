// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.shaders.flip_image;

export import :shader_system.shaders.base_image_processing;

export namespace prosper {
	class DLLPROSPER ShaderFlipImage : public ShaderBaseImageProcessing {
	  public:
#pragma pack(push, 1)
		struct PushConstants {
			uint32_t flipHorizontally;
			uint32_t flipVertically;
		};
#pragma pack(pop)

		ShaderFlipImage(IPrContext &context, const std::string &identifier);
		virtual ~ShaderFlipImage() override;
		bool RecordDraw(ICommandBuffer &cmd, IDescriptorSet &descSetTexture, bool flipHorizontally, bool flipVertically) const;
		// This overload assumes that the texture has already been bound!
		bool RecordDraw(ShaderBindState &bindState, bool flipHorizontally, bool flipVertically) const;
	  protected:
		bool RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const;
		virtual void InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
