/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_msaa_texture.hpp"
#include "image/prosper_texture.hpp"
#include "prosper_util.hpp"
#include <wrappers/command_buffer.h>
#include <misc/image_create_info.h>

using namespace prosper;

prosper::MSAATexture::MSAATexture(
	Context &context,const std::shared_ptr<Image> &img,const std::vector<std::shared_ptr<ImageView>> &imgViews,const std::shared_ptr<Sampler> &sampler,
	const std::shared_ptr<Texture> &resolvedTexture
)
	: Texture(context,img,imgViews,sampler),m_resolvedTexture(resolvedTexture)
{
	if((img->GetAnvilImage().get_create_info_ptr()->get_usage_flags() &Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT) == Anvil::ImageUsageFlagBits::NONE)
		throw std::logic_error("MSAA source image must be created with VK_IMAGE_USAGE_TRANSFER_SRC_BIT usage flag!");
	if((resolvedTexture->GetImage()->GetAnvilImage().get_create_info_ptr()->get_usage_flags() &Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT) == Anvil::ImageUsageFlagBits::NONE)
		throw std::logic_error("MSAA destination image must be created with VK_IMAGE_USAGE_TRANSFER_DST_BIT usage flag!");
}

const std::shared_ptr<Texture> &prosper::MSAATexture::GetResolvedTexture() const {return m_resolvedTexture;}

std::shared_ptr<Texture> prosper::MSAATexture::Resolve(
	Anvil::CommandBufferBase &cmdBuffer,Anvil::ImageLayout msaaLayoutIn,Anvil::ImageLayout msaaLayoutOut,Anvil::ImageLayout resolvedLayoutIn,Anvil::ImageLayout resolvedLayoutOut
)
{
	auto &imgSrc = GetImage();
	if(util::is_depth_format(imgSrc->GetFormat()))
		return nullptr; // MSAA depth-images cannot be resolved!
	if(imgSrc->GetSampleCount() == Anvil::SampleCountFlagBits::_1_BIT)
	{
		m_bResolved = true;
		return shared_from_this();
	}
	if(m_bResolved == false)
	{
		auto extents = imgSrc->GetExtents();
		auto &imgDst = m_resolvedTexture->GetImage();
		if(msaaLayoutIn != Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL)
			prosper::util::record_image_barrier(cmdBuffer,**imgSrc,msaaLayoutIn,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL);
		if(resolvedLayoutIn != Anvil::ImageLayout::TRANSFER_DST_OPTIMAL)
			prosper::util::record_image_barrier(cmdBuffer,**imgDst,resolvedLayoutIn,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL);
		const auto srcAspectFlags = Anvil::ImageAspectFlagBits::COLOR_BIT;
		const auto dstAspectFlags = Anvil::ImageAspectFlagBits::COLOR_BIT;
		Anvil::ImageSubresourceLayers srcLayer {srcAspectFlags,0,0,1};
		Anvil::ImageSubresourceLayers destLayer {dstAspectFlags,0,0,1};
		Anvil::ImageResolve resolve {srcLayer,vk::Offset3D{0,0,0},destLayer,vk::Offset3D{0,0,0},vk::Extent3D{extents.width,extents.height,1}};
		m_bResolved = cmdBuffer.record_resolve_image(
			&GetImage()->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
			&m_resolvedTexture->GetImage()->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,1u,&resolve
		);
		if(msaaLayoutOut != Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL)
			prosper::util::record_image_barrier(cmdBuffer,**imgSrc,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,msaaLayoutOut);
		if(resolvedLayoutOut != Anvil::ImageLayout::TRANSFER_DST_OPTIMAL)
			prosper::util::record_image_barrier(cmdBuffer,**imgDst,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,resolvedLayoutOut);
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
