// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:shader_system.shaders.blur;

export import :shader_system.shader;
export import :types;

export {
	namespace prosper {
		class RenderTarget;
		class Texture;
		class IDescriptorSet;
		class DLLPROSPER ShaderBlurBase : public ShaderGraphics {
		  public:
			static VertexBinding VERTEX_BINDING_VERTEX;
			static VertexAttribute VERTEX_ATTRIBUTE_POSITION;
			static VertexAttribute VERTEX_ATTRIBUTE_UV;

			static DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

			enum class Pipeline : uint32_t {
				R8G8B8A8Unorm,
				R8Unorm,
				R16G16B16A16Sfloat,
				//BC1,
				//BC2,
				//BC3,

				Count
			};

#pragma pack(push, 1)
			struct PushConstants {
				Vector4 colorScale;
				float blurSize;
				int32_t kernelSize;
			};
#pragma pack(pop)

			ShaderBlurBase(IPrContext &context, const std::string &identifier, const std::string &fsShader);
			bool RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx = Pipeline::R8G8B8A8Unorm) const;
			bool RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const;
		  protected:
			virtual void InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
			virtual void InitializeGfxPipeline(GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
			virtual void InitializeShaderResources() override;
		};

		/////////////////////////

		class DLLPROSPER ShaderBlurH : public ShaderBlurBase {
		  public:
			ShaderBlurH(IPrContext &context, const std::string &identifier);
			~ShaderBlurH();
		};

		/////////////////////////

		class DLLPROSPER ShaderBlurV : public ShaderBlurBase {
		  public:
			ShaderBlurV(IPrContext &context, const std::string &identifier);
			~ShaderBlurV();
		};

		/////////////////////////

		class DLLPROSPER BlurSet {
		  public:
			// If no source texture is specified, the texture of 'finalRt' will be used both as a source and a target
			static std::shared_ptr<BlurSet> Create(IPrContext &context, const std::shared_ptr<RenderTarget> &finalRt, const std::shared_ptr<Texture> &srcTexture = nullptr);

			const std::shared_ptr<RenderTarget> &GetFinalRenderTarget() const;
			IDescriptorSet &GetFinalDescriptorSet() const;
			const std::shared_ptr<RenderTarget> &GetStagingRenderTarget() const;
			IDescriptorSet &GetStagingDescriptorSet() const;
		  private:
			BlurSet(const std::shared_ptr<RenderTarget> &rtFinal, const std::shared_ptr<IDescriptorSetGroup> &descSetFinalGroup, const std::shared_ptr<RenderTarget> &rtStaging, const std::shared_ptr<IDescriptorSetGroup> &descSetStagingGroup,
			  const std::shared_ptr<Texture> &srcTexture);

			std::shared_ptr<RenderTarget> m_outRenderTarget = nullptr;
			std::shared_ptr<IDescriptorSetGroup> m_outDescSetGroup = nullptr;
			std::shared_ptr<RenderTarget> m_stagingRenderTarget = nullptr;
			std::shared_ptr<IDescriptorSetGroup> m_stagingDescSetGroup = nullptr;

			// This member is only used to keep source texture alive, in case it is not part of final render target
			std::shared_ptr<Texture> m_srcTexture = nullptr;
		};

		namespace util {
			struct DLLPROSPER ShaderInfo {
				ShaderBlurBase *shaderH = nullptr;
				uint32_t shaderHPipeline = 0u;
				ShaderBlurBase *shaderV = nullptr;
				uint32_t shaderVPipeline = 0u;
			};
			DLLPROSPER bool record_blur_image(IPrContext &context, const std::shared_ptr<IPrimaryCommandBuffer> &cmdBuffer, const BlurSet &blurSet, const ShaderBlurBase::PushConstants &pushConstants, uint32_t blurStrength = 1u, const ShaderInfo *shaderInfo = nullptr);
		};
	};
}
