/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_texture.hpp"
#include "image/prosper_msaa_texture.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_sampler.hpp"
#include "image/prosper_image_view.hpp"
#include <misc/image_create_info.h>

using namespace prosper;

Texture::Texture(Context &context,const std::shared_ptr<Image> &img,const std::vector<std::shared_ptr<ImageView>> &imgViews,const std::shared_ptr<Sampler> &sampler)
	: ContextObject(context),std::enable_shared_from_this<Texture>(),m_image(img),
	m_imageViews(imgViews),m_sampler(sampler)
{}

const std::shared_ptr<Image> &Texture::GetImage() const {return m_image;}
const std::shared_ptr<ImageView> &Texture::GetImageView(uint32_t layerId) const
{
	++layerId;
	static std::shared_ptr<ImageView> nptr = nullptr;
	return (layerId < m_imageViews.size()) ? m_imageViews.at(layerId) : nptr;
}
const std::shared_ptr<ImageView> &Texture::GetImageView() const {return m_imageViews.at(0u);}
const std::shared_ptr<Sampler> &Texture::GetSampler() const {return m_sampler;}
bool Texture::IsMSAATexture() const {return false;}
void Texture::SetDebugName(const std::string &name)
{
	ContextObject::SetDebugName(name);
	if(m_image != nullptr)
		m_image->SetDebugName(name +"_img");
	auto idx = 0u;
	for(auto &imgView : m_imageViews)
		imgView->SetDebugName(name +"_imgView" +std::to_string(idx++));
	if(m_sampler != nullptr)
		m_sampler->SetDebugName(name +"_smp");
}

//////////////////////////

prosper::util::TextureCreateInfo::TextureCreateInfo(const std::shared_ptr<ImageView> &imgView,const std::shared_ptr<Sampler> &smp)
	: imageView(imgView),sampler(smp)
{}
prosper::util::TextureCreateInfo::TextureCreateInfo()
	: TextureCreateInfo(nullptr,nullptr)
{}

std::shared_ptr<Texture> prosper::util::create_texture(
	Anvil::BaseDevice &dev,const TextureCreateInfo &createInfo,const std::shared_ptr<Image> &img,
	const ImageViewCreateInfo *imageViewCreateInfo,const SamplerCreateInfo *samplerCreateInfo
)
{
	auto imageView = createInfo.imageView;
	auto sampler = createInfo.sampler;

	if(imageView == nullptr && imageViewCreateInfo != nullptr)
	{
		auto originalLevelCount = const_cast<ImageViewCreateInfo*>(imageViewCreateInfo)->levelCount;
		if((img->GetCreateFlags() &Anvil::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT) != Anvil::ImageCreateFlagBits::NONE && imageViewCreateInfo->baseLayer == 0u)
			const_cast<ImageViewCreateInfo*>(imageViewCreateInfo)->levelCount = 6u; // TODO: Do this properly
		imageView = create_image_view(dev,*imageViewCreateInfo,img);
		const_cast<ImageViewCreateInfo*>(imageViewCreateInfo)->levelCount = originalLevelCount;
	}

	std::vector<std::shared_ptr<ImageView>> imageViews = {imageView};
	if((createInfo.flags &TextureCreateInfo::Flags::CreateImageViewForEachLayer) != TextureCreateInfo::Flags::None)
	{
		if(imageViewCreateInfo == nullptr)
			throw std::invalid_argument("If 'CreateImageViewForEachLayer' flag is set, a valid ImageViewCreateInfo struct has to be supplied!");
		auto numLayers = img->GetLayerCount();
		imageViews.reserve(imageViews.size() +numLayers);
		for(auto i=decltype(numLayers){0};i<numLayers;++i)
		{
			auto imgViewCreateInfoLayer = *imageViewCreateInfo;
			imgViewCreateInfoLayer.baseLayer = i;
			imageViews.push_back(create_image_view(dev,imgViewCreateInfoLayer,img));
		}
	}

	if(sampler == nullptr && samplerCreateInfo != nullptr)
		sampler = create_sampler(dev,*samplerCreateInfo);
	if(img->GetSampleCount() != Anvil::SampleCountFlagBits::_1_BIT && (createInfo.flags &TextureCreateInfo::Flags::Resolvable) != TextureCreateInfo::Flags::None)
	{
		if(util::is_depth_format(img->GetFormat()))
			throw std::logic_error("Cannot create resolvable multi-sampled depth image: Multi-sample depth images cannot be resolved!");
		auto extents = img->GetExtents();
		prosper::util::ImageCreateInfo imgCreateInfo {};
		imgCreateInfo.format = img->GetFormat();
		imgCreateInfo.width = extents.width;
		imgCreateInfo.height = extents.height;
		imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		imgCreateInfo.type = img->GetType();
		imgCreateInfo.usage = img->GetUsageFlags() | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT;
		imgCreateInfo.tiling = img->GetTiling();
		imgCreateInfo.layers = img->GetLayerCount();
		imgCreateInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
		auto resolvedImg = prosper::util::create_image(dev,imgCreateInfo);
		auto resolvedTexture = create_texture(dev,{},resolvedImg,imageViewCreateInfo,samplerCreateInfo);
		return std::shared_ptr<MSAATexture>(new MSAATexture(Context::GetContext(dev),img,imageViews,sampler,resolvedTexture));
	}
	return std::shared_ptr<Texture>(new Texture(Context::GetContext(dev),img,imageViews,sampler));
}
