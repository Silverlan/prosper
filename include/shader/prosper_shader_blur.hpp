/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_BLUR_HPP__
#define __PROSPER_SHADER_BLUR_HPP__

#include "shader/prosper_shader.hpp"
#include <uvec.h>

namespace prosper
{
	class RenderTarget;
	class Texture;
	class Buffer;
	class DLLPROSPER ShaderBlurBase
		: public ShaderGraphics
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::Shader::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		enum class Pipeline : uint32_t
		{
			R8G8B8A8Unorm,
			R8Unorm,
			R16G16B16A16Sfloat,

			Count
		};

#pragma pack(push,1)
		struct PushConstants
		{
			Vector4 colorScale;
			float blurSize;
			int32_t kernelSize;
		};
#pragma pack(pop)

		ShaderBlurBase(prosper::Context &context,const std::string &identifier,const std::string &fsShader);
		bool BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx=Pipeline::R8G8B8A8Unorm);
		bool Draw(Anvil::DescriptorSet &descSetTexture,const PushConstants &pushConstants);
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	/////////////////////////

	class DLLPROSPER ShaderBlurH
		: public ShaderBlurBase
	{
	public:
		ShaderBlurH(prosper::Context &context,const std::string &identifier);
		~ShaderBlurH();
	};

	/////////////////////////

	class DLLPROSPER ShaderBlurV
		: public ShaderBlurBase
	{
	public:
		ShaderBlurV(prosper::Context &context,const std::string &identifier);
		~ShaderBlurV();
	};

	/////////////////////////

	class DLLPROSPER BlurSet
	{
	public:
		// If no source texture is specified, the texture of 'finalRt' will be used both as a source and a target
		static std::shared_ptr<BlurSet> Create(Anvil::BaseDevice &dev,const std::shared_ptr<prosper::RenderTarget> &finalRt,const std::shared_ptr<prosper::Texture> &srcTexture=nullptr);

		const std::shared_ptr<prosper::RenderTarget> &GetFinalRenderTarget() const;
		Anvil::DescriptorSet &GetFinalDescriptorSet() const;
		const std::shared_ptr<prosper::RenderTarget> &GetStagingRenderTarget() const;
		Anvil::DescriptorSet &GetStagingDescriptorSet() const;
	private:
		BlurSet(
			const std::shared_ptr<prosper::RenderTarget> &rtFinal,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetFinalGroup,
			const std::shared_ptr<prosper::RenderTarget> &rtStaging,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetStagingGroup,
			const std::shared_ptr<prosper::Texture> &srcTexture
		);

		std::shared_ptr<prosper::RenderTarget> m_outRenderTarget = nullptr;
		std::shared_ptr<prosper::DescriptorSetGroup> m_outDescSetGroup = nullptr;
		std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
		std::shared_ptr<prosper::DescriptorSetGroup> m_stagingDescSetGroup = nullptr;

		// This member is only used to keep source texture alive, in case it is not part of final render target
		std::shared_ptr<prosper::Texture> m_srcTexture = nullptr;
	};

	namespace util
	{
		DLLPROSPER bool record_blur_image(Anvil::BaseDevice &dev,const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,const BlurSet &blurSet,const ShaderBlurBase::PushConstants &pushConstants);
	};
};

#endif
