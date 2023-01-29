/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shader/prosper_shader_blur.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_shader_t.hpp"
#include "prosper_context.hpp"
#include "prosper_util.hpp"
#include "image/prosper_render_target.hpp"
#include "prosper_descriptor_set_group.hpp"
#include "buffers/prosper_buffer.hpp"
#include "image/prosper_sampler.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

decltype(ShaderBlurBase::VERTEX_BINDING_VERTEX) ShaderBlurBase::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION) ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareVertexFormat()};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_UV) ShaderBlurBase::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX, CommonBufferCache::GetSquareUvFormat()};

decltype(ShaderBlurBase::DESCRIPTOR_SET_TEXTURE) ShaderBlurBase::DESCRIPTOR_SET_TEXTURE = {{prosper::DescriptorSetInfo::Binding {DescriptorType::CombinedImageSampler, ShaderStageFlags::FragmentBit}}};

static constexpr std::array<prosper::Format, umath::to_integral(prosper::ShaderBlurBase::Pipeline::Count)> g_pipelineFormats = {
  prosper::Format::R8G8B8A8_UNorm, prosper::Format::R8_UNorm, prosper::Format::R16G16B16A16_SFloat,
  //prosper::Format::BC1_RGBA_UNorm_Block,
  //prosper::Format::BC2_UNorm_Block,
  //prosper::Format::BC3_UNorm_Block
};

ShaderBlurBase::ShaderBlurBase(prosper::IPrContext &context, const std::string &identifier, const std::string &fsShader) : ShaderGraphics(context, identifier, "screen/vs_screen_uv", fsShader) { SetPipelineCount(umath::to_integral(Pipeline::Count)); }

void ShaderBlurBase::InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx) { CreateCachedRenderPass<ShaderBlurBase>({{{static_cast<prosper::Format>(g_pipelineFormats.at(pipelineIdx))}}}, outRenderPass, pipelineIdx); }

void ShaderBlurBase::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);

	pipelineInfo.ToggleDynamicStates(true, {prosper::DynamicState::Scissor});
	AddVertexAttribute(pipelineInfo, VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo, VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo, pipelineIdx, DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo, pipelineIdx, 0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}

bool ShaderBlurBase::RecordBeginDraw(ShaderBindState &bindState, Pipeline pipelineIdx) const { return ShaderGraphics::RecordBeginDraw(bindState, umath::to_integral(pipelineIdx)); }
bool ShaderBlurBase::RecordDraw(ShaderBindState &bindState, IDescriptorSet &descSetTexture, const PushConstants &pushConstants) const
{
	if(RecordBindVertexBuffer(bindState, *GetContext().GetCommonBufferCache().GetSquareVertexUvBuffer()) == false || RecordBindDescriptorSet(bindState, static_cast<IDescriptorSet &>(descSetTexture)) == false || RecordPushConstants(bindState, pushConstants) == false
	  || ShaderGraphics::RecordDraw(bindState, GetContext().GetCommonBufferCache().GetSquareVertexCount()) == false)
		return false;
	return true;
}

/////////////////////////

static ShaderBlurH *s_blurShaderH = nullptr;
ShaderBlurH::ShaderBlurH(prosper::IPrContext &context, const std::string &identifier) : ShaderBlurBase(context, identifier, "screen/fs_gaussianblur_horizontal") { s_blurShaderH = this; }
ShaderBlurH::~ShaderBlurH() { s_blurShaderH = nullptr; }

/////////////////////////

static ShaderBlurV *s_blurShaderV = nullptr;
ShaderBlurV::ShaderBlurV(prosper::IPrContext &context, const std::string &identifier) : ShaderBlurBase(context, identifier, "screen/fs_gaussianblur_vertical")
{
	s_blurShaderV = this;
	SetBaseShader<ShaderBlurH>();
}
ShaderBlurV::~ShaderBlurV() { s_blurShaderV = nullptr; }

/////////////////////////

bool prosper::util::record_blur_image(prosper::IPrContext &context, const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer, const BlurSet &blurSet, const ShaderBlurBase::PushConstants &pushConstants, uint32_t blurStrength)
{
	if(s_blurShaderH == nullptr || s_blurShaderV == nullptr)
		return false;
	auto &shaderH = *s_blurShaderH;
	auto &shaderV = *s_blurShaderV;
	if(shaderH.IsValid() == false || shaderV.IsValid() == false)
		return false;
	auto &stagingRt = *blurSet.GetStagingRenderTarget();
	auto &stagingImg = stagingRt.GetTexture().GetImage();
	auto imgFormat = stagingImg.GetFormat();
	auto pipelineId = ShaderBlurBase::Pipeline::R8G8B8A8Unorm;
	switch(imgFormat) {
	case prosper::Format::R8G8B8A8_UNorm:
	case prosper::Format::BC1_RGBA_UNorm_Block:
	case prosper::Format::BC2_UNorm_Block:
	case prosper::Format::BC3_UNorm_Block:
		pipelineId = ShaderBlurBase::Pipeline::R8G8B8A8Unorm;
		break;
	case prosper::Format::R8_UNorm:
		pipelineId = ShaderBlurBase::Pipeline::R8Unorm;
		break;
	case prosper::Format::R16G16B16A16_SFloat:
		pipelineId = ShaderBlurBase::Pipeline::R16G16B16A16Sfloat;
		break;
	/*case prosper::Format::BC1_RGBA_UNorm_Block:
			pipelineId = ShaderBlurBase::Pipeline::BC1;
			break;
		case prosper::Format::BC2_UNorm_Block:
			pipelineId = ShaderBlurBase::Pipeline::BC2;
			break;
		case prosper::Format::BC3_UNorm_Block:
			pipelineId = ShaderBlurBase::Pipeline::BC3;
			break;*/
	default:
		throw std::logic_error("Unsupported image format for blur input image!");
	}
	auto &finalRt = *blurSet.GetFinalRenderTarget();
	for(auto i = decltype(blurStrength) {0u}; i < blurStrength; ++i) {
		cmdBuffer->RecordImageBarrier(stagingImg, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::ColorAttachmentOptimal);
		if(cmdBuffer->RecordBeginRenderPass(stagingRt) == false)
			return false;
		ShaderBindState bindState {*cmdBuffer};
		if(shaderH.RecordBeginDraw(bindState, pipelineId) == false) {
			cmdBuffer->RecordEndRenderPass();
			return false;
		}
		shaderH.RecordDraw(bindState, blurSet.GetFinalDescriptorSet(), pushConstants);
		shaderH.RecordEndDraw(bindState);
		cmdBuffer->RecordEndRenderPass();

		cmdBuffer->RecordPostRenderPassImageBarrier(stagingImg, ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal);
		cmdBuffer->RecordImageBarrier(finalRt.GetTexture().GetImage(), ImageLayout::ShaderReadOnlyOptimal, ImageLayout::ColorAttachmentOptimal);

		if(cmdBuffer->RecordBeginRenderPass(finalRt) == false)
			return false;
		if(shaderV.RecordBeginDraw(bindState, pipelineId) == false)
			return false;
		shaderV.RecordDraw(bindState, blurSet.GetStagingDescriptorSet(), pushConstants);
		shaderV.RecordEndDraw(bindState);
		auto success = cmdBuffer->RecordEndRenderPass();
		if(success == false)
			return success;
		cmdBuffer->RecordPostRenderPassImageBarrier(finalRt.GetTexture().GetImage(), ImageLayout::ColorAttachmentOptimal, ImageLayout::ShaderReadOnlyOptimal);
	}
	return true;
}

/////////////////////////

BlurSet::BlurSet(const std::shared_ptr<prosper::RenderTarget> &rtFinal, const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetFinalGroup, const std::shared_ptr<prosper::RenderTarget> &rtStaging, const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetStagingGroup,
  const std::shared_ptr<prosper::Texture> &srcTexture)
    : m_outRenderTarget(rtFinal), m_outDescSetGroup(descSetFinalGroup), m_stagingRenderTarget(rtStaging), m_stagingDescSetGroup(descSetStagingGroup), m_srcTexture(srcTexture)
{
}

std::shared_ptr<BlurSet> BlurSet::Create(prosper::IPrContext &context, const std::shared_ptr<prosper::RenderTarget> &finalRt, const std::shared_ptr<prosper::Texture> &srcTexture)
{
	if(s_blurShaderH == nullptr)
		return nullptr;
	auto &rp = finalRt->GetRenderPass();
	auto finalDescSetGroup = context.CreateDescriptorSetGroup(prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &finalTex = (srcTexture != nullptr) ? *srcTexture : finalRt->GetTexture();
	auto &finalImg = finalTex.GetImage();
	finalDescSetGroup->GetDescriptorSet()->SetBindingTexture(finalTex, 0u);

	auto format = finalImg.GetFormat();
	auto extents = finalImg.GetExtents();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = extents.width;
	createInfo.height = extents.height;
	createInfo.format = prosper::util::is_compressed_format(format) ? prosper::Format::R8G8B8A8_UNorm : format; // If it's a compressed format, we'll fall back to RGBA8
	createInfo.usage = ImageUsageFlags::SampledBit | ImageUsageFlags::ColorAttachmentBit | ImageUsageFlags::TransferDstBit;
	createInfo.postCreateLayout = ImageLayout::ShaderReadOnlyOptimal;
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	samplerCreateInfo.addressModeU = SamplerAddressMode::ClampToEdge;
	samplerCreateInfo.addressModeV = SamplerAddressMode::ClampToEdge;
	auto img = context.CreateImage(createInfo);
	auto tex = context.CreateTexture({}, *img, imgViewCreateInfo, samplerCreateInfo);
	auto stagingRt = context.CreateRenderTarget({tex}, rp.shared_from_this());
	stagingRt->SetDebugName("blur_staging_rt");

	auto stagingDescSetGroup = context.CreateDescriptorSetGroup(prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &stagingTex = stagingRt->GetTexture();
	stagingDescSetGroup->GetDescriptorSet()->SetBindingTexture(stagingTex, 0u);
	return std::shared_ptr<BlurSet>(new BlurSet(finalRt, finalDescSetGroup, stagingRt, stagingDescSetGroup, finalTex.shared_from_this()));
}

const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetFinalRenderTarget() const { return m_outRenderTarget; }
prosper::IDescriptorSet &BlurSet::GetFinalDescriptorSet() const { return *m_outDescSetGroup->GetDescriptorSet(); }
const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetStagingRenderTarget() const { return m_stagingRenderTarget; }
prosper::IDescriptorSet &BlurSet::GetStagingDescriptorSet() const { return *m_stagingDescSetGroup->GetDescriptorSet(); }
