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
#include "image/prosper_sampler.hpp"
#include "prosper_command_buffer.hpp"
#include <vulkan/vulkan.hpp>
#include <wrappers/descriptor_set_group.h>
#include <misc/image_create_info.h>

using namespace prosper;

decltype(ShaderBlurBase::VERTEX_BINDING_VERTEX) ShaderBlurBase::VERTEX_BINDING_VERTEX = {Anvil::VertexInputRate::VERTEX};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION) ShaderBlurBase::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::util::get_square_vertex_format()};
decltype(ShaderBlurBase::VERTEX_ATTRIBUTE_UV) ShaderBlurBase::VERTEX_ATTRIBUTE_UV = {VERTEX_BINDING_VERTEX,prosper::util::get_square_uv_format()};

decltype(ShaderBlurBase::DESCRIPTOR_SET_TEXTURE) ShaderBlurBase::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding {
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
ShaderBlurBase::ShaderBlurBase(prosper::Context &context,const std::string &identifier,const std::string &fsShader)
	: ShaderGraphics(context,identifier,"screen/vs_screen_uv",fsShader)
{
	SetPipelineCount(umath::to_integral(Pipeline::Count));
}

void ShaderBlurBase::InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	switch(static_cast<Pipeline>(pipelineIdx))
	{
		case Pipeline::R8G8B8A8Unorm:
			CreateCachedRenderPass<ShaderBlurBase>({{{Anvil::Format::R8G8B8A8_UNORM}}},outRenderPass,pipelineIdx);
			break;
		case Pipeline::R8Unorm:
			CreateCachedRenderPass<ShaderBlurBase>({{{Anvil::Format::R8_UNORM}}},outRenderPass,pipelineIdx);
			break;
		case Pipeline::R16G16B16A16Sfloat:
			CreateCachedRenderPass<ShaderBlurBase>({{{Anvil::Format::R16G16B16A16_SFLOAT}}},outRenderPass,pipelineIdx);
			break;
	}
}

void ShaderBlurBase::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	pipelineInfo.toggle_dynamic_states(true,{Anvil::DynamicState::SCISSOR});
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_UV);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}

bool ShaderBlurBase::BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,Pipeline pipelineIdx)
{
	return ShaderGraphics::BeginDraw(cmdBuffer,umath::to_integral(pipelineIdx));
}
bool ShaderBlurBase::Draw(Anvil::DescriptorSet &descSetTexture,const PushConstants &pushConstants)
{
	auto &dev = GetContext().GetDevice();
	if(
		RecordBindVertexBuffer(prosper::util::get_square_vertex_uv_buffer(dev)->GetAnvilBuffer()) == false ||
		RecordBindDescriptorSet(descSetTexture) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(prosper::util::get_square_vertex_count()) == false
	)
		return false;
	return true;
}

/////////////////////////

static ShaderBlurH *s_blurShaderH = nullptr;
ShaderBlurH::ShaderBlurH(prosper::Context &context,const std::string &identifier)
	: ShaderBlurBase(context,identifier,"screen/fs_gaussianblur_horizontal")
{
	s_blurShaderH = this;
}
ShaderBlurH::~ShaderBlurH() {s_blurShaderH = nullptr;}

/////////////////////////

static ShaderBlurV *s_blurShaderV = nullptr;
ShaderBlurV::ShaderBlurV(prosper::Context &context,const std::string &identifier)
	: ShaderBlurBase(context,identifier,"screen/fs_gaussianblur_vertical")
{
	s_blurShaderV = this;
	SetBaseShader<ShaderBlurH>();
}
ShaderBlurV::~ShaderBlurV() {s_blurShaderV = nullptr;}

/////////////////////////

bool prosper::util::record_blur_image(Anvil::BaseDevice &dev,const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,const BlurSet &blurSet,const ShaderBlurBase::PushConstants &pushConstants)
{
	if(s_blurShaderH == nullptr || s_blurShaderV == nullptr)
		return false;
	auto &shaderH = *s_blurShaderH;
	auto &shaderV = *s_blurShaderV;

	auto &primBuffer = cmdBuffer->GetAnvilCommandBuffer();
	auto &stagingRt = *blurSet.GetStagingRenderTarget();
	auto &stagingImg = *stagingRt.GetTexture()->GetImage();
	prosper::util::record_image_barrier(primBuffer,*stagingImg,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);

	if(prosper::util::record_begin_render_pass(primBuffer,stagingRt) == false)
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
		default:
			throw std::logic_error("Unsupported image format for blur input image!");
	}
	if(shaderH.BeginDraw(cmdBuffer,pipelineId) == false)
	{
		prosper::util::record_end_render_pass(primBuffer);
		return false;
	}
	shaderH.Draw(blurSet.GetFinalDescriptorSet(),pushConstants);
	shaderH.EndDraw();
	prosper::util::record_end_render_pass(primBuffer);

	auto &finalRt = *blurSet.GetFinalRenderTarget();
	prosper::util::record_image_barrier(
		primBuffer,*stagingImg,
		Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,Anvil::AccessFlagBits::SHADER_READ_BIT
	);
	prosper::util::record_image_barrier(
		primBuffer,finalRt.GetTexture()->GetImage()->GetAnvilImage(),
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
	);

	if(prosper::util::record_begin_render_pass(primBuffer,finalRt) == false)
		return false;
	if(shaderV.BeginDraw(cmdBuffer,pipelineId) == false)
		return false;
	shaderV.Draw(blurSet.GetStagingDescriptorSet(),pushConstants);
	shaderV.EndDraw();
	return prosper::util::record_end_render_pass(primBuffer);
}

/////////////////////////

BlurSet::BlurSet(
	const std::shared_ptr<prosper::RenderTarget> &rtFinal,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetFinalGroup,
	const std::shared_ptr<prosper::RenderTarget> &rtStaging,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetStagingGroup,
	const std::shared_ptr<prosper::Texture> &srcTexture
)
	: m_outRenderTarget(rtFinal),m_outDescSetGroup(descSetFinalGroup),
		m_stagingRenderTarget(rtStaging),m_stagingDescSetGroup(descSetStagingGroup),
		m_srcTexture(srcTexture)
{}

std::shared_ptr<BlurSet> BlurSet::Create(Anvil::BaseDevice &dev,const std::shared_ptr<prosper::RenderTarget> &finalRt,const std::shared_ptr<prosper::Texture> &srcTexture)
{
	if(s_blurShaderH == nullptr)
		return nullptr;
	auto rp = finalRt->GetRenderPass();
	if(rp == nullptr)
		return nullptr;
	auto finalDescSetGroup = prosper::util::create_descriptor_set_group(dev,prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &finalTex = (srcTexture != nullptr) ? *srcTexture : *finalRt->GetTexture();
	auto &finalImg = *finalTex.GetImage();
	prosper::util::set_descriptor_set_binding_texture(*(*finalDescSetGroup)->get_descriptor_set(0u),finalTex,0u);

	auto extents = finalImg.GetExtents();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = extents.width;
	createInfo.height = extents.height;
	createInfo.format = finalImg.GetFormat();
	createInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT;
	createInfo.postCreateLayout = Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	samplerCreateInfo.addressModeU = Anvil::SamplerAddressMode::CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = Anvil::SamplerAddressMode::CLAMP_TO_EDGE;
	auto img = prosper::util::create_image(dev,createInfo);
	auto tex = prosper::util::create_texture(dev,{},std::move(img),&imgViewCreateInfo,&samplerCreateInfo);
	auto stagingRt = prosper::util::create_render_target(dev,{tex},rp);
	stagingRt->SetDebugName("blur_staging_rt");

	auto stagingDescSetGroup = prosper::util::create_descriptor_set_group(dev,prosper::ShaderBlurBase::DESCRIPTOR_SET_TEXTURE);
	auto &stagingTex = *stagingRt->GetTexture();
	prosper::util::set_descriptor_set_binding_texture(*(*stagingDescSetGroup)->get_descriptor_set(0u),stagingTex,0u);
	return std::shared_ptr<BlurSet>(new BlurSet(
		finalRt,finalDescSetGroup,
		stagingRt,stagingDescSetGroup,
		finalTex.shared_from_this()
	));
}

const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetFinalRenderTarget() const {return m_outRenderTarget;}
Anvil::DescriptorSet &BlurSet::GetFinalDescriptorSet() const {return *(*m_outDescSetGroup)->get_descriptor_set(0u);}
const std::shared_ptr<prosper::RenderTarget> &BlurSet::GetStagingRenderTarget() const {return m_stagingRenderTarget;}
Anvil::DescriptorSet &BlurSet::GetStagingDescriptorSet() const {return *(*m_stagingDescSetGroup)->get_descriptor_set(0u);}
