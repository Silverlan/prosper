/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_msaa_texture.hpp"
#include "image/prosper_texture.hpp"
#include "image/vk_image.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include <wrappers/command_buffer.h>
#include <misc/image_create_info.h>

using namespace prosper;

prosper::MSAATexture::MSAATexture(
	Context &context,IImage &img,const std::vector<std::shared_ptr<IImageView>> &imgViews,ISampler *sampler,
	const std::shared_ptr<Texture> &resolvedTexture
)
	: Texture(context,img,imgViews,sampler),m_resolvedTexture(resolvedTexture)
{
	if(umath::is_flag_set(img.GetUsageFlags(),prosper::ImageUsageFlags::TransferSrcBit) == false)
		throw std::logic_error("MSAA source image must be created with VK_IMAGE_USAGE_TRANSFER_SRC_BIT usage flag!");
	if((static_cast<VlkImage&>(resolvedTexture->GetImage()).GetAnvilImage().get_create_info_ptr()->get_usage_flags() &Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT) == Anvil::ImageUsageFlagBits::NONE)
		throw std::logic_error("MSAA destination image must be created with VK_IMAGE_USAGE_TRANSFER_DST_BIT usage flag!");
}

const std::shared_ptr<Texture> &prosper::MSAATexture::GetResolvedTexture() const {return m_resolvedTexture;}

std::shared_ptr<Texture> prosper::MSAATexture::Resolve(
	prosper::ICommandBuffer &cmdBuffer,ImageLayout msaaLayoutIn,ImageLayout msaaLayoutOut,ImageLayout resolvedLayoutIn,ImageLayout resolvedLayoutOut
)
{
	auto &imgSrc = GetImage();
	if(util::is_depth_format(imgSrc.GetFormat()))
		return nullptr; // MSAA depth-images cannot be resolved!
	if(imgSrc.GetSampleCount() == SampleCountFlags::e1Bit)
	{
		m_bResolved = true;
		return shared_from_this();
	}
	if(m_bResolved == false)
	{
		auto extents = imgSrc.GetExtents();
		auto &imgDst = m_resolvedTexture->GetImage();
		if(msaaLayoutIn != ImageLayout::TransferSrcOptimal)
			cmdBuffer.RecordImageBarrier(imgSrc,msaaLayoutIn,ImageLayout::TransferSrcOptimal);
		if(resolvedLayoutIn != ImageLayout::TransferDstOptimal)
			cmdBuffer.RecordImageBarrier(imgDst,resolvedLayoutIn,ImageLayout::TransferDstOptimal);
		m_bResolved = cmdBuffer.RecordResolveImage(GetImage(),m_resolvedTexture->GetImage());
		if(msaaLayoutOut != ImageLayout::TransferSrcOptimal)
			cmdBuffer.RecordImageBarrier(imgSrc,ImageLayout::TransferSrcOptimal,msaaLayoutOut);
		if(resolvedLayoutOut != ImageLayout::TransferDstOptimal)
			cmdBuffer.RecordImageBarrier(imgDst,ImageLayout::TransferDstOptimal,resolvedLayoutOut);
	}
	return m_resolvedTexture;
}

void prosper::MSAATexture::SetDebugName(const std::string &name)
{
	Texture::SetDebugName(name);
	if(m_resolvedTexture == nullptr)
		return;
	m_resolvedTexture->SetDebugName(name +"_resolved");
}

bool prosper::MSAATexture::IsMSAATexture() const {return true;}

void prosper::MSAATexture::Reset() {m_bResolved = false;}
