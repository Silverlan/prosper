/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader_blur.hpp"
#include "prosper_context.hpp"
#include "prosper_util_square_shape.hpp"
#include "prosper_util.hpp"
#include "image/prosper_render_target.hpp"
#include "prosper_descriptor_set_group.hpp"
#include "buffers/prosper_buffer.hpp"
#include "buffers/vk_buffer.hpp"
#include "image/prosper_sampler.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"
#include <vulkan/vulkan.hpp>
#include <wrappers/descriptor_set_group.h>
#include <wrappers/graphics_pipeline_manager.h>
#include <misc/image_create_info.h>

using namespace prosper;

decltype(ShaderBlurBase::VERTEX_BINDING_VERTEX) ShaderBlurBase::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION) ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_UV) ShaderBlurBase::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX,prosper::util::get_square_uv_format()};

decltype(ShaderBlurBase::DESCRIPTOR_SET_TEXTURE) ShaderBlurBase::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding {
			DescriptorType::CombinedImageSampler,
			ShaderStageFlags::FragmentBit
		}
	}
};

static constexpr std::array<vk::Format,umath::to_integral(prosper::ShaderBlurBase::Pipeline::Count)> g_pipelineFormats = {
	vk::Format::eR8G8B8A8Unorm,
	vk::Format::eR8Unorm,
	vk::Format::eR16G16B16A16Sfloat,
	vk::Format::eBc1RgbaUnormBlock,
	vk::Format::eBc2UnormBlock,
	vk::Format::eBc3UnormBlock
};

ShaderBlurBase::ShaderBlurBase(prosper::IPrContext &context,const std::string &identifier,const std::string &fsShader)
	: ShaderGraphics(context,identifier,"screen/vs_screen_uv",fsShader)
{
	SetPipelineCount(umath::to_integral(Pipeline::Count));
}

void ShaderBlurBase::InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderBlurBase>({{{static_cast<prosper::Format>(g_pipelineFormats.at(pipelineIdx))}}},outRenderPass,pipelineIdx);
}

void ShaderBlurBase::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	pipelineInfo.toggle_dynamic_states(true,{Anvil::DynamicState::SCISSOR});
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),prosper::ShaderStageFlags::FragmentBit);
}

bool ShaderBlurBase::BeginDraw(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx)
{
	return ShaderGraphics::BeginDraw(cmdBuffer,umath::to_integral(pipelineIdx));
}
bool ShaderBlurBase::Draw(IDescriptorSet &descSetTexture,const PushConstants &pushConstants)
{
	if(
		RecordBindVertexBuffer(dynamic_cast<VlkBuffer&>(*prosper::util::get_square_vertex_uv_buffer(GetContext()))) == false ||
		RecordBindDescriptorSet(static_cast<IDescriptorSet&>(descSetTexture)) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count()) == false
	)
		return false;
	return true;
}

/////////////////////////

static ShaderBlurH *s_blurShaderH = nullptr;
ShaderBlurH::ShaderBlurH(prosper::IPrContext &context,const std::string &identifier)
	: ShaderBlurBase(context,identifier,"screen/fs_gaussianblur_horizontal")
{
	s_blurShaderH = this;
}
ShaderBlurH::~ShaderBlurH() {s_blurShaderH = nullptr;}

/////////////////////////

static ShaderBlurV *s_blurShaderV = nullptr;
ShaderBlurV::ShaderBlurV(prosper::IPrContext &context,const std::string &identifier)
	: ShaderBlurBase(context,identifier,"screen/fs_gaussianblur_vertical")
{
	s_blurShaderV = this;
	SetBaseShader<ShaderBlurH>();
}
ShaderBlurV::~ShaderBlurV() {s_blurShaderV = nullptr;}

/////////////////////////

bool prosper::util::record_blur_image(prosper::IPrContext &context,const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,const BlurSet &blurSet,const ShaderBlurBase::PushConstants &pushConstants)
{
	if(s_blurShaderH == nullptr || s_blurShaderV == nullptr)
		return false;
	auto &shaderH = *s_blurShaderH;
	auto &shaderV = *s_blurShaderV;

	auto &stagingRt = *blurSet.GetStagingRenderTarget();
	auto &stagingImg = stagingRt.GetTexture().GetImage();
	cmdBuffer->RecordImageBarrier(stagingImg,ImageLayout::ShaderReadOnlyOptimal,ImageLayout::ColorAttachmentOptimal);

	if(cmdBuffer->RecordBeginRenderPass(stagingRt) == false)
		return false;
	auto imgFormat = static_cast<vk::Format>(stagingImg.GetFormat());
	auto pipelineId = ShaderBlurBase::Pipeline::R8G8B8A8Unorm;
	switch(imgFormat)
	{
		case vk::Format::eR8G8B8A8Unorm:
			pipelineId = ShaderBlurBase::Pipeline::R8G8B8A8Unorm;
			break;
		case vk::Format::eR8Unorm:
			pipelineId = ShaderBlurBase::Pipeline::R8Unorm;
			break;
		case vk::Format::eR16G16B16A16Sfloat:
			pipelineId = ShaderBlurBase::Pipeline::R16G16B16A16Sfloat;
			break;
		case vk::Format::eBc1RgbaUnormBlock:
			pipelineId = ShaderBlurBase::Pipeline::BC1;
			break;
		case vk::Format::eBc2UnormBlock:
			pipelineId = ShaderBlurBase::Pipeline::BC2;
			break;
		case vk::Format::eBc3UnormBlock:
			pipelineId = ShaderBlurBase::Pipeline::BC3;
			break;
		default:
			throw std::logic_error("Unsupported image format for blur input image!");
	}
	if(shaderH.BeginDraw(cmdBuffer,pipelineId) == false)
	{
		cmdBuffer->RecordEndRenderPass();
		return false;
	}
	shaderH.Draw(blurSet.GetFinalDescriptorSet(),pushConstants);
	shaderH.EndDraw();
	cmdBuffer->RecordEndRenderPass();

	auto &finalRt = *blurSet.GetFinalRenderTarget();
	cmdBuffer->RecordPostRenderPassImageBarrier(
		stagingImg,
		ImageLayout::ColorAttachmentOptimal,ImageLayout::ShaderReadOnlyOptimal
	);
	cmdBuffer->RecordImageBarrier(
		finalRt.GetTexture().GetImage(),
		ImageLayout::ShaderReadOnlyOptimal,ImageLayout::ColorAttachmentOptimal
	);

	if(cmdBuffer->RecordBeginRenderPass(finalRt) == false)
		return false;
	if(shaderV.BeginDraw(cmdBuffer,pipelineId) == false)
		return false;
	shaderV.Draw(blurSet.GetStagingDescriptorSet(),pushConstants);
	shaderV.EndDraw();
	auto success = cmdBuffer->RecordEndRenderPass();
	if(success == false)
		return success;
	cmdBuffer->RecordPostRenderPassImageBarrier(
		finalRt.GetTexture().GetImage(),
		ImageLayout::ColorAttachmentOptimal,ImageLayout::ShaderReadOnlyOptimal
	);
	return success;
}

/////////////////////////

BlurSet::BlurSet(
	const std::shared_ptr<prosper::RenderTarget> &rtFinal,const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetFinalGroup,
	const std::shared_ptr<prosper::RenderTarget> &rtStaging,const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetStagingGroup,
	const std::shared_ptr<prosper::Texture> &srcTexture
)
	: m_outRenderTarget(rtFinal),m_outDescSetGroup(descSetFinalGroup),
		m_stagingRenderTarget(rtStaging),m_stagingDescSetGroup(descSetStagingGroup),
		m_srcTexture(srcTexture)
{}

std::shared_ptr<BlurSet> BlurSet::Create(prosper::IPrContext &context,const std::shared_ptr<prosper::RenderTarget> &finalRt,const std::shared_ptr<prosper::Texture> &srcTexture)
{
	if(s_blurShaderH == nullptr)
		return nullptr;
	auto &rp = finalRt->GetRenderPass();
	auto finalDescSetGroup = context.CreateDescriptorSetGroup(prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &finalTex = (srcTexture != nullptr) ? *srcTexture : finalRt->GetTexture();
	auto &finalImg = finalTex.GetImage();
	finalDescSetGroup->GetDescriptorSet()->SetBindingTexture(finalTex,0u);

	auto extents = finalImg.GetExtents();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = extents.width;
	createInfo.height = extents.height;
	createInfo.format = finalImg.GetFormat();
	createInfo.usage = ImageUsageFlags::SampledBit | ImageUsageFlags::ColorAttachmentBit;
	createInfo.postCreateLayout = ImageLayout::ShaderReadOnlyOptimal;
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	samplerCreateInfo.addressModeU = SamplerAddressMode::ClampToEdge;
	samplerCreateInfo.addressModeV = SamplerAddressMode::ClampToEdge;
	auto img = context.CreateImage(createInfo);
	auto tex = context.CreateTexture({},*img,imgViewCreateInfo,samplerCreateInfo);
	auto stagingRt = context.CreateRenderTarget({tex},rp.shared_from_this());
	stagingRt->SetDebugName("blur_staging_rt");

	auto stagingDescSetGroup = context.CreateDescriptorSetGroup(prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &stagingTex = stagingRt->GetTexture();
	stagingDescSetGroup->GetDescriptorSet()->SetBindingTexture(stagingTex,0u);
	return std::shared_ptr<BlurSet>(new BlurSet(
		finalRt,finalDescSetGroup,
		stagingRt,stagingDescSetGroup,
		finalTex.shared_from_this()
	));
}

const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetFinalRenderTarget() const {return m_outRenderTarget;}
prosper::IDescriptorSet &BlurSet::GetFinalDescriptorSet() const {return *m_outDescSetGroup->GetDescriptorSet();}
const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetStagingRenderTarget() const {return m_stagingRenderTarget;}
prosper::IDescriptorSet &BlurSet::GetStagingDescriptorSet() const {return *m_stagingDescSetGroup->GetDescriptorSet();}
