/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_image.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "debug/prosper_debug.hpp"
#include "vk_image.hpp"
#include "prosper_memory_tracker.hpp"
#include "prosper_command_buffer.hpp"
#include "buffers/vk_buffer.hpp"

using namespace prosper;

static_assert(sizeof(prosper::Extent2D) == sizeof(vk::Extent2D));

IImage::IImage(IPrContext &context,const prosper::util::ImageCreateInfo &createInfo)
	: ContextObject(context),std::enable_shared_from_this<IImage>(),m_createInfo{createInfo}
{
	MemoryTracker::GetInstance().AddResource(*this);
}

IImage::~IImage() {MemoryTracker::GetInstance().RemoveResource(*this);}

ImageType IImage::GetType() const {return m_createInfo.type;}
bool IImage::IsCubemap() const
{
	return umath::is_flag_set(m_createInfo.flags,prosper::util::ImageCreateInfo::Flags::Cubemap);
}
ImageUsageFlags IImage::GetUsageFlags() const {return m_createInfo.usage;}
uint32_t IImage::GetLayerCount() const {return m_createInfo.layers;}
ImageTiling IImage::GetTiling() const {return m_createInfo.tiling;}
Format IImage::GetFormat() const {return m_createInfo.format;}
SampleCountFlags IImage::GetSampleCount() const {return m_createInfo.samples;}
Extent2D IImage::GetExtents(uint32_t mipLevel) const
{
	return {GetWidth(mipLevel),GetHeight(mipLevel)};
}
uint32_t IImage::GetWidth(uint32_t mipLevel) const {return prosper::util::calculate_mipmap_size(m_createInfo.width,mipLevel);}
uint32_t IImage::GetHeight(uint32_t mipLevel) const {return prosper::util::calculate_mipmap_size(m_createInfo.height,mipLevel);}
uint32_t IImage::GetMipmapCount() const
{
	if(umath::is_flag_set(m_createInfo.flags,prosper::util::ImageCreateInfo::Flags::FullMipmapChain))
	{
		auto extents = GetExtents();
		return prosper::util::calculate_mipmap_count(extents.width,extents.height);
	}
	return 1u;
}
SharingMode IImage::GetSharingMode() const
{
	auto sharingMode = SharingMode::Exclusive;
	if((m_createInfo.flags &prosper::util::ImageCreateInfo::Flags::ConcurrentSharing) != prosper::util::ImageCreateInfo::Flags::None)
		sharingMode = SharingMode::Concurrent;
	return sharingMode;
}
ImageAspectFlags IImage::GetAspectFlags() const {return prosper::util::get_aspect_mask(GetFormat());}
const prosper::IBuffer *IImage::GetMemoryBuffer() const {return m_buffer.get();}
prosper::IBuffer *IImage::GetMemoryBuffer() {return m_buffer.get();}

bool IImage::SetMemoryBuffer(prosper::IBuffer &buffer)
{
	m_buffer = buffer.shared_from_this();
	return DoSetMemoryBuffer(buffer);
}

const prosper::util::ImageCreateInfo &IImage::GetCreateInfo() const {return m_createInfo;}

std::shared_ptr<IImage> IImage::Copy(prosper::ICommandBuffer &cmd,const prosper::util::ImageCreateInfo &copyCreateInfo)
{
	auto &context = GetContext();

	auto createInfo = copyCreateInfo;
	createInfo.usage |= ImageUsageFlags::TransferDstBit;
	auto finalLayout = createInfo.postCreateLayout;
	createInfo.postCreateLayout = ImageLayout::TransferDstOptimal;

	auto imgCopy = GetContext().CreateImage(createInfo);
	if(imgCopy == nullptr)
		return nullptr;
	prosper::util::BlitInfo blitInfo {};
	blitInfo.dstSubresourceLayer.layerCount = blitInfo.srcSubresourceLayer.layerCount = umath::min(imgCopy->GetLayerCount(),GetLayerCount());

	auto numMipmaps = umath::min(imgCopy->GetMipmapCount(),GetMipmapCount());
	for(auto i=decltype(numMipmaps){0u};i<numMipmaps;++i)
	{
		blitInfo.dstSubresourceLayer.mipLevel = blitInfo.srcSubresourceLayer.mipLevel = i;
		if(cmd.RecordBlitImage(blitInfo,*this,*imgCopy) == false)
			return nullptr;
	}
	cmd.RecordImageBarrier(*imgCopy,ImageLayout::TransferDstOptimal,finalLayout);
	return imgCopy;
}
