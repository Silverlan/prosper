/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_image.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "debug/prosper_debug.hpp"
#include "prosper_memory_tracker.hpp"
#include "prosper_command_buffer.hpp"
#include <misc/image_create_info.h>
#include <wrappers/memory_block.h>

using namespace prosper;

std::shared_ptr<Image> Image::Create(Context &context,Anvil::ImageUniquePtr img,const std::function<void(Image&)> &onDestroyedCallback)
{
	if(img == nullptr)
		return nullptr;
	return std::shared_ptr<Image>(new Image(context,std::move(img)),[onDestroyedCallback](Image *img) {
		if(onDestroyedCallback != nullptr)
			onDestroyedCallback(*img);
		img->GetContext().ReleaseResource<Image>(img);
	});
}

static std::unique_ptr<std::unordered_map<Anvil::Image*,Image*>> s_imageMap = nullptr;
Image *prosper::debug::get_image_from_anvil_image(Anvil::Image &img)
{
	if(s_imageMap == nullptr)
		return nullptr;
	auto it = s_imageMap->find(&img);
	return (it != s_imageMap->end()) ? it->second : nullptr;
}
Image::Image(Context &context,Anvil::ImageUniquePtr img)
	: ContextObject(context),std::enable_shared_from_this<Image>(),m_image(std::move(img))
{
	if(prosper::debug::is_debug_mode_enabled())
	{
		if(s_imageMap == nullptr)
			s_imageMap = std::make_unique<std::unordered_map<Anvil::Image*,Image*>>();
		(*s_imageMap)[m_image.get()] = this;
	}
	prosper::debug::register_debug_object(m_image->get_image(),this,prosper::debug::ObjectType::Image);
	MemoryTracker::GetInstance().AddResource(*this);
}

Image::~Image()
{
	if(s_imageMap != nullptr)
	{
		auto it = s_imageMap->find(m_image.get());
		if(it != s_imageMap->end())
			s_imageMap->erase(it);
	}
	prosper::debug::deregister_debug_object(m_image->get_image());
	MemoryTracker::GetInstance().RemoveResource(*this);
}
Anvil::Image &Image::GetAnvilImage() const {return *m_image;}
Anvil::Image &Image::operator*() {return *m_image;}
const Anvil::Image &Image::operator*() const {return const_cast<Image*>(this)->operator*();}
Anvil::Image *Image::operator->() {return m_image.get();}
const Anvil::Image *Image::operator->() const {return const_cast<Image*>(this)->operator->();}

Anvil::ImageType Image::GetType() const {return m_image->get_create_info_ptr()->get_type();}
bool Image::IsCubemap() const
{
	return (GetCreateFlags() &Anvil::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT) != Anvil::ImageCreateFlagBits::NONE;
}
Anvil::ImageUsageFlags Image::GetUsageFlags() const {return m_image->get_create_info_ptr()->get_usage_flags();}
uint32_t Image::GetLayerCount() const {return m_image->get_create_info_ptr()->get_n_layers();}
Anvil::ImageTiling Image::GetTiling() const {return m_image->get_create_info_ptr()->get_tiling();}
Anvil::Format Image::GetFormat() const {return m_image->get_create_info_ptr()->get_format();}
Anvil::SampleCountFlagBits Image::GetSampleCount() const {return m_image->get_create_info_ptr()->get_sample_count();}
vk::Extent2D Image::GetExtents(uint32_t mipLevel) const
{
	auto extents = m_image->get_image_extent_2D(mipLevel);
	return reinterpret_cast<vk::Extent2D&>(extents);
}
Anvil::ImageCreateFlags Image::GetCreateFlags() const {return m_image->get_create_info_ptr()->get_create_flags();}
uint32_t Image::GetMipmapCount() const {return m_image->get_n_mipmaps();}
Anvil::SharingMode Image::GetSharingMode() const {return m_image->get_create_info_ptr()->get_sharing_mode();}
Anvil::ImageAspectFlagBits Image::GetAspectFlags() const {return prosper::util::get_aspect_mask(GetFormat());}
std::optional<Anvil::SubresourceLayout> Image::GetSubresourceLayout(uint32_t layerId,uint32_t mipMapIdx)
{
	Anvil::SubresourceLayout subresourceLayout;
	if(GetAnvilImage().get_aspect_subresource_layout(Anvil::ImageAspectFlagBits::COLOR_BIT,layerId,mipMapIdx,&subresourceLayout) == false)
		return std::optional<Anvil::SubresourceLayout>{};
	return subresourceLayout;
}

void Image::GetCreateInfo(prosper::util::ImageCreateInfo &outCreateInfo) const
{
	outCreateInfo.flags = util::ImageCreateInfo::Flags::None;
	if(GetMipmapCount() > 1)
		outCreateInfo.flags |= util::ImageCreateInfo::Flags::FullMipmapChain;
	if(IsCubemap())
		outCreateInfo.flags |= util::ImageCreateInfo::Flags::Cubemap;
	outCreateInfo.format = GetFormat();
	auto extents = GetExtents();
	outCreateInfo.width = extents.width;
	outCreateInfo.height = extents.height;
	outCreateInfo.layers = GetLayerCount();

	auto &memBlock = *GetAnvilImage().get_memory_block();
	outCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::None;
	auto memFeatures = memBlock.get_create_info_ptr()->get_memory_features();
	if((memFeatures &Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT) != Anvil::MemoryFeatureFlagBits::NONE)
		outCreateInfo.memoryFeatures |= prosper::util::MemoryFeatureFlags::DeviceLocal;
	if((memFeatures &Anvil::MemoryFeatureFlagBits::HOST_CACHED_BIT) != Anvil::MemoryFeatureFlagBits::NONE)
		outCreateInfo.memoryFeatures |= prosper::util::MemoryFeatureFlags::HostCached;
	if((memFeatures &Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT) != Anvil::MemoryFeatureFlagBits::NONE)
		outCreateInfo.memoryFeatures |= prosper::util::MemoryFeatureFlags::HostCoherent;
	if((memFeatures &Anvil::MemoryFeatureFlagBits::LAZILY_ALLOCATED_BIT) != Anvil::MemoryFeatureFlagBits::NONE)
		outCreateInfo.memoryFeatures |= prosper::util::MemoryFeatureFlags::LazilyAllocated;
	if((memFeatures &Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) != Anvil::MemoryFeatureFlagBits::NONE)
		outCreateInfo.memoryFeatures |= prosper::util::MemoryFeatureFlags::HostAccessable;

	outCreateInfo.samples = GetSampleCount();
	outCreateInfo.tiling = GetTiling();
	outCreateInfo.type = GetType();
	outCreateInfo.usage = GetUsageFlags();
}

std::shared_ptr<Image> Image::Copy(prosper::CommandBuffer &cmd,const prosper::util::ImageCreateInfo &copyCreateInfo)
{
	auto &context = GetContext();
	auto &dev = context.GetDevice();

	auto createInfo = copyCreateInfo;
	createInfo.usage |= Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT;
	auto finalLayout = createInfo.postCreateLayout;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;

	auto imgCopy = prosper::util::create_image(dev,createInfo);
	if(imgCopy == nullptr)
		return nullptr;
	prosper::util::BlitInfo blitInfo {};
	blitInfo.dstSubresourceLayer.layer_count = blitInfo.srcSubresourceLayer.layer_count = umath::min(imgCopy->GetLayerCount(),GetLayerCount());

	auto numMipmaps = umath::min(imgCopy->GetMipmapCount(),GetMipmapCount());
	for(auto i=decltype(numMipmaps){0u};i<numMipmaps;++i)
	{
		blitInfo.dstSubresourceLayer.mip_level = blitInfo.srcSubresourceLayer.mip_level = i;
		if(prosper::util::record_blit_image(*cmd,blitInfo,**this,**imgCopy) == false)
			return nullptr;
	}
	prosper::util::record_image_barrier(*cmd,**imgCopy,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,finalLayout);
	return imgCopy;
}
