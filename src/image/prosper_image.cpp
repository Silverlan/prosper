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
#include <sharedutils/scope_guard.h>
#include <util_image.hpp>
#include <util_texture_info.hpp>

using namespace prosper;

#pragma optimize("",off)
IImage::IImage(IPrContext &context,const prosper::util::ImageCreateInfo &createInfo)
	: ContextObject(context),std::enable_shared_from_this<IImage>(),m_createInfo{createInfo}
{}

IImage::~IImage() {}

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


prosper::util::ImageCreateInfo &IImage::GetCreateInfo() {return m_createInfo;}
const prosper::util::ImageCreateInfo &IImage::GetCreateInfo() const {return const_cast<IImage*>(this)->GetCreateInfo();;}

std::shared_ptr<IImage> IImage::Convert(prosper::ICommandBuffer &cmd,Format newFormat)
{
	auto createInfo = GetCreateInfo();
	createInfo.format = newFormat;
	return Copy(cmd,createInfo);
}

bool IImage::IsSrgb() const {return umath::is_flag_set(GetCreateInfo().flags,util::ImageCreateInfo::Flags::Srgb);}
bool IImage::IsNormalMap() const {return umath::is_flag_set(GetCreateInfo().flags,util::ImageCreateInfo::Flags::NormalMap);}
void IImage::SetSrgb(bool srgb) {return umath::set_flag(GetCreateInfo().flags,util::ImageCreateInfo::Flags::Srgb,srgb);}
void IImage::SetNormalMap(bool normalMap) {return umath::set_flag(GetCreateInfo().flags,util::ImageCreateInfo::Flags::NormalMap,normalMap);}

uint32_t IImage::GetPixelSize() const {return util::get_pixel_size(GetFormat());}
uint32_t IImage::GetSize() const
{
	auto numMipmaps = GetMipmapCount();
	auto w = GetWidth();
	auto h = GetHeight();
	DeviceSize size = 0;
	auto pixelSize = GetPixelSize();
	for(auto i=decltype(numMipmaps){0u};i<numMipmaps;++i)
	{
		uint32_t wm,hm;
		util::calculate_mipmap_size(w,h,&wm,&hm,i);
		size += wm *hm *pixelSize;
	}
	return size;
}

static std::optional<uimg::TextureInfo::InputFormat> get_texture_info_input_format(prosper::Format format)
{
	switch(format)
	{
	case prosper::Format::R8G8B8A8_UNorm:
		return uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
	case prosper::Format::R16G16B16A16_SFloat:
		return uimg::TextureInfo::InputFormat::R16G16B16A16_Float;
	case prosper::Format::R32G32B32A32_SFloat:
		return uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
	}
	return {};
}
static std::optional<uimg::TextureInfo::OutputFormat> get_texture_info_output_format(prosper::Format format)
{
	switch(format)
	{
	case prosper::Format::BC1_RGBA_UNorm_Block:
		return uimg::TextureInfo::OutputFormat::BC1a;
	case prosper::Format::BC1_RGB_UNorm_Block:
		return uimg::TextureInfo::OutputFormat::BC1;
	case prosper::Format::BC2_UNorm_Block:
		return uimg::TextureInfo::OutputFormat::BC2;
	case prosper::Format::BC3_UNorm_Block:
		return uimg::TextureInfo::OutputFormat::BC3;
	}
	return {};
}

std::shared_ptr<IImage> IImage::Copy(prosper::ICommandBuffer &cmd,const prosper::util::ImageCreateInfo &copyCreateInfo)
{
	auto &context = GetContext();

	auto createInfo = copyCreateInfo;
	createInfo.usage |= ImageUsageFlags::TransferDstBit;
	auto finalLayout = createInfo.postCreateLayout;
	createInfo.postCreateLayout = ImageLayout::TransferDstOptimal;

	if(util::is_compressed_format(createInfo.format) && (!util::is_compressed_format(GetFormat()) || createInfo.format != GetFormat()))
	{
		// We can't blit into a compressed image, so we'll have to compress the image data ourselves.
		// This is going to be *very* slow.

		using ImageMipmapData = std::vector<uint8_t>;
		using ImageLayerData = std::vector<ImageMipmapData>;
		using ImageData = std::vector<ImageLayerData>;

		auto texInputFormat = get_texture_info_input_format(GetFormat());
		auto texOutputFormat = get_texture_info_output_format(copyCreateInfo.format);
		if(!texInputFormat.has_value() || !texOutputFormat.has_value())
			return nullptr;
		uimg::TextureInfo texInfo {};
		if(IsSrgb())
			texInfo.flags |= uimg::TextureInfo::Flags::SRGB;
		if(IsNormalMap())
			texInfo.SetNormalMap();
		auto numSrcMipmaps = GetMipmapCount();
		auto numDstMipmaps = umath::is_flag_set(copyCreateInfo.flags,util::ImageCreateInfo::Flags::FullMipmapChain) ? util::calculate_mipmap_count(copyCreateInfo.width,copyCreateInfo.height) : 1;
		auto generateMipmaps = (numSrcMipmaps == 1 && numDstMipmaps > 1);
		texInfo.inputFormat = *texInputFormat;
		texInfo.outputFormat = *texOutputFormat;
		if(generateMipmaps)
			texInfo.flags |= uimg::TextureInfo::Flags::GenerateMipmaps;

		ImageData compressedData;
		auto numLayers = GetLayerCount();
		compressedData.resize(numLayers,ImageLayerData(numDstMipmaps));
		auto iLevel = std::numeric_limits<uint32_t>::max();
		auto iMipmap = std::numeric_limits<uint32_t>::max();
		uimg::TextureOutputHandler outputHandler {};
		outputHandler.beginImage = [&compressedData,&iLevel,&iMipmap](int size, int width, int height, int depth, int face, int miplevel) {
			iLevel = face;
			iMipmap = miplevel;
			compressedData[face][miplevel].resize(size);
		};
		outputHandler.writeData = [&compressedData,&iLevel,&iMipmap](const void * data, int size) -> bool {
			if(iLevel == std::numeric_limits<uint32_t>::max() || iMipmap == std::numeric_limits<uint32_t>::max())
				return true;
			memcpy(compressedData[iLevel][iMipmap].data(),data,size);
			return true;
		};
		outputHandler.endImage = [&iLevel,&iMipmap]() {
			iLevel = std::numeric_limits<uint32_t>::max();
			iMipmap = std::numeric_limits<uint32_t>::max();
		};
		auto result = prosper::util::compress_image(*this,texInfo,outputHandler);
		if(result == false)
			return nullptr;
		return GetContext().CreateImage(createInfo,[&compressedData,&createInfo](uint32_t layerId,uint32_t mipmapId,uint32_t &dataSize,uint32_t &rowSize) -> const uint8_t* {
			auto &mipData = compressedData[layerId][mipmapId];
			dataSize = mipData.size();
			assert((dataSize %createInfo.height) == 0);
			rowSize = dataSize /createInfo.height;
			return mipData.data();
		});
		/*auto *memBuf = GetMemoryBuffer();
		if(memBuf == nullptr)
			return nullptr;
		auto szPerPixel = util::get_byte_size(GetFormat());
		auto numLayers = GetLayerCount();
		auto numMipmaps = GetMipmapCount();
		using ImageData = std::vector<uint8_t>;
		using MipmapData = std::vector<ImageData>;
		using LayerData = std::vector<MipmapData>;
		LayerData layerData {};
		layerData.resize(numLayers,MipmapData(numMipmaps,ImageData{}));

		if(!umath::is_flag_set(memBuf->GetCreateInfo().memoryFeatures,prosper::MemoryFeatureFlags::HostAccessable))
		{
			// Memory is not accessable, which means we'll have to copy the data into host-readable memory first
			auto createInfo = copyCreateInfo;
			prosper::IPrimaryCommandBuffer *x;
			x->RecordCopyImageToBuffer();
		}
		std::vector<uint8_t> imageData;
		//memBuf->Read(
		auto cubemap = IsCubemap();
		uimg::compress_texture(outputHandler,[](uint32_t layer,uint32_t mipmap,std::function<void()> &deleter) -> const uint8_t* {
			// TODO
		},createInfo.width,createInfo.height,szPerPixel,numLayers,numMipmaps,cubemap,texInfo,errorHandler);
		*/
	}
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
#pragma optimize("",on)
