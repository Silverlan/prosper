/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_context.hpp"
#include "prosper_util.hpp"
#include "prosper_util_image_buffer.hpp"
#include "image/prosper_render_target.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_descriptor_set_group.hpp"
#include "image/prosper_sampler.hpp"
#include "image/prosper_msaa_texture.hpp"
#include "buffers/vk_buffer.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"
#include "shader/prosper_shader.hpp"
#include "image/prosper_image_view.hpp"
#include "image/vk_image.hpp"
#include "image/vk_image_view.hpp"
#include "buffers/vk_uniform_resizable_buffer.hpp"
#include "buffers/vk_dynamic_resizable_buffer.hpp"
#include <sharedutils/util.h>
#include <config.h>
#include <wrappers/image.h>
#include <wrappers/image_view.h>
#include <wrappers/sampler.h>
#include <wrappers/command_buffer.h>
#include <wrappers/memory_block.h>
#include <wrappers/buffer.h>
#include <wrappers/device.h>
#include <wrappers/physical_device.h>
#include <misc/image_create_info.h>
#include <misc/image_view_create_info.h>
#include <misc/buffer_create_info.h>
#include <misc/framebuffer_create_info.h>
#include <util_image_buffer.hpp>
#include <util_image.hpp>
#include <util_texture_info.hpp>
#include <gli/gli.hpp>

#ifdef DEBUG_VERBOSE
#include <iostream>
#endif

#pragma optimize("",off)
bool prosper::util::RenderPassCreateInfo::AttachmentInfo::operator==(const AttachmentInfo &other) const
{
	return format == other.format &&
		sampleCount == other.sampleCount &&
		loadOp == other.loadOp &&
		storeOp == other.storeOp &&
		initialLayout == other.initialLayout &&
		finalLayout == other.finalLayout;
}
bool prosper::util::RenderPassCreateInfo::AttachmentInfo::operator!=(const AttachmentInfo &other) const {return !operator==(other);}

/////////////////

bool prosper::util::RenderPassCreateInfo::SubPass::operator==(const SubPass &other) const
{
	return colorAttachments == other.colorAttachments &&
		useDepthStencilAttachment == other.useDepthStencilAttachment &&
		dependencies == other.dependencies;
}
bool prosper::util::RenderPassCreateInfo::SubPass::operator!=(const SubPass &other) const {return !operator==(other);}

/////////////////

bool prosper::util::RenderPassCreateInfo::SubPass::Dependency::operator==(const Dependency &other) const
{
	return sourceSubPassId == other.sourceSubPassId &&
		destinationSubPassId == other.destinationSubPassId &&
		sourceStageMask == other.sourceStageMask &&
		destinationStageMask == other.destinationStageMask &&
		sourceAccessMask == other.sourceAccessMask &&
		destinationAccessMask == other.destinationAccessMask;
}
bool prosper::util::RenderPassCreateInfo::SubPass::Dependency::operator!=(const Dependency &other) const {return !operator==(other);}

/////////////////

prosper::util::RenderPassCreateInfo::AttachmentInfo::AttachmentInfo(
	prosper::Format format,prosper::ImageLayout initialLayout,
	prosper::AttachmentLoadOp loadOp,prosper::AttachmentStoreOp storeOp,
	prosper::SampleCountFlags sampleCount,prosper::ImageLayout finalLayout
)
	: format{format},initialLayout{initialLayout},loadOp{loadOp},storeOp{storeOp},
	sampleCount{sampleCount},finalLayout{finalLayout}
{}
bool prosper::util::RenderPassCreateInfo::operator==(const RenderPassCreateInfo &other) const
{
	return attachments == other.attachments && subPasses == other.subPasses;
}
bool prosper::util::RenderPassCreateInfo::operator!=(const RenderPassCreateInfo &other) const {return !operator==(other);}

prosper::util::RenderPassCreateInfo::SubPass::SubPass(const std::vector<std::size_t> &colorAttachments,bool useDepthStencilAttachment,std::vector<Dependency> dependencies)
	: colorAttachments{colorAttachments},useDepthStencilAttachment{useDepthStencilAttachment},dependencies{dependencies}
{}
prosper::util::RenderPassCreateInfo::SubPass::Dependency::Dependency(
	std::size_t sourceSubPassId,std::size_t destinationSubPassId,
	prosper::PipelineStageFlags sourceStageMask,prosper::PipelineStageFlags destinationStageMask,
	prosper::AccessFlags sourceAccessMask,prosper::AccessFlags destinationAccessMask
)
	: sourceSubPassId{sourceSubPassId},destinationSubPassId{destinationSubPassId},
	sourceStageMask{sourceStageMask},destinationStageMask{destinationStageMask},
	sourceAccessMask{sourceAccessMask},destinationAccessMask{destinationAccessMask}
{}
prosper::util::RenderPassCreateInfo::RenderPassCreateInfo(const std::vector<AttachmentInfo> &attachments,const std::vector<SubPass> &subPasses)
	: attachments{attachments},subPasses{subPasses}
{}

static Anvil::QueueFamilyFlags queue_family_flags_to_anvil_queue_family(prosper::QueueFamilyFlags flags)
{
	Anvil::QueueFamilyFlags queueFamilies {};
	if((flags &prosper::QueueFamilyFlags::GraphicsBit) != prosper::QueueFamilyFlags::None)
		queueFamilies = queueFamilies | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT;
	if((flags &prosper::QueueFamilyFlags::ComputeBit) != prosper::QueueFamilyFlags::None)
		queueFamilies = queueFamilies | Anvil::QueueFamilyFlagBits::COMPUTE_BIT;
	if((flags &prosper::QueueFamilyFlags::DMABit) != prosper::QueueFamilyFlags::None)
		queueFamilies = queueFamilies | Anvil::QueueFamilyFlagBits::DMA_BIT;
	return queueFamilies;
}

static Anvil::MemoryFeatureFlags memory_feature_flags_to_anvil_flags(prosper::MemoryFeatureFlags flags)
{
	auto memoryFeatureFlags = Anvil::MemoryFeatureFlags{};
	if((flags &prosper::MemoryFeatureFlags::DeviceLocal) != prosper::MemoryFeatureFlags::None)
		memoryFeatureFlags |= Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT;
	if((flags &prosper::MemoryFeatureFlags::HostCached) != prosper::MemoryFeatureFlags::None)
		memoryFeatureFlags |= Anvil::MemoryFeatureFlagBits::HOST_CACHED_BIT;
	if((flags &prosper::MemoryFeatureFlags::HostCoherent) != prosper::MemoryFeatureFlags::None)
		memoryFeatureFlags |= Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT;
	if((flags &prosper::MemoryFeatureFlags::LazilyAllocated) != prosper::MemoryFeatureFlags::None)
		memoryFeatureFlags |= Anvil::MemoryFeatureFlagBits::LAZILY_ALLOCATED_BIT;
	if((flags &prosper::MemoryFeatureFlags::HostAccessable) != prosper::MemoryFeatureFlags::None)
		memoryFeatureFlags |= Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT;
	return memoryFeatureFlags;
}

std::pair<const Anvil::MemoryType*,prosper::MemoryFeatureFlags> prosper::util::find_compatible_memory_type(Anvil::BaseDevice &dev,MemoryFeatureFlags featureFlags)
{
	auto r = std::pair<const Anvil::MemoryType*,prosper::MemoryFeatureFlags>{nullptr,featureFlags};
	if(umath::to_integral(featureFlags) == 0u)
		return r;
	prosper::MemoryFeatureFlags requiredFeatures {};

    // Convert usage to requiredFlags and preferredFlags.
	switch(featureFlags)
	{
	case prosper::MemoryFeatureFlags::CPUToGPU:
	case prosper::MemoryFeatureFlags::GPUToCPU:
		requiredFeatures |= prosper::MemoryFeatureFlags::HostAccessable;
	}

	auto anvFlags = memory_feature_flags_to_anvil_flags(featureFlags);
	auto anvRequiredFlags = memory_feature_flags_to_anvil_flags(requiredFeatures);
	auto &memProps = dev.get_physical_device_memory_properties();
	for(auto &type : memProps.types)
	{
		if((type.features &anvFlags) == anvFlags)
		{
			// Found preferred flags
			r.first = &type;
			return r;
		}
		if((type.features &anvRequiredFlags) != Anvil::MemoryFeatureFlagBits::NONE)
			r.first = &type; // Found required flags
	}
	r.second = requiredFeatures;
	return r;
}

uint64_t prosper::util::clamp_gpu_memory_size(Anvil::BaseDevice &dev,uint64_t size,float percentageOfGPUMemory,MemoryFeatureFlags featureFlags)
{
	auto r = find_compatible_memory_type(dev,featureFlags);
	if(r.first == nullptr || r.first->heap_ptr == nullptr)
		throw std::runtime_error("Incompatible memory feature flags");
	auto maxMem = std::floorl(r.first->heap_ptr->size *static_cast<long double>(percentageOfGPUMemory));
	return umath::min(size,static_cast<uint64_t>(maxMem));
}

static void find_compatible_memory_feature_flags(Anvil::BaseDevice &dev,prosper::MemoryFeatureFlags &featureFlags)
{
	auto r = prosper::util::find_compatible_memory_type(dev,featureFlags);
	if(r.first == nullptr)
		throw std::runtime_error("Incompatible memory feature flags");
	featureFlags = r.second;
}

std::shared_ptr<prosper::ISampler> prosper::Context::CreateSampler(const util::SamplerCreateInfo &createInfo)
{
	return Sampler::Create(*this,createInfo);
}

std::shared_ptr<prosper::IImageView> prosper::Context::CreateImageView(const prosper::util::ImageViewCreateInfo &createInfo,prosper::IImage &img)
{
	auto format = createInfo.format;
	if(format == Format::Unknown)
		format = img.GetFormat();

	auto aspectMask = util::get_aspect_mask(format);
	auto imgType = img.GetType();
	auto numLayers = createInfo.levelCount;
	auto type = ImageViewType::e2D;
	if(numLayers > 1u && img.IsCubemap())
		type = (numLayers > 6u) ? ImageViewType::CubeArray : ImageViewType::Cube;
	else
	{
		switch(imgType)
		{
		case ImageType::e1D:
			type = (numLayers > 1u) ? ImageViewType::e1DArray : ImageViewType::e1D;
			break;
		case ImageType::e2D:
			type = (numLayers > 1u) ? ImageViewType::e2DArray : ImageViewType::e2D;
			break;
		case ImageType::e3D:
			type = ImageViewType::e3D;
			break;
		}
	}
	switch(type)
	{
	case ImageViewType::e2D:
		return VlkImageView::Create(*this,img,createInfo,type,aspectMask,Anvil::ImageView::create(
			Anvil::ImageViewCreateInfo::create_2D(
				&GetDevice(),&static_cast<VlkImage&>(img).GetAnvilImage(),createInfo.baseLayer.has_value() ? *createInfo.baseLayer : 0u,createInfo.baseMipmap,umath::min(createInfo.baseMipmap +createInfo.mipmapLevels,img.GetMipmapCount()) -createInfo.baseMipmap,
				static_cast<Anvil::ImageAspectFlagBits>(aspectMask),static_cast<Anvil::Format>(format),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleRed),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleGreen),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleBlue),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleAlpha)
			)
		));
	case ImageViewType::Cube:
		return VlkImageView::Create(*this,img,createInfo,type,aspectMask,Anvil::ImageView::create(
			Anvil::ImageViewCreateInfo::create_cube_map(
				&GetDevice(),&static_cast<VlkImage&>(img).GetAnvilImage(),createInfo.baseLayer.has_value() ? *createInfo.baseLayer : 0u,createInfo.baseMipmap,umath::min(createInfo.baseMipmap +createInfo.mipmapLevels,img.GetMipmapCount()) -createInfo.baseMipmap,
				static_cast<Anvil::ImageAspectFlagBits>(aspectMask),static_cast<Anvil::Format>(format),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleRed),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleGreen),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleBlue),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleAlpha)
			)
		));
	case ImageViewType::e2DArray:
		return VlkImageView::Create(*this,img,createInfo,type,aspectMask,Anvil::ImageView::create(
			Anvil::ImageViewCreateInfo::create_2D_array(
				&GetDevice(),&static_cast<VlkImage&>(img).GetAnvilImage(),createInfo.baseLayer.has_value() ? *createInfo.baseLayer : 0u,numLayers,createInfo.baseMipmap,umath::min(createInfo.baseMipmap +createInfo.mipmapLevels,img.GetMipmapCount()) -createInfo.baseMipmap,
				static_cast<Anvil::ImageAspectFlagBits>(aspectMask),static_cast<Anvil::Format>(format),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleRed),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleGreen),
				static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleBlue),static_cast<Anvil::ComponentSwizzle>(createInfo.swizzleAlpha)
			)
		));
	default:
		throw std::invalid_argument("Image view type " +std::to_string(umath::to_integral(type)) +" is currently unsupported!");
	}
}

std::shared_ptr<prosper::IUniformResizableBuffer> prosper::Context::CreateUniformResizableBuffer(
	util::BufferCreateInfo createInfo,uint64_t bufferInstanceSize,
	uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data
)
{
	return util::create_uniform_resizable_buffer(*this,createInfo,bufferInstanceSize,maxTotalSize,clampSizeToAvailableGPUMemoryPercentage,data);
}
std::shared_ptr<prosper::IDynamicResizableBuffer> prosper::Context::CreateDynamicResizableBuffer(
	util::BufferCreateInfo createInfo,
	uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data
)
{
	return util::create_dynamic_resizable_buffer(*this,createInfo,maxTotalSize,clampSizeToAvailableGPUMemoryPercentage,data);
}

std::shared_ptr<prosper::IBuffer> prosper::Context::CreateBuffer(const util::BufferCreateInfo &createInfo,const void *data)
{
	if(createInfo.size == 0ull)
		return nullptr;
	auto sharingMode = Anvil::SharingMode::EXCLUSIVE;
	if((createInfo.flags &util::BufferCreateInfo::Flags::ConcurrentSharing) != util::BufferCreateInfo::Flags::None)
		sharingMode = Anvil::SharingMode::CONCURRENT;

	auto &dev = GetDevice();
	if((createInfo.flags &util::BufferCreateInfo::Flags::Sparse) != util::BufferCreateInfo::Flags::None)
	{
		Anvil::BufferCreateFlags createFlags {Anvil::BufferCreateFlagBits::NONE};
		if((createInfo.flags &util::BufferCreateInfo::Flags::SparseAliasedResidency) != util::BufferCreateInfo::Flags::None)
			createFlags |= Anvil::BufferCreateFlagBits::SPARSE_ALIASED_BIT | Anvil::BufferCreateFlagBits::SPARSE_RESIDENCY_BIT;

		return VlkBuffer::Create(*this,Anvil::Buffer::create(
			Anvil::BufferCreateInfo::create_no_alloc(
				&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
				sharingMode,createFlags,static_cast<Anvil::BufferUsageFlagBits>(createInfo.usageFlags)
			)
		),createInfo,0ull,createInfo.size);
	}
	Anvil::BufferCreateFlags createFlags {Anvil::BufferCreateFlagBits::NONE};
	if((createInfo.flags &util::BufferCreateInfo::Flags::DontAllocateMemory) == util::BufferCreateInfo::Flags::None)
	{
		auto memoryFeatures = createInfo.memoryFeatures;
		find_compatible_memory_feature_flags(dev,memoryFeatures);
		auto bufferCreateInfo = Anvil::BufferCreateInfo::create_alloc(
			&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
			sharingMode,createFlags,static_cast<Anvil::BufferUsageFlagBits>(createInfo.usageFlags),memory_feature_flags_to_anvil_flags(memoryFeatures)
		);
		bufferCreateInfo->set_client_data(data);
		auto r = VlkBuffer::Create(*this,Anvil::Buffer::create(
			std::move(bufferCreateInfo)
		),createInfo,0ull,createInfo.size);
		return r;
	}
	return VlkBuffer::Create(*this,Anvil::Buffer::create(
		Anvil::BufferCreateInfo::create_no_alloc(
			&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
			sharingMode,createFlags,static_cast<Anvil::BufferUsageFlagBits>(createInfo.usageFlags)
		)
	),createInfo,0ull,createInfo.size);
}

static std::shared_ptr<prosper::IImage> create_image(prosper::Context &context,const prosper::util::ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> *data)
{
	auto layers = createInfo.layers;
	auto imageCreateFlags = Anvil::ImageCreateFlags{};
	if((createInfo.flags &prosper::util::ImageCreateInfo::Flags::Cubemap) != prosper::util::ImageCreateInfo::Flags::None)
	{
		imageCreateFlags |= Anvil::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT;
		layers = 6u;
	}

	auto postCreateLayout = createInfo.postCreateLayout;
	if(postCreateLayout == prosper::ImageLayout::ColorAttachmentOptimal && prosper::util::is_depth_format(createInfo.format))
		postCreateLayout = prosper::ImageLayout::DepthStencilAttachmentOptimal;

	auto memoryFeatures = createInfo.memoryFeatures;
	find_compatible_memory_feature_flags(context.GetDevice(),memoryFeatures);
	auto memoryFeatureFlags = memory_feature_flags_to_anvil_flags(memoryFeatures);
	auto queueFamilies = queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask);
	auto sharingMode = Anvil::SharingMode::EXCLUSIVE;
	if((createInfo.flags &prosper::util::ImageCreateInfo::Flags::ConcurrentSharing) != prosper::util::ImageCreateInfo::Flags::None)
		sharingMode = Anvil::SharingMode::CONCURRENT;

	auto useDiscreteMemory = umath::is_flag_set(createInfo.flags,prosper::util::ImageCreateInfo::Flags::AllocateDiscreteMemory);
	if(
		useDiscreteMemory == false &&
		((createInfo.memoryFeatures &prosper::MemoryFeatureFlags::HostAccessable) != prosper::MemoryFeatureFlags::None ||
		(createInfo.memoryFeatures &prosper::MemoryFeatureFlags::DeviceLocal) == prosper::MemoryFeatureFlags::None)
		)
		useDiscreteMemory = true; // Pre-allocated memory currently only supported for device local memory

	auto sparse = (createInfo.flags &prosper::util::ImageCreateInfo::Flags::Sparse) != prosper::util::ImageCreateInfo::Flags::None;
	auto dontAllocateMemory = umath::is_flag_set(createInfo.flags,prosper::util::ImageCreateInfo::Flags::DontAllocateMemory);

	auto bUseFullMipmapChain = (createInfo.flags &prosper::util::ImageCreateInfo::Flags::FullMipmapChain) != prosper::util::ImageCreateInfo::Flags::None;
	if(useDiscreteMemory == false || sparse || dontAllocateMemory)
	{
		if((createInfo.flags &prosper::util::ImageCreateInfo::Flags::SparseAliasedResidency) != prosper::util::ImageCreateInfo::Flags::None)
			imageCreateFlags |= Anvil::ImageCreateFlagBits::SPARSE_ALIASED_BIT | Anvil::ImageCreateFlagBits::SPARSE_RESIDENCY_BIT;
		;
		auto img = prosper::VlkImage::Create(context,Anvil::Image::create(
			Anvil::ImageCreateInfo::create_no_alloc(
				&context.GetDevice(),static_cast<Anvil::ImageType>(createInfo.type),static_cast<Anvil::Format>(createInfo.format),
				static_cast<Anvil::ImageTiling>(createInfo.tiling),static_cast<Anvil::ImageUsageFlagBits>(createInfo.usage),
				createInfo.width,createInfo.height,1u,layers,
				static_cast<Anvil::SampleCountFlagBits>(createInfo.samples),queueFamilies,
				sharingMode,bUseFullMipmapChain,imageCreateFlags,
				static_cast<Anvil::ImageLayout>(postCreateLayout),data
			)
		),createInfo,false);
		if(sparse == false && dontAllocateMemory == false)
			context.AllocateDeviceImageBuffer(*img);
		return img;
	}

	std::cout<<"Allocating SSAO Image "<<createInfo.width<<"x"<<createInfo.height<<","<<umath::to_integral(createInfo.samples)<<","<<bUseFullMipmapChain<<std::endl;
	auto result = prosper::VlkImage::Create(context,Anvil::Image::create(
		Anvil::ImageCreateInfo::create_alloc(
			&context.GetDevice(),static_cast<Anvil::ImageType>(createInfo.type),static_cast<Anvil::Format>(createInfo.format),
			static_cast<Anvil::ImageTiling>(createInfo.tiling),static_cast<Anvil::ImageUsageFlagBits>(createInfo.usage),
			createInfo.width,createInfo.height,1u,layers,
			static_cast<Anvil::SampleCountFlagBits>(createInfo.samples),queueFamilies,
			sharingMode,bUseFullMipmapChain,memoryFeatureFlags,imageCreateFlags,
			static_cast<Anvil::ImageLayout>(postCreateLayout),data
		)
	),createInfo,false);
	std::cout<<"Done!"<<std::endl;
	return result;
}

static std::shared_ptr<prosper::IImage> create_image(prosper::Context &context,std::vector<std::shared_ptr<uimg::ImageBuffer>> &imgBuffers,bool cubemap)
{
	if(imgBuffers.empty() || imgBuffers.size() != (cubemap ? 6 : 1))
		return nullptr;
	auto &img0 = imgBuffers.at(0);
	auto format = img0->GetFormat();
	auto w = img0->GetWidth();
	auto h = img0->GetHeight();
	for(auto &img : imgBuffers)
	{
		if(img->GetFormat() != format || img->GetWidth() != w || img->GetHeight() != h)
			return nullptr;
	}

	std::optional<uimg::ImageBuffer::Format> conversionFormatRequired = {};
	prosper::Format prosperFormat;
	switch(format)
	{
	case uimg::ImageBuffer::Format::RGB8:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA8;
		break;
	case uimg::ImageBuffer::Format::RGB16:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA16;
		break;
	case uimg::ImageBuffer::Format::RGB32:
		conversionFormatRequired = uimg::ImageBuffer::Format::RGBA32;
		break;
	case uimg::ImageBuffer::Format::RGBA8:
		prosperFormat = prosper::Format::R8G8B8A8_UNorm;
		break;
	case uimg::ImageBuffer::Format::RGBA16:
		prosperFormat = prosper::Format::R16G16B16A16_UNorm;
		break;
	case uimg::ImageBuffer::Format::RGBA32:
		prosperFormat = prosper::Format::R32G32B32A32_SFloat;
		break;
	}
	static_assert(umath::to_integral(uimg::ImageBuffer::Format::Count) == 7);
	if(conversionFormatRequired.has_value())
	{
		std::vector<std::shared_ptr<uimg::ImageBuffer>> converted {};
		converted.reserve(imgBuffers.size());
		for(auto &img : imgBuffers)
		{
			auto copy = img->Copy(*conversionFormatRequired);
			converted.push_back(copy);
		}
		return create_image(context,converted,cubemap);
	}
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosperFormat;
	imgCreateInfo.width = w;
	imgCreateInfo.height = h;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
	imgCreateInfo.layers = imgBuffers.size();
	if(cubemap)
		imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;

	auto byteSize = prosper::util::get_byte_size(imgCreateInfo.format);
	std::vector<Anvil::MipmapRawData> layers {};
	layers.reserve(imgBuffers.size());
	for(auto iLayer=decltype(imgBuffers.size()){0u};iLayer<imgBuffers.size();++iLayer)
	{
		auto &imgBuf = imgBuffers.at(iLayer);
		if(cubemap)
		{
			layers.push_back(Anvil::MipmapRawData::create_cube_map_from_uchar_ptr(
				Anvil::ImageAspectFlagBits::COLOR_BIT,iLayer,0,
				static_cast<uint8_t*>(imgBuf->GetData()),imgCreateInfo.width *imgCreateInfo.height *byteSize,imgCreateInfo.width *byteSize
			));
		}
		else
		{
			layers.push_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(
				Anvil::ImageAspectFlagBits::COLOR_BIT,0u,
				static_cast<uint8_t*>(imgBuf->GetData()),imgCreateInfo.width *imgCreateInfo.height *byteSize,imgCreateInfo.width *byteSize
			));
		}
	}
	return context.CreateImage(imgCreateInfo,layers);
}

std::shared_ptr<prosper::IImage> prosper::Context::CreateImage(const util::ImageCreateInfo &createInfo,const uint8_t *data)
{
	if(data == nullptr)
		return ::create_image(*this,createInfo,nullptr);
	auto byteSize = util::get_byte_size(createInfo.format);
	return CreateImage(createInfo,std::vector<Anvil::MipmapRawData>{
		Anvil::MipmapRawData::create_2D_from_uchar_ptr(
			Anvil::ImageAspectFlagBits::COLOR_BIT,0u,
			data,createInfo.width *createInfo.height *byteSize,createInfo.width *byteSize
		)
	});
}
std::shared_ptr<prosper::IImage> prosper::Context::CreateImage(const util::ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> &data)
{
	return ::create_image(*this,createInfo,&data);
}
std::shared_ptr<prosper::IImage> prosper::Context::CreateImage(uimg::ImageBuffer &imgBuffer)
{
	return ::create_image(*this,std::vector<std::shared_ptr<uimg::ImageBuffer>>{imgBuffer.shared_from_this()},false);
}
std::shared_ptr<prosper::IImage> prosper::Context::CreateCubemap(std::array<std::shared_ptr<uimg::ImageBuffer>,6> &imgBuffers)
{
	std::vector<std::shared_ptr<uimg::ImageBuffer>> vImgBuffers {};
	vImgBuffers.reserve(imgBuffers.size());
	for(auto &imgBuf : imgBuffers)
		vImgBuffers.push_back(imgBuf);
	return ::create_image(*this,vImgBuffers,true);
}
std::shared_ptr<prosper::IRenderPass> prosper::Context::CreateRenderPass(std::unique_ptr<Anvil::RenderPassCreateInfo> renderPassInfo)
{
	return prosper::RenderPass::Create(*this,Anvil::RenderPass::create(std::move(renderPassInfo),GetSwapchain().get()));
}
std::shared_ptr<prosper::IRenderPass> prosper::Context::CreateRenderPass(const util::RenderPassCreateInfo &renderPassInfo)
{
	if(renderPassInfo.attachments.empty())
		throw std::logic_error("Attempted to create render pass with 0 attachments, this is not allowed!");
	auto rpInfo = std::make_unique<Anvil::RenderPassCreateInfo>(&GetDevice());
	std::vector<Anvil::RenderPassAttachmentID> attachmentIds;
	attachmentIds.reserve(renderPassInfo.attachments.size());
	auto depthStencilAttId = std::numeric_limits<Anvil::RenderPassAttachmentID>::max();
	for(auto &attInfo : renderPassInfo.attachments)
	{
		Anvil::RenderPassAttachmentID attId;
		if(util::is_depth_format(static_cast<prosper::Format>(attInfo.format)) == false)
		{
			rpInfo->add_color_attachment(
				static_cast<Anvil::Format>(attInfo.format),static_cast<Anvil::SampleCountFlagBits>(attInfo.sampleCount),
				static_cast<Anvil::AttachmentLoadOp>(attInfo.loadOp),static_cast<Anvil::AttachmentStoreOp>(attInfo.storeOp),
				static_cast<Anvil::ImageLayout>(attInfo.initialLayout),static_cast<Anvil::ImageLayout>(attInfo.finalLayout),
				true,&attId
			);
		}
		else
		{
			if(depthStencilAttId != std::numeric_limits<Anvil::RenderPassAttachmentID>::max())
				throw std::logic_error("Attempted to add more than one depth stencil attachment to render pass, this is not allowed!");
			rpInfo->add_depth_stencil_attachment(
				static_cast<Anvil::Format>(attInfo.format),static_cast<Anvil::SampleCountFlagBits>(attInfo.sampleCount),
				static_cast<Anvil::AttachmentLoadOp>(attInfo.loadOp),static_cast<Anvil::AttachmentStoreOp>(attInfo.storeOp),
				Anvil::AttachmentLoadOp::DONT_CARE,Anvil::AttachmentStoreOp::DONT_CARE,
				static_cast<Anvil::ImageLayout>(attInfo.initialLayout),static_cast<Anvil::ImageLayout>(attInfo.finalLayout),
				true,&attId
			);
			depthStencilAttId = attId;
		}
		attachmentIds.push_back(attId);
	}
	if(renderPassInfo.subPasses.empty() == false)
	{
		for(auto &subPassInfo : renderPassInfo.subPasses)
		{
			Anvil::SubPassID subPassId;
			rpInfo->add_subpass(&subPassId);
			auto location = 0u;
			for(auto attId : subPassInfo.colorAttachments)
				rpInfo->add_subpass_color_attachment(subPassId,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,attId,location++,nullptr);
			if(subPassInfo.useDepthStencilAttachment)
			{
				if(depthStencilAttId == std::numeric_limits<Anvil::RenderPassAttachmentID>::max())
					throw std::logic_error("Attempted to add depth stencil attachment sub-pass, but no depth stencil attachment has been specified!");
				rpInfo->add_subpass_depth_stencil_attachment(subPassId,Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,depthStencilAttId);
			}
			for(auto &dependency : subPassInfo.dependencies)
			{
				rpInfo->add_subpass_to_subpass_dependency(
					dependency.destinationSubPassId,dependency.sourceSubPassId,
					static_cast<Anvil::PipelineStageFlagBits>(dependency.destinationStageMask),static_cast<Anvil::PipelineStageFlagBits>(dependency.sourceStageMask),
					static_cast<Anvil::AccessFlagBits>(dependency.destinationAccessMask),static_cast<Anvil::AccessFlagBits>(dependency.sourceAccessMask),
					Anvil::DependencyFlagBits::NONE
				);
			}
		}
	}
	else
	{
		Anvil::SubPassID subPassId;
		rpInfo->add_subpass(&subPassId);

		if(depthStencilAttId != std::numeric_limits<Anvil::RenderPassAttachmentID>::max())
			rpInfo->add_subpass_depth_stencil_attachment(subPassId,Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,depthStencilAttId);

		auto attId = 0u;
		auto location = 0u;
		for(auto &attInfo : renderPassInfo.attachments)
		{
			if(util::is_depth_format(static_cast<prosper::Format>(attInfo.format)) == false)
				rpInfo->add_subpass_color_attachment(subPassId,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,attId,location++,nullptr);
			++attId;
		}
	}
	return CreateRenderPass(std::move(rpInfo));
}
static void init_default_dsg_bindings(Anvil::BaseDevice &dev,Anvil::DescriptorSetGroup &dsg)
{
	// Initialize image sampler bindings with dummy texture
	auto &context = prosper::Context::GetContext(dev);
	auto &dummyTex = context.GetDummyTexture();
	auto &dummyBuf = context.GetDummyBuffer();
	auto numSets = dsg.get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
	{
		auto *descSet = dsg.get_descriptor_set(i);
		auto *descSetLayout = dsg.get_descriptor_set_layout(i);
		auto &info = *descSetLayout->get_create_info();
		auto numBindings = info.get_n_bindings();
		auto bindingIndex = 0u;
		for(auto j=decltype(numBindings){0};j<numBindings;++j)
		{
			Anvil::DescriptorType descType;
			uint32_t arraySize;
			info.get_binding_properties_by_index_number(j,nullptr,&descType,&arraySize,nullptr,nullptr);
			switch(descType)
			{
			case Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER:
			{
				std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
					Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					&static_cast<prosper::VlkImageView&>(*dummyTex->GetImageView()).GetAnvilImageView(),&static_cast<prosper::Sampler&>(*dummyTex->GetSampler()).GetAnvilSampler()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::UNIFORM_BUFFER:
			{
				std::vector<Anvil::DescriptorSet::UniformBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::UniformBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::UNIFORM_BUFFER_DYNAMIC:
			{
				std::vector<Anvil::DescriptorSet::DynamicUniformBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::DynamicUniformBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::STORAGE_BUFFER:
			{
				std::vector<Anvil::DescriptorSet::StorageBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::StorageBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			case Anvil::DescriptorType::STORAGE_BUFFER_DYNAMIC:
			{
				std::vector<Anvil::DescriptorSet::DynamicStorageBufferBindingElement> bindingElements(arraySize,Anvil::DescriptorSet::DynamicStorageBufferBindingElement{
					&dynamic_cast<prosper::VlkBuffer&>(*dummyBuf).GetAnvilBuffer(),0ull,dummyBuf->GetSize()
					});
				descSet->set_binding_array_items(bindingIndex,{0u,arraySize},bindingElements.data());
				break;
			}
			}
			//bindingIndex += arraySize;
			++bindingIndex;
		}
	}
}
std::shared_ptr<prosper::IDescriptorSetGroup> prosper::Context::CreateDescriptorSetGroup(const DescriptorSetInfo &descSetInfo)
{
	return CreateDescriptorSetGroup(std::move(descSetInfo.ToAnvilDescriptorSetInfo()));
}
std::shared_ptr<prosper::IDescriptorSetGroup> prosper::Context::CreateDescriptorSetGroup(std::unique_ptr<Anvil::DescriptorSetCreateInfo> descSetInfo)
{
	std::vector<std::unique_ptr<Anvil::DescriptorSetCreateInfo>> descSetInfos = {};
	descSetInfos.push_back(std::move(descSetInfo));
	auto dsg = Anvil::DescriptorSetGroup::create(&GetDevice(),descSetInfos,Anvil::DescriptorPoolCreateFlagBits::FREE_DESCRIPTOR_SET_BIT);
	init_default_dsg_bindings(GetDevice(),*dsg);
	return prosper::DescriptorSetGroup::Create(*this,std::move(dsg));
}
std::shared_ptr<prosper::IFramebuffer> prosper::Context::CreateFramebuffer(uint32_t width,uint32_t height,uint32_t layers,const std::vector<prosper::IImageView*> &attachments)
{
	auto createInfo = Anvil::FramebufferCreateInfo::create(
		&GetDevice(),width,height,layers
	);
	uint32_t depth = 1u;
	for(auto *att : attachments)
		createInfo->add_attachment(&static_cast<prosper::VlkImageView*>(att)->GetAnvilImageView(),nullptr);
	return prosper::Framebuffer::Create(*this,attachments,width,height,depth,layers,Anvil::Framebuffer::create(
		std::move(createInfo)
	));
}
std::shared_ptr<prosper::Texture> prosper::Context::CreateTexture(
	const util::TextureCreateInfo &createInfo,IImage &img,
	const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo,
	const std::optional<util::SamplerCreateInfo> &samplerCreateInfo
)
{
	auto imageView = createInfo.imageView;
	auto sampler = createInfo.sampler;

	if(imageView == nullptr && imageViewCreateInfo.has_value())
	{
		auto originalLevelCount = const_cast<util::ImageViewCreateInfo&>(*imageViewCreateInfo).levelCount;
		if(img.IsCubemap() && imageViewCreateInfo->baseLayer.has_value() == false)
			const_cast<util::ImageViewCreateInfo&>(*imageViewCreateInfo).levelCount = 6u; // TODO: Do this properly
		imageView = CreateImageView(*imageViewCreateInfo,img);
		const_cast<util::ImageViewCreateInfo&>(*imageViewCreateInfo).levelCount = originalLevelCount;
	}

	std::vector<std::shared_ptr<IImageView>> imageViews = {imageView};
	if((createInfo.flags &util::TextureCreateInfo::Flags::CreateImageViewForEachLayer) != util::TextureCreateInfo::Flags::None)
	{
		if(imageViewCreateInfo.has_value() == false)
			throw std::invalid_argument("If 'CreateImageViewForEachLayer' flag is set, a valid ImageViewCreateInfo struct has to be supplied!");
		auto numLayers = img.GetLayerCount();
		imageViews.reserve(imageViews.size() +numLayers);
		for(auto i=decltype(numLayers){0};i<numLayers;++i)
		{
			auto imgViewCreateInfoLayer = *imageViewCreateInfo;
			imgViewCreateInfoLayer.baseLayer = i;
			imageViews.push_back(CreateImageView(imgViewCreateInfoLayer,img));
		}
	}

	if(sampler == nullptr && samplerCreateInfo.has_value())
		sampler = CreateSampler(*samplerCreateInfo);
	if(img.GetSampleCount() != SampleCountFlags::e1Bit && (createInfo.flags &util::TextureCreateInfo::Flags::Resolvable) != util::TextureCreateInfo::Flags::None)
	{
		if(util::is_depth_format(img.GetFormat()))
			throw std::logic_error("Cannot create resolvable multi-sampled depth image: Multi-sample depth images cannot be resolved!");
		auto extents = img.GetExtents();
		prosper::util::ImageCreateInfo imgCreateInfo {};
		imgCreateInfo.format = img.GetFormat();
		imgCreateInfo.width = extents.width;
		imgCreateInfo.height = extents.height;
		imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		imgCreateInfo.type = img.GetType();
		imgCreateInfo.usage = img.GetUsageFlags() | ImageUsageFlags::TransferDstBit;
		imgCreateInfo.tiling = img.GetTiling();
		imgCreateInfo.layers = img.GetLayerCount();
		imgCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
		auto resolvedImg = CreateImage(imgCreateInfo);
		auto resolvedTexture = CreateTexture({},*resolvedImg,imageViewCreateInfo,samplerCreateInfo);
		return std::shared_ptr<MSAATexture>{new MSAATexture{*this,img,imageViews,sampler.get(),resolvedTexture},[](MSAATexture *msaaTex) {
			msaaTex->OnRelease();
			delete msaaTex;
		}};
	}
	return std::shared_ptr<Texture>(new Texture{*this,img,imageViews,sampler.get()},[](Texture *tex) {
		tex->OnRelease();
		delete tex;
	});
}

std::shared_ptr<prosper::RenderTarget> prosper::Context::CreateRenderTarget(
	const std::vector<std::shared_ptr<Texture>> &textures,const std::shared_ptr<IRenderPass> &rp,const util::RenderTargetCreateInfo &rtCreateInfo
)
{
	if(textures.empty())
		return nullptr;
	auto &tex = textures.front();
	if(tex == nullptr)
		return nullptr;
	auto &context = tex->GetContext();
	auto &img = tex->GetImage();
	auto extents = img.GetExtents();
	auto numLayers = img.GetLayerCount();
	std::vector<std::shared_ptr<IFramebuffer>> framebuffers;
	std::vector<prosper::IImageView*> attachments;
	attachments.reserve(textures.size());
	for(auto &tex : textures)
		attachments.push_back(tex->GetImageView());
	framebuffers.push_back(context.CreateFramebuffer(extents.width,extents.height,1u,attachments));
	if(rtCreateInfo.useLayerFramebuffers == true)
	{
		framebuffers.reserve(framebuffers.size() +numLayers);
		for(auto i=decltype(numLayers){0};i<numLayers;++i)
		{
			attachments = {tex->GetImageView(i)};
			framebuffers.push_back(context.CreateFramebuffer(extents.width,extents.height,1u,attachments));
		}
	}
	return std::shared_ptr<RenderTarget>{new RenderTarget{context,textures,framebuffers,*rp},[](RenderTarget *rt) {
		rt->OnRelease();
		delete rt;
	}};
}
std::shared_ptr<prosper::RenderTarget> prosper::Context::CreateRenderTarget(
	Texture &texture,IImageView &imgView,IRenderPass &rp,const util::RenderTargetCreateInfo &rtCreateInfo)
{
	auto &img = imgView.GetImage();
	auto &context = rp.GetContext();
	auto extents = img.GetExtents();
	std::vector<prosper::IImageView*> attachments = {&imgView};
	std::vector<std::shared_ptr<IFramebuffer>> framebuffers;
	framebuffers.push_back(context.CreateFramebuffer(extents.width,extents.height,1u,attachments));
	return std::shared_ptr<RenderTarget>{new RenderTarget{context,std::vector<std::shared_ptr<Texture>>{texture.shared_from_this()},framebuffers,rp},[](RenderTarget *rt) {
		rt->OnRelease();
		delete rt;
	}};
}

prosper::Format prosper::util::get_vk_format(uimg::ImageBuffer::Format format)
{
	switch(format)
	{
	case uimg::ImageBuffer::Format::RGB8:
		return prosper::Format::R8G8B8_UNorm_PoorCoverage;
	case uimg::ImageBuffer::Format::RGBA8:
		return prosper::Format::R8G8B8A8_UNorm;
	case uimg::ImageBuffer::Format::RGB16:
		return prosper::Format::R16G16B16_UNorm_PoorCoverage;
	case uimg::ImageBuffer::Format::RGBA16:
		return prosper::Format::R16G16B16A16_UNorm;
	case uimg::ImageBuffer::Format::RGB32:
		return prosper::Format::R32G32B32_SFloat;
	case uimg::ImageBuffer::Format::RGBA32:
		return prosper::Format::R32G32B32A32_SFloat;
	}
	static_assert(umath::to_integral(uimg::ImageBuffer::Format::Count) == 7);
	return prosper::Format::Unknown;
}

void prosper::util::initialize_image(Anvil::BaseDevice &dev,const uimg::ImageBuffer &imgSrc,IImage &img)
{
	auto extents = img.GetExtents();
	auto w = extents.width;
	auto h = extents.height;
	auto srcFormat = img.GetFormat();
	if(srcFormat != Format::R8G8B8A8_UNorm && srcFormat != Format::R8G8B8_UNorm_PoorCoverage)
		throw std::invalid_argument("Invalid image format for tga target image: Only VK_FORMAT_R8G8B8A8_UNORM and VK_FORMAT_R8G8B8_UNORM are supported!");
	if(imgSrc.GetWidth() != w || imgSrc.GetHeight() != h)
		throw std::invalid_argument("Invalid image extents for tga target image: Extents must be the same for source tga image and target image!");

	auto memBlock = static_cast<VlkImage&>(img)->get_memory_block();
	auto size = w *h *get_byte_size(srcFormat);
	uint8_t *outDataPtr = nullptr;
	memBlock->map(0ull,size,reinterpret_cast<void**>(&outDataPtr));

	auto *imgData = static_cast<const uint8_t*>(imgSrc.GetData());
	auto pos = decltype(imgSrc.GetSize()){0};
	auto tgaFormat = imgSrc.GetFormat();
	auto srcPxSize = get_byte_size(srcFormat);
	auto rowPitch = w *srcPxSize;
	for(auto y=h;y>0;)
	{
		--y;
		auto *row = outDataPtr +y *rowPitch;
		for(auto x=decltype(w){0};x<w;++x)
		{
			auto *px = row;
			px[0] = imgData[pos];
			px[1] = imgData[pos +1];
			px[2] = imgData[pos +2];
			switch(tgaFormat)
			{
			case uimg::ImageBuffer::Format::RGBA8:
				{
					pos += 4;
					if(srcFormat == Format::R8G8B8A8_UNorm)
						px[3] = imgData[pos +3];
					break;
				}
				default:
				{
					if(srcFormat == Format::R8G8B8A8_UNorm)
						px[3] = std::numeric_limits<uint8_t>::max();
					pos += 3;
					break;
				}
			}
			row += srcPxSize;
		}
	}

	memBlock->unmap();
}

uint32_t prosper::util::calculate_buffer_alignment(Anvil::BaseDevice &dev,BufferUsageFlags usageFlags)
{
	auto alignment = 0u;
	if((usageFlags &BufferUsageFlags::UniformBufferBit) != BufferUsageFlags::None)
		alignment = dev.get_physical_device_properties().core_vk1_0_properties_ptr->limits.min_uniform_buffer_offset_alignment;
	if((usageFlags &BufferUsageFlags::StorageBufferBit) != BufferUsageFlags::None)
	{
		alignment = umath::get_least_common_multiple(
			static_cast<vk::DeviceSize>(alignment),
			dev.get_physical_device_properties().core_vk1_0_properties_ptr->limits.min_storage_buffer_offset_alignment
		);
	}
	return alignment;
}

void prosper::util::calculate_mipmap_size(uint32_t w,uint32_t h,uint32_t *wMipmap,uint32_t *hMipmap,uint32_t level)
{
	*wMipmap = w;
	*hMipmap = h;
	int scale = static_cast<int>(std::pow(2,level));
	*wMipmap /= scale;
	*hMipmap /= scale;
	if(*wMipmap == 0)
		*wMipmap = 1;
	if(*hMipmap == 0)
		*hMipmap = 1;
}

uint32_t prosper::util::calculate_mipmap_size(uint32_t v,uint32_t level)
{
	auto r = v;
	auto scale = static_cast<int>(std::pow(2,level));
	r /= scale;
	if(r == 0)
		r = 1;
	return r;
}

uint32_t prosper::util::calculate_mipmap_count(uint32_t w,uint32_t h) {return 1 +static_cast<uint32_t>(floor(log2(fmaxf(static_cast<float>(w),static_cast<float>(h)))));}
std::string prosper::util::to_string(vk::Result r) {return vk::to_string(r);}

std::string prosper::util::to_string(vk::DebugReportFlagsEXT flags)
{
	auto values = umath::get_power_of_2_values(static_cast<uint32_t>(flags));
	std::string r;
	for(auto it=values.begin();it!=values.end();++it)
	{
		if(it != values.begin())
			r += " | ";
		r += prosper::util::to_string(static_cast<vk::DebugReportFlagBitsEXT>(*it));
	}
	return r;
}
std::string prosper::util::to_string(vk::DebugReportObjectTypeEXT type) {return vk::to_string(type);}
std::string prosper::util::to_string(vk::DebugReportFlagBitsEXT flags) {return vk::to_string(flags);}
std::string prosper::util::to_string(prosper::Format format) {return vk::to_string(static_cast<vk::Format>(format));}
std::string prosper::util::to_string(prosper::ShaderStageFlags shaderStage) {return vk::to_string(static_cast<vk::ShaderStageFlagBits>(shaderStage));}
std::string prosper::util::to_string(prosper::ImageUsageFlags usage)
{
	auto values = umath::get_power_of_2_values(static_cast<uint32_t>(static_cast<vk::ImageUsageFlagBits>(usage)));
	std::string r;
	for(auto it=values.begin();it!=values.end();++it)
	{
		if(it != values.begin())
			r += " | ";
		r += vk::to_string(static_cast<vk::ImageUsageFlagBits>(*it));
	}
	return r;
}
std::string prosper::util::to_string(prosper::ImageCreateFlags createFlags)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(createFlags));
	std::string r;
	for(auto it=values.begin();it!=values.end();++it)
	{
		if(it != values.begin())
			r += " | ";
		r += vk::to_string(static_cast<vk::ImageCreateFlagBits>(*it));
	}
	return r;
}
std::string prosper::util::to_string(prosper::ImageType type) {return vk::to_string(static_cast<vk::ImageType>(type));}
std::string prosper::util::to_string(prosper::ShaderStage stage)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(stage));
	std::stringstream ss;
	auto bFirst = true;
	for(auto &v : values)
	{
		if(bFirst == false)
			ss<<" | ";
		else
			bFirst = false;
		ss<<vk::to_string(static_cast<vk::ShaderStageFlagBits>(v));
	}
	return ss.str();
}
std::string prosper::util::to_string(prosper::PipelineStageFlags stage) {return vk::to_string(static_cast<vk::PipelineStageFlagBits>(stage));}
std::string prosper::util::to_string(prosper::ImageTiling tiling) {return vk::to_string(static_cast<vk::ImageTiling>(tiling));}
std::string prosper::util::to_string(vk::PhysicalDeviceType type) {return vk::to_string(type);}

bool prosper::util::has_alpha(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
		case vk::Format::eBc1RgbaUnormBlock:
		case vk::Format::eBc1RgbaSrgbBlock:
		case vk::Format::eBc2UnormBlock:
		case vk::Format::eBc2SrgbBlock:
		case vk::Format::eBc3UnormBlock:
		case vk::Format::eBc3SrgbBlock:
		case vk::Format::eEtc2R8G8B8A1UnormBlock:
		case vk::Format::eEtc2R8G8B8A1SrgbBlock:
		case vk::Format::eEtc2R8G8B8A8UnormBlock:
		case vk::Format::eEtc2R8G8B8A8SrgbBlock:
		case vk::Format::eR4G4B4A4UnormPack16:
		case vk::Format::eB4G4R4A4UnormPack16:
		case vk::Format::eR5G5B5A1UnormPack16:
		case vk::Format::eB5G5R5A1UnormPack16:
		case vk::Format::eA1R5G5B5UnormPack16:
		case vk::Format::eR8G8B8A8Unorm:
		case vk::Format::eR8G8B8A8Snorm:
		case vk::Format::eR8G8B8A8Uscaled:
		case vk::Format::eR8G8B8A8Sscaled:
		case vk::Format::eR8G8B8A8Uint:
		case vk::Format::eR8G8B8A8Sint:
		case vk::Format::eR8G8B8A8Srgb:
		case vk::Format::eB8G8R8A8Unorm:
		case vk::Format::eB8G8R8A8Snorm:
		case vk::Format::eB8G8R8A8Uscaled:
		case vk::Format::eB8G8R8A8Sscaled:
		case vk::Format::eB8G8R8A8Uint:
		case vk::Format::eB8G8R8A8Sint:
		case vk::Format::eB8G8R8A8Srgb:
		case vk::Format::eA8B8G8R8UnormPack32:
		case vk::Format::eA8B8G8R8SnormPack32:
		case vk::Format::eA8B8G8R8UscaledPack32:
		case vk::Format::eA8B8G8R8SscaledPack32:
		case vk::Format::eA8B8G8R8UintPack32:
		case vk::Format::eA8B8G8R8SintPack32:
		case vk::Format::eA8B8G8R8SrgbPack32:
		case vk::Format::eA2R10G10B10UnormPack32:
		case vk::Format::eA2R10G10B10SnormPack32:
		case vk::Format::eA2R10G10B10UscaledPack32:
		case vk::Format::eA2R10G10B10SscaledPack32:
		case vk::Format::eA2R10G10B10UintPack32:
		case vk::Format::eA2R10G10B10SintPack32:
		case vk::Format::eA2B10G10R10UnormPack32:
		case vk::Format::eA2B10G10R10SnormPack32:
		case vk::Format::eA2B10G10R10UscaledPack32:
		case vk::Format::eA2B10G10R10SscaledPack32:
		case vk::Format::eA2B10G10R10UintPack32:
		case vk::Format::eA2B10G10R10SintPack32:
		case vk::Format::eR16G16B16A16Unorm:
		case vk::Format::eR16G16B16A16Snorm:
		case vk::Format::eR16G16B16A16Uscaled:
		case vk::Format::eR16G16B16A16Sscaled:
		case vk::Format::eR16G16B16A16Uint:
		case vk::Format::eR16G16B16A16Sint:
		case vk::Format::eR16G16B16A16Sfloat:
		case vk::Format::eR32G32B32A32Uint:
		case vk::Format::eR32G32B32A32Sint:
		case vk::Format::eR32G32B32A32Sfloat:
		case vk::Format::eR64G64B64A64Uint:
		case vk::Format::eR64G64B64A64Sint:
		case vk::Format::eR64G64B64A64Sfloat:
		case vk::Format::eBc7UnormBlock:
		case vk::Format::eBc7SrgbBlock:
		case vk::Format::eAstc4x4UnormBlock:
		case vk::Format::eAstc4x4SrgbBlock:
		case vk::Format::eAstc5x4UnormBlock:
		case vk::Format::eAstc5x4SrgbBlock:
		case vk::Format::eAstc5x5UnormBlock:
		case vk::Format::eAstc5x5SrgbBlock:
		case vk::Format::eAstc6x5UnormBlock:
		case vk::Format::eAstc6x5SrgbBlock:
		case vk::Format::eAstc6x6UnormBlock:
		case vk::Format::eAstc6x6SrgbBlock:
		case vk::Format::eAstc8x5UnormBlock:
		case vk::Format::eAstc8x5SrgbBlock:
		case vk::Format::eAstc8x6UnormBlock:
		case vk::Format::eAstc8x6SrgbBlock:
		case vk::Format::eAstc8x8UnormBlock:
		case vk::Format::eAstc8x8SrgbBlock:
		case vk::Format::eAstc10x5UnormBlock:
		case vk::Format::eAstc10x5SrgbBlock:
		case vk::Format::eAstc10x6UnormBlock:
		case vk::Format::eAstc10x6SrgbBlock:
		case vk::Format::eAstc10x8UnormBlock:
		case vk::Format::eAstc10x8SrgbBlock:
		case vk::Format::eAstc10x10UnormBlock:
		case vk::Format::eAstc10x10SrgbBlock:
		case vk::Format::eAstc12x10UnormBlock:
		case vk::Format::eAstc12x10SrgbBlock:
		case vk::Format::eAstc12x12UnormBlock:
		case vk::Format::eAstc12x12SrgbBlock:
			return true;
	}
	return false;
}

bool prosper::util::is_depth_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
		case vk::Format::eD16Unorm:
		case vk::Format::eD16UnormS8Uint:
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD32Sfloat:
		case vk::Format::eD32SfloatS8Uint:
		case vk::Format::eX8D24UnormPack32:
			return true;
	};
	return false;
}

bool prosper::util::is_compressed_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
		case vk::Format::eBc1RgbUnormBlock:
		case vk::Format::eBc1RgbSrgbBlock:
		case vk::Format::eBc1RgbaUnormBlock:
		case vk::Format::eBc1RgbaSrgbBlock:
		case vk::Format::eBc2UnormBlock:
		case vk::Format::eBc2SrgbBlock:
		case vk::Format::eBc3UnormBlock:
		case vk::Format::eBc3SrgbBlock:
		case vk::Format::eBc4UnormBlock:
		case vk::Format::eBc4SnormBlock:
		case vk::Format::eBc5UnormBlock:
		case vk::Format::eBc5SnormBlock:
		case vk::Format::eBc6HUfloatBlock:
		case vk::Format::eBc6HSfloatBlock:
		case vk::Format::eBc7UnormBlock:
		case vk::Format::eBc7SrgbBlock:
		case vk::Format::eEtc2R8G8B8UnormBlock:
		case vk::Format::eEtc2R8G8B8SrgbBlock:
		case vk::Format::eEtc2R8G8B8A1UnormBlock:
		case vk::Format::eEtc2R8G8B8A1SrgbBlock:
		case vk::Format::eEtc2R8G8B8A8UnormBlock:
		case vk::Format::eEtc2R8G8B8A8SrgbBlock:
		case vk::Format::eEacR11UnormBlock:
		case vk::Format::eEacR11SnormBlock:
		case vk::Format::eEacR11G11UnormBlock:
		case vk::Format::eEacR11G11SnormBlock:
		case vk::Format::eAstc4x4UnormBlock:
		case vk::Format::eAstc4x4SrgbBlock:
		case vk::Format::eAstc5x4UnormBlock:
		case vk::Format::eAstc5x4SrgbBlock:
		case vk::Format::eAstc5x5UnormBlock:
		case vk::Format::eAstc5x5SrgbBlock:
		case vk::Format::eAstc6x5UnormBlock:
		case vk::Format::eAstc6x5SrgbBlock:
		case vk::Format::eAstc6x6UnormBlock:
		case vk::Format::eAstc6x6SrgbBlock:
		case vk::Format::eAstc8x5UnormBlock:
		case vk::Format::eAstc8x5SrgbBlock:
		case vk::Format::eAstc8x6UnormBlock:
		case vk::Format::eAstc8x6SrgbBlock:
		case vk::Format::eAstc8x8UnormBlock:
		case vk::Format::eAstc8x8SrgbBlock:
		case vk::Format::eAstc10x5UnormBlock:
		case vk::Format::eAstc10x5SrgbBlock:
		case vk::Format::eAstc10x6UnormBlock:
		case vk::Format::eAstc10x6SrgbBlock:
		case vk::Format::eAstc10x8UnormBlock:
		case vk::Format::eAstc10x8SrgbBlock:
		case vk::Format::eAstc10x10UnormBlock:
		case vk::Format::eAstc10x10SrgbBlock:
		case vk::Format::eAstc12x10UnormBlock:
		case vk::Format::eAstc12x10SrgbBlock:
		case vk::Format::eAstc12x12UnormBlock:
		case vk::Format::eAstc12x12SrgbBlock:
			return true;
	};
	return false;
}
bool prosper::util::is_uncompressed_format(Format format)
{
	if(format == Format::Unknown)
		return false;
	return !is_compressed_format(format);
}

uint32_t prosper::util::get_bit_size(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
		// Sub-Byte formats
		case vk::Format::eUndefined:
			return 0;
		case vk::Format::eR4G4UnormPack8:
			return 8;
		case vk::Format::eR4G4B4A4UnormPack16:
		case vk::Format::eB4G4R4A4UnormPack16:
		case vk::Format::eR5G6B5UnormPack16:
		case vk::Format::eB5G6R5UnormPack16:
		case vk::Format::eR5G5B5A1UnormPack16:
		case vk::Format::eB5G5R5A1UnormPack16:
		case vk::Format::eA1R5G5B5UnormPack16:
			return 16;
		//
		case vk::Format::eR8Unorm:
		case vk::Format::eR8Snorm:
		case vk::Format::eR8Uscaled:
		case vk::Format::eR8Sscaled:
		case vk::Format::eR8Uint:
		case vk::Format::eR8Sint:
		case vk::Format::eR8Srgb:
			return 8;
		case vk::Format::eR8G8Unorm:
		case vk::Format::eR8G8Snorm:
		case vk::Format::eR8G8Uscaled:
		case vk::Format::eR8G8Sscaled:
		case vk::Format::eR8G8Uint:
		case vk::Format::eR8G8Sint:
		case vk::Format::eR8G8Srgb:
			return 16;
		case vk::Format::eR8G8B8Unorm:
		case vk::Format::eR8G8B8Snorm:
		case vk::Format::eR8G8B8Uscaled:
		case vk::Format::eR8G8B8Sscaled:
		case vk::Format::eR8G8B8Uint:
		case vk::Format::eR8G8B8Sint:
		case vk::Format::eR8G8B8Srgb:
		case vk::Format::eB8G8R8Unorm:
		case vk::Format::eB8G8R8Snorm:
		case vk::Format::eB8G8R8Uscaled:
		case vk::Format::eB8G8R8Sscaled:
		case vk::Format::eB8G8R8Uint:
		case vk::Format::eB8G8R8Sint:
		case vk::Format::eB8G8R8Srgb:
			return 24;
		case vk::Format::eR8G8B8A8Unorm:
		case vk::Format::eR8G8B8A8Snorm:
		case vk::Format::eR8G8B8A8Uscaled:
		case vk::Format::eR8G8B8A8Sscaled:
		case vk::Format::eR8G8B8A8Uint:
		case vk::Format::eR8G8B8A8Sint:
		case vk::Format::eR8G8B8A8Srgb:
		case vk::Format::eB8G8R8A8Unorm:
		case vk::Format::eB8G8R8A8Snorm:
		case vk::Format::eB8G8R8A8Uscaled:
		case vk::Format::eB8G8R8A8Sscaled:
		case vk::Format::eB8G8R8A8Uint:
		case vk::Format::eB8G8R8A8Sint:
		case vk::Format::eB8G8R8A8Srgb:
		case vk::Format::eA8B8G8R8UnormPack32:
		case vk::Format::eA8B8G8R8SnormPack32:
		case vk::Format::eA8B8G8R8UscaledPack32:
		case vk::Format::eA8B8G8R8SscaledPack32:
		case vk::Format::eA8B8G8R8UintPack32:
		case vk::Format::eA8B8G8R8SintPack32:
		case vk::Format::eA8B8G8R8SrgbPack32:
			return 32;
		case vk::Format::eA2R10G10B10UnormPack32:
		case vk::Format::eA2R10G10B10SnormPack32:
		case vk::Format::eA2R10G10B10UscaledPack32:
		case vk::Format::eA2R10G10B10SscaledPack32:
		case vk::Format::eA2R10G10B10UintPack32:
		case vk::Format::eA2R10G10B10SintPack32:
		case vk::Format::eA2B10G10R10UnormPack32:
		case vk::Format::eA2B10G10R10SnormPack32:
		case vk::Format::eA2B10G10R10UscaledPack32:
		case vk::Format::eA2B10G10R10SscaledPack32:
		case vk::Format::eA2B10G10R10UintPack32:
		case vk::Format::eA2B10G10R10SintPack32:
			return 32;
		case vk::Format::eR16Unorm:
		case vk::Format::eR16Snorm:
		case vk::Format::eR16Uscaled:
		case vk::Format::eR16Sscaled:
		case vk::Format::eR16Uint:
		case vk::Format::eR16Sint:
		case vk::Format::eR16Sfloat:
			return 16;
		case vk::Format::eR16G16Unorm:
		case vk::Format::eR16G16Snorm:
		case vk::Format::eR16G16Uscaled:
		case vk::Format::eR16G16Sscaled:
		case vk::Format::eR16G16Uint:
		case vk::Format::eR16G16Sint:
		case vk::Format::eR16G16Sfloat:
			return 32;
		case vk::Format::eR16G16B16Unorm:
		case vk::Format::eR16G16B16Snorm:
		case vk::Format::eR16G16B16Uscaled:
		case vk::Format::eR16G16B16Sscaled:
		case vk::Format::eR16G16B16Uint:
		case vk::Format::eR16G16B16Sint:
		case vk::Format::eR16G16B16Sfloat:
			return 48;
		case vk::Format::eR16G16B16A16Unorm:
		case vk::Format::eR16G16B16A16Snorm:
		case vk::Format::eR16G16B16A16Uscaled:
		case vk::Format::eR16G16B16A16Sscaled:
		case vk::Format::eR16G16B16A16Uint:
		case vk::Format::eR16G16B16A16Sint:
		case vk::Format::eR16G16B16A16Sfloat:
			return 64;
		case vk::Format::eR32Uint:
		case vk::Format::eR32Sint:
		case vk::Format::eR32Sfloat:
			return 32;
		case vk::Format::eR32G32Uint:
		case vk::Format::eR32G32Sint:
		case vk::Format::eR32G32Sfloat:
			return 64;
		case vk::Format::eR32G32B32Uint:
		case vk::Format::eR32G32B32Sint:
		case vk::Format::eR32G32B32Sfloat:
			return 96;
		case vk::Format::eR32G32B32A32Uint:
		case vk::Format::eR32G32B32A32Sint:
		case vk::Format::eR32G32B32A32Sfloat:
			return 128;
		case vk::Format::eR64Uint:
		case vk::Format::eR64Sint:
		case vk::Format::eR64Sfloat:
			return 64;
		case vk::Format::eR64G64Uint:
		case vk::Format::eR64G64Sint:
		case vk::Format::eR64G64Sfloat:
			return 128;
		case vk::Format::eR64G64B64Uint:
		case vk::Format::eR64G64B64Sint:
		case vk::Format::eR64G64B64Sfloat:
			return 192;
		case vk::Format::eR64G64B64A64Uint:
		case vk::Format::eR64G64B64A64Sint:
		case vk::Format::eR64G64B64A64Sfloat:
			return 256;
		case vk::Format::eB10G11R11UfloatPack32:
		case vk::Format::eE5B9G9R9UfloatPack32:
			return 32;
		case vk::Format::eD16Unorm:
			return 16;
		case vk::Format::eX8D24UnormPack32:
			return 32;
		case vk::Format::eD32Sfloat:
			return 32;
		case vk::Format::eS8Uint:
			return 8;
		case vk::Format::eD16UnormS8Uint:
			return 16;
		case vk::Format::eD24UnormS8Uint:
			return 24;
		case vk::Format::eD32SfloatS8Uint:
			return 32;
	};
	return 0;
}

bool prosper::util::is_8bit_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
	case vk::Format::eR8Unorm:
	case vk::Format::eR8Snorm:
	case vk::Format::eR8Uscaled:
	case vk::Format::eR8Sscaled:
	case vk::Format::eR8Uint:
	case vk::Format::eR8Sint:
	case vk::Format::eR8Srgb:
	case vk::Format::eR8G8Unorm:
	case vk::Format::eR8G8Snorm:
	case vk::Format::eR8G8Uscaled:
	case vk::Format::eR8G8Sscaled:
	case vk::Format::eR8G8Uint:
	case vk::Format::eR8G8Sint:
	case vk::Format::eR8G8Srgb:
	case vk::Format::eR8G8B8Unorm:
	case vk::Format::eR8G8B8Snorm:
	case vk::Format::eR8G8B8Uscaled:
	case vk::Format::eR8G8B8Sscaled:
	case vk::Format::eR8G8B8Uint:
	case vk::Format::eR8G8B8Sint:
	case vk::Format::eR8G8B8Srgb:
	case vk::Format::eB8G8R8Unorm:
	case vk::Format::eB8G8R8Snorm:
	case vk::Format::eB8G8R8Uscaled:
	case vk::Format::eB8G8R8Sscaled:
	case vk::Format::eB8G8R8Uint:
	case vk::Format::eB8G8R8Sint:
	case vk::Format::eB8G8R8Srgb:
	case vk::Format::eR8G8B8A8Unorm:
	case vk::Format::eR8G8B8A8Snorm:
	case vk::Format::eR8G8B8A8Uscaled:
	case vk::Format::eR8G8B8A8Sscaled:
	case vk::Format::eR8G8B8A8Uint:
	case vk::Format::eR8G8B8A8Sint:
	case vk::Format::eR8G8B8A8Srgb:
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eB8G8R8A8Snorm:
	case vk::Format::eB8G8R8A8Uscaled:
	case vk::Format::eB8G8R8A8Sscaled:
	case vk::Format::eB8G8R8A8Uint:
	case vk::Format::eB8G8R8A8Sint:
	case vk::Format::eB8G8R8A8Srgb:
	case vk::Format::eA8B8G8R8UnormPack32:
	case vk::Format::eA8B8G8R8SnormPack32:
	case vk::Format::eA8B8G8R8UscaledPack32:
	case vk::Format::eA8B8G8R8SscaledPack32:
	case vk::Format::eA8B8G8R8UintPack32:
	case vk::Format::eA8B8G8R8SintPack32:
	case vk::Format::eA8B8G8R8SrgbPack32:
	case vk::Format::eS8Uint:
		return true;
	};
	return false;
}
bool prosper::util::is_16bit_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
	case vk::Format::eR16Unorm:
	case vk::Format::eR16Snorm:
	case vk::Format::eR16Uscaled:
	case vk::Format::eR16Sscaled:
	case vk::Format::eR16Uint:
	case vk::Format::eR16Sint:
	case vk::Format::eR16Sfloat:
	case vk::Format::eR16G16Unorm:
	case vk::Format::eR16G16Snorm:
	case vk::Format::eR16G16Uscaled:
	case vk::Format::eR16G16Sscaled:
	case vk::Format::eR16G16Uint:
	case vk::Format::eR16G16Sint:
	case vk::Format::eR16G16Sfloat:
	case vk::Format::eR16G16B16Unorm:
	case vk::Format::eR16G16B16Snorm:
	case vk::Format::eR16G16B16Uscaled:
	case vk::Format::eR16G16B16Sscaled:
	case vk::Format::eR16G16B16Uint:
	case vk::Format::eR16G16B16Sint:
	case vk::Format::eR16G16B16Sfloat:
	case vk::Format::eR16G16B16A16Unorm:
	case vk::Format::eR16G16B16A16Snorm:
	case vk::Format::eR16G16B16A16Uscaled:
	case vk::Format::eR16G16B16A16Sscaled:
	case vk::Format::eR16G16B16A16Uint:
	case vk::Format::eR16G16B16A16Sint:
	case vk::Format::eR16G16B16A16Sfloat:
	case vk::Format::eD16Unorm:
	case vk::Format::eD16UnormS8Uint:
		return true;
	};
	return false;
}
bool prosper::util::is_32bit_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
	case vk::Format::eR16Unorm:
	case vk::Format::eR16Snorm:
	case vk::Format::eR16Uscaled:
	case vk::Format::eR16Sscaled:
	case vk::Format::eR16Uint:
	case vk::Format::eR16Sint:
	case vk::Format::eR16Sfloat:
	case vk::Format::eR16G16Unorm:
	case vk::Format::eR16G16Snorm:
	case vk::Format::eR16G16Uscaled:
	case vk::Format::eR16G16Sscaled:
	case vk::Format::eR16G16Uint:
	case vk::Format::eR16G16Sint:
	case vk::Format::eR16G16Sfloat:
	case vk::Format::eR16G16B16Unorm:
	case vk::Format::eR16G16B16Snorm:
	case vk::Format::eR16G16B16Uscaled:
	case vk::Format::eR16G16B16Sscaled:
	case vk::Format::eR16G16B16Uint:
	case vk::Format::eR16G16B16Sint:
	case vk::Format::eR16G16B16Sfloat:
	case vk::Format::eR16G16B16A16Unorm:
	case vk::Format::eR16G16B16A16Snorm:
	case vk::Format::eR16G16B16A16Uscaled:
	case vk::Format::eR16G16B16A16Sscaled:
	case vk::Format::eR16G16B16A16Uint:
	case vk::Format::eR16G16B16A16Sint:
	case vk::Format::eR16G16B16A16Sfloat:
	case vk::Format::eD16Unorm:
	case vk::Format::eD16UnormS8Uint:
		return true;
	};
	return false;
}
bool prosper::util::is_64bit_format(Format format)
{
	switch(static_cast<vk::Format>(format))
	{
	case vk::Format::eR64Uint:
	case vk::Format::eR64Sint:
	case vk::Format::eR64Sfloat:
	case vk::Format::eR64G64Uint:
	case vk::Format::eR64G64Sint:
	case vk::Format::eR64G64Sfloat:
	case vk::Format::eR64G64B64Uint:
	case vk::Format::eR64G64B64Sint:
	case vk::Format::eR64G64B64Sfloat:
	case vk::Format::eR64G64B64A64Uint:
	case vk::Format::eR64G64B64A64Sint:
	case vk::Format::eR64G64B64A64Sfloat:
		return true;
	};
	return false;
}

uint32_t prosper::util::get_byte_size(Format format)
{
	auto numBits = get_bit_size(format);
	if(numBits == 0 || (numBits %8) != 0)
		return 0;
	return numBits /8;
}

Anvil::AccessFlags prosper::util::get_read_access_mask() {return Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::HOST_READ_BIT | Anvil::AccessFlagBits::INDEX_READ_BIT | Anvil::AccessFlagBits::INDIRECT_COMMAND_READ_BIT | Anvil::AccessFlagBits::INPUT_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::MEMORY_READ_BIT | Anvil::AccessFlagBits::SHADER_READ_BIT | Anvil::AccessFlagBits::TRANSFER_READ_BIT | Anvil::AccessFlagBits::UNIFORM_READ_BIT | Anvil::AccessFlagBits::VERTEX_ATTRIBUTE_READ_BIT;}
Anvil::AccessFlags prosper::util::get_write_access_mask() {return Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT | Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::MEMORY_WRITE_BIT | Anvil::AccessFlagBits::SHADER_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;}

Anvil::AccessFlags prosper::util::get_image_read_access_mask() {return Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::HOST_READ_BIT | Anvil::AccessFlagBits::MEMORY_READ_BIT | Anvil::AccessFlagBits::SHADER_READ_BIT | Anvil::AccessFlagBits::TRANSFER_READ_BIT | Anvil::AccessFlagBits::UNIFORM_READ_BIT;}
Anvil::AccessFlags prosper::util::get_image_write_access_mask() {return Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT | Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::MEMORY_WRITE_BIT | Anvil::AccessFlagBits::SHADER_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;}

Anvil::PipelineBindPoint prosper::util::get_pipeline_bind_point(Anvil::ShaderStageFlags shaderStages)
{
	return ((shaderStages &Anvil::ShaderStageFlagBits::COMPUTE_BIT) != Anvil::ShaderStageFlagBits(0)) ? Anvil::PipelineBindPoint::COMPUTE : Anvil::PipelineBindPoint::GRAPHICS;
}

prosper::ImageAspectFlags prosper::util::get_aspect_mask(Format format)
{
	auto aspectMask = ImageAspectFlags::ColorBit;
	if(is_depth_format(format))
		aspectMask = ImageAspectFlags::DepthBit;
	return aspectMask;
}

uint32_t prosper::util::get_offset_alignment_padding(uint32_t offset,uint32_t alignment)
{
	if(alignment == 0u)
		return 0u;
	auto r = offset %alignment;
	if(r == 0u)
		return 0u;
	return alignment -r;
}
uint32_t prosper::util::get_aligned_size(uint32_t size,uint32_t alignment)
{
	if(alignment == 0u)
		return size;
	auto r = size %alignment;
	if(r == 0u)
		return size;
	return size +(alignment -r);
}

prosper::ImageAspectFlags prosper::util::get_aspect_mask(IImage &img) {return get_aspect_mask(img.GetFormat());}

void prosper::util::get_image_layout_transition_access_masks(Anvil::ImageLayout oldLayout,Anvil::ImageLayout newLayout,Anvil::AccessFlags &readAccessMask,Anvil::AccessFlags &writeAccessMask)
{
	// Source Layout
	switch(oldLayout)
	{
		// Undefined layout
		// Only allowed as initial layout!
		// Make sure any writes to the image have been finished
		case Anvil::ImageLayout::PREINITIALIZED:
			readAccessMask = Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;
			break;
		// Old layout is color attachment
		// Make sure any writes to the color buffer have been finished
		case Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
			readAccessMask = Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT;
			break;
		// Old layout is depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		case Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			readAccessMask = Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		// Old layout is transfer source
		// Make sure any reads from the image have been finished
		case Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL:
			readAccessMask = Anvil::AccessFlagBits::TRANSFER_READ_BIT;
			break;
		// Old layout is shader read (sampler, input attachment)
		// Make sure any shader reads from the image have been finished
		case Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL:
			readAccessMask = Anvil::AccessFlagBits::SHADER_READ_BIT;
			break;
	};

	// Target Layout
	switch(newLayout)
	{
		// New layout is transfer destination (copy, blit)
		// Make sure any copyies to the image have been finished
		case Anvil::ImageLayout::TRANSFER_DST_OPTIMAL:
			writeAccessMask = Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;
			break;
		// New layout is transfer source (copy, blit)
		// Make sure any reads from and writes to the image have been finished
		case Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL:
			readAccessMask = readAccessMask | Anvil::AccessFlagBits::TRANSFER_READ_BIT;
			writeAccessMask = Anvil::AccessFlagBits::TRANSFER_READ_BIT;
			break;
		// New layout is color attachment
		// Make sure any writes to the color buffer hav been finished
		case Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
			writeAccessMask = Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT;
			readAccessMask = readAccessMask | Anvil::AccessFlagBits::TRANSFER_READ_BIT;
			break;
		// New layout is depth attachment
		// Make sure any writes to depth/stencil buffer have been finished
		case Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			writeAccessMask = writeAccessMask | Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		// New layout is shader read (sampler, input attachment)
		// Make sure any writes to the image have been finished
		case Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL:
			readAccessMask = readAccessMask | Anvil::AccessFlagBits::HOST_WRITE_BIT | Anvil::AccessFlagBits::TRANSFER_WRITE_BIT;
			writeAccessMask = Anvil::AccessFlagBits::SHADER_READ_BIT;
			break;
		case Anvil::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			readAccessMask = readAccessMask | Anvil::AccessFlagBits::SHADER_WRITE_BIT;
			writeAccessMask = Anvil::AccessFlagBits::SHADER_READ_BIT;
			break;
	};
}

static Anvil::Format get_anvil_format(uimg::TextureInfo::InputFormat format)
{
	Anvil::Format anvFormat = Anvil::Format::R8G8B8A8_UNORM;
	switch(format)
	{
	case uimg::TextureInfo::InputFormat::R8G8B8A8_UInt:
		anvFormat = Anvil::Format::B8G8R8A8_UNORM;
		break;
	case uimg::TextureInfo::InputFormat::R16G16B16A16_Float:
		anvFormat = Anvil::Format::R16G16B16A16_SFLOAT;
		break;
	case uimg::TextureInfo::InputFormat::R32G32B32A32_Float:
		anvFormat = Anvil::Format::R32G32B32A32_SFLOAT;
		break;
	case uimg::TextureInfo::InputFormat::R32_Float:
		anvFormat = Anvil::Format::R32_SFLOAT;
		break;
	case uimg::TextureInfo::InputFormat::KeepInputImageFormat:
		break;
	}
	return anvFormat;
}
static Anvil::Format get_anvil_format(uimg::TextureInfo::OutputFormat format)
{
	Anvil::Format anvFormat = Anvil::Format::R8G8B8A8_UNORM;
	switch(format)
	{
	case uimg::TextureInfo::OutputFormat::RGB:
	case uimg::TextureInfo::OutputFormat::RGBA:
		return Anvil::Format::R8G8B8A8_UNORM;
	case uimg::TextureInfo::OutputFormat::DXT1:
	case uimg::TextureInfo::OutputFormat::BC1:
	case uimg::TextureInfo::OutputFormat::DXT1n:
	case uimg::TextureInfo::OutputFormat::CTX1:
	case uimg::TextureInfo::OutputFormat::DXT1a:
	case uimg::TextureInfo::OutputFormat::BC1a:
		return Anvil::Format::BC1_RGBA_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::DXT3:
	case uimg::TextureInfo::OutputFormat::BC2:
		return Anvil::Format::BC2_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::DXT5:
	case uimg::TextureInfo::OutputFormat::DXT5n:
	case uimg::TextureInfo::OutputFormat::BC3:
	case uimg::TextureInfo::OutputFormat::BC3n:
	case uimg::TextureInfo::OutputFormat::BC3_RGBM:
		return Anvil::Format::BC3_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::BC4:
		return Anvil::Format::BC4_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::BC5:
		return Anvil::Format::BC5_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::BC6:
		return Anvil::Format::BC6H_SFLOAT_BLOCK;
	case uimg::TextureInfo::OutputFormat::BC7:
		return Anvil::Format::BC7_UNORM_BLOCK;
	case uimg::TextureInfo::OutputFormat::ETC1:
	case uimg::TextureInfo::OutputFormat::ETC2_R:
	case uimg::TextureInfo::OutputFormat::ETC2_RG:
	case uimg::TextureInfo::OutputFormat::ETC2_RGB:
	case uimg::TextureInfo::OutputFormat::ETC2_RGBA:
	case uimg::TextureInfo::OutputFormat::ETC2_RGB_A1:
	case uimg::TextureInfo::OutputFormat::ETC2_RGBM:
		return Anvil::Format::ETC2_R8G8B8A8_UNORM_BLOCK;
	}
	return anvFormat;
}
static bool is_compatible(prosper::Format imgFormat,uimg::TextureInfo::OutputFormat outputFormat)
{
	if(outputFormat == uimg::TextureInfo::OutputFormat::KeepInputImageFormat)
		return true;
	switch(outputFormat)
	{
	case uimg::TextureInfo::OutputFormat::DXT1:
	case uimg::TextureInfo::OutputFormat::DXT1a:
	case uimg::TextureInfo::OutputFormat::DXT1n:
	case uimg::TextureInfo::OutputFormat::BC1:
	case uimg::TextureInfo::OutputFormat::BC1a:
		return imgFormat == prosper::Format::BC1_RGBA_SRGB_Block ||
			imgFormat == prosper::Format::BC1_RGBA_UNorm_Block ||
			imgFormat == prosper::Format::BC1_RGB_SRGB_Block ||
			imgFormat == prosper::Format::BC1_RGB_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::DXT3:
	case uimg::TextureInfo::OutputFormat::BC2:
		return imgFormat == prosper::Format::BC2_SRGB_Block ||
			imgFormat == prosper::Format::BC2_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::DXT5:
	case uimg::TextureInfo::OutputFormat::DXT5n:
	case uimg::TextureInfo::OutputFormat::BC3:
	case uimg::TextureInfo::OutputFormat::BC3n:
	case uimg::TextureInfo::OutputFormat::BC3_RGBM:
		return imgFormat == prosper::Format::BC3_SRGB_Block ||
			imgFormat == prosper::Format::BC3_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC4:
		return imgFormat == prosper::Format::BC4_SNorm_Block ||
			imgFormat == prosper::Format::BC4_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC5:
		return imgFormat == prosper::Format::BC5_SNorm_Block ||
			imgFormat == prosper::Format::BC5_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC6:
		return imgFormat == prosper::Format::BC6H_SFloat_Block ||
			imgFormat == prosper::Format::BC6H_UFloat_Block;
	case uimg::TextureInfo::OutputFormat::BC7:
		return imgFormat == prosper::Format::BC7_SRGB_Block ||
			imgFormat == prosper::Format::BC7_UNorm_Block;
#if 0 // Disabled due to poor coverage
	case uimg::TextureInfo::OutputFormat::ETC2_R:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RG:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGB:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGBA:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGB_A1:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGBM:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
#endif
	case uimg::TextureInfo::OutputFormat::RGB:
	case uimg::TextureInfo::OutputFormat::RGBA:
		return prosper::util::is_uncompressed_format(static_cast<prosper::Format>(imgFormat));
	case uimg::TextureInfo::OutputFormat::CTX1:
	case uimg::TextureInfo::OutputFormat::ETC1:
	default:
		return false;
	}
	return false;
}
bool prosper::util::save_texture(const std::string &fileName,prosper::IImage &image,const uimg::TextureInfo &texInfo,const std::function<void(const std::string&)> &errorHandler)
{
	std::shared_ptr<prosper::IImage> imgRead = image.shared_from_this();
	auto srcFormat = image.GetFormat();
	auto dstFormat = srcFormat;
	if(texInfo.inputFormat != uimg::TextureInfo::InputFormat::KeepInputImageFormat)
		dstFormat = static_cast<prosper::Format>(get_anvil_format(texInfo.inputFormat));
	if(texInfo.outputFormat == uimg::TextureInfo::OutputFormat::KeepInputImageFormat || is_compressed_format(imgRead->GetFormat()))
	{
		std::optional<uimg::TextureInfo> convTexInfo = {};
		if(is_compatible(imgRead->GetFormat(),texInfo.outputFormat) == false)
		{
			// Convert the image into the target format
			auto &context = image.GetContext();
			auto &setupCmd = context.GetSetupCommandBuffer();

			// Note: We should just be able to convert the image to
			// the destination format directly, but for some reason this code
			// causes issues when dealing with compressed formats. So instead,
			// we convert it to a generic color format, and then redirect the image compression
			// to the util_image library instead of gli.
			// TODO: It would be much more efficient to use gli for this, too, find out why this isn't working?
			auto copyCreateInfo = image.GetCreateInfo();
			//copyCreateInfo.format = Anvil::Format::BC1_RGBA_UNORM_BLOCK;//get_anvil_format(texInfo.outputFormat);
			copyCreateInfo.format = Format::R32G32B32A32_SFloat;
			copyCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
			copyCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
			copyCreateInfo.tiling = ImageTiling::Optimal; // Needs to be in optimal tiling because some GPUs do not support linear tiling with mipmaps
			setupCmd->RecordImageBarrier(image,ImageLayout::ShaderReadOnlyOptimal,ImageLayout::TransferSrcOptimal);
			imgRead = image.Copy(*setupCmd,copyCreateInfo);
			setupCmd->RecordImageBarrier(image,ImageLayout::TransferSrcOptimal,ImageLayout::ShaderReadOnlyOptimal);
			setupCmd->RecordImageBarrier(*imgRead,ImageLayout::TransferDstOptimal,ImageLayout::ShaderReadOnlyOptimal);
			context.FlushSetupCommandBuffer();

			convTexInfo = texInfo;
			convTexInfo->inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
		}

		// No conversion needed, just copy the image data to a buffer and save it directly
		auto extents = imgRead->GetExtents();
		auto numLayers = imgRead->GetLayerCount();
		auto numLevels = imgRead->GetMipmapCount();
		auto &context = imgRead->GetContext();
		auto &setupCmd = context.GetSetupCommandBuffer();

		gli::extent2d gliExtents {extents.width,extents.height};
		gli::texture2d gliTex {static_cast<gli::texture::format_type>(imgRead->GetFormat()),gliExtents,numLevels};

		prosper::util::BufferCreateInfo bufCreateInfo {};
		bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUToCPU;
		bufCreateInfo.size = gliTex.size();
		bufCreateInfo.usageFlags = prosper::BufferUsageFlags::TransferDstBit;
		auto buf = context.CreateBuffer(bufCreateInfo);
		buf->SetPermanentlyMapped(true);

		setupCmd->RecordImageBarrier(*imgRead,ImageLayout::ShaderReadOnlyOptimal,ImageLayout::TransferSrcOptimal);
		// Initialize buffer with image data
		size_t bufferOffset = 0;
		for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
		{
			for(auto iMipmap=decltype(numLevels){0u};iMipmap<numLevels;++iMipmap)
			{
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex.size(iMipmap);
				prosper::util::BufferImageCopyInfo copyInfo {};
				copyInfo.baseArrayLayer = iLayer;
				copyInfo.bufferOffset = bufferOffset;
				copyInfo.dstImageLayout = ImageLayout::TransferSrcOptimal;
				copyInfo.width = extents.width;
				copyInfo.height = extents.height;
				copyInfo.layerCount = 1;
				copyInfo.mipLevel = iMipmap;
				setupCmd->RecordCopyImageToBuffer(copyInfo,*imgRead,ImageLayout::TransferSrcOptimal,*buf);

				bufferOffset += mipmapSize;
			}
		}
		setupCmd->RecordImageBarrier(*imgRead,ImageLayout::TransferSrcOptimal,ImageLayout::ShaderReadOnlyOptimal);
		context.FlushSetupCommandBuffer();

		// Copy the data to a gli texture object
		bufferOffset = 0ull;
		for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
		{
			for(auto iMipmap=decltype(numLevels){0u};iMipmap<numLevels;++iMipmap)
			{
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex.size(iMipmap);
				auto *dstData = gliTex.data(iLayer,0u /* face */,iMipmap);
				memset(dstData,1,mipmapSize);
				buf->Read(bufferOffset,mipmapSize,dstData);
				bufferOffset += mipmapSize;
			}
		}
		if(convTexInfo.has_value())
		{
			std::vector<std::vector<const void*>> layerMipmapData {};
			layerMipmapData.reserve(numLayers);
			for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
			{
				layerMipmapData.push_back({});
				auto &mipmapData = layerMipmapData.back();
				mipmapData.reserve(numLevels);
				for(auto iMipmap=decltype(numLevels){0u};iMipmap<numLevels;++iMipmap)
				{
					auto *dstData = gliTex.data(iLayer,0u /* face */,iMipmap);
					mipmapData.push_back(dstData);
				}
			}
			return uimg::save_texture(fileName,layerMipmapData,extents.width,extents.height,sizeof(float) *4,*convTexInfo,false);
		}
		auto fullFileName = uimg::get_absolute_path(fileName,texInfo.containerFormat);
		switch(texInfo.containerFormat)
		{
		case uimg::TextureInfo::ContainerFormat::DDS:
			return gli::save_dds(gliTex,fullFileName);
		case uimg::TextureInfo::ContainerFormat::KTX:
			return gli::save_ktx(gliTex,fullFileName);
		}
		return false;
	}

	std::shared_ptr<prosper::IBuffer> buf = nullptr;
	std::vector<std::vector<size_t>> layerMipmapOffsets {};
	uint32_t sizePerPixel = 0u;
	if(
		image.GetTiling() != ImageTiling::Linear ||
		umath::is_flag_set(image.GetCreateInfo().memoryFeatures,prosper::MemoryFeatureFlags::HostAccessable) == false ||
		image.GetFormat() != dstFormat
		)
	{
		// Convert the image into the target format
		auto &context = image.GetContext();
		auto &setupCmd = context.GetSetupCommandBuffer();

		auto copyCreateInfo = image.GetCreateInfo();
		copyCreateInfo.format = dstFormat;
		copyCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		copyCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
		copyCreateInfo.tiling = ImageTiling::Optimal; // Needs to be in optimal tiling because some GPUs do not support linear tiling with mipmaps
		setupCmd->RecordImageBarrier(image,ImageLayout::ShaderReadOnlyOptimal,ImageLayout::TransferSrcOptimal);
		imgRead = image.Copy(*setupCmd,copyCreateInfo);
		setupCmd->RecordImageBarrier(image,ImageLayout::TransferSrcOptimal,ImageLayout::ShaderReadOnlyOptimal);

		// Copy the image data to a buffer
		uint64_t size = 0;
		uint64_t offset = 0;
		auto numLayers = image.GetLayerCount();
		auto numMipmaps = image.GetMipmapCount();
		sizePerPixel = prosper::util::is_compressed_format(dstFormat) ? gli::block_size(static_cast<gli::texture::format_type>(dstFormat)) : prosper::util::get_byte_size(dstFormat);
		layerMipmapOffsets.resize(numLayers);
		for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
		{
			auto &mipmapOffsets = layerMipmapOffsets.at(iLayer);
			mipmapOffsets.resize(numMipmaps);
			for(auto iMipmap=decltype(numMipmaps){0u};iMipmap<numMipmaps;++iMipmap)
			{
				mipmapOffsets.at(iMipmap) = size;

				auto extents = image.GetExtents(iMipmap);
				size += extents.width *extents.height *sizePerPixel;
			}
		}
		buf = context.AllocateTemporaryBuffer(size);
		if(buf == nullptr)
		{
			context.FlushSetupCommandBuffer();
			return false; // Buffer allocation failed; Requested size too large?
		}
		setupCmd->RecordImageBarrier(*imgRead,ImageLayout::TransferDstOptimal,ImageLayout::TransferSrcOptimal);

		prosper::util::BufferImageCopyInfo copyInfo {};
		copyInfo.dstImageLayout = ImageLayout::TransferSrcOptimal;

		struct ImageMipmapData
		{
			uint32_t mipmapIndex = 0u;
			uint64_t bufferOffset = 0ull;
			uint64_t bufferSize = 0ull;
			vk::Extent2D extents = {};
		};
		struct ImageLayerData
		{
			std::vector<ImageMipmapData> mipmaps = {};
			uint32_t layerIndex = 0u;
		};
		std::vector<ImageLayerData> layers = {};
		for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
		{
			layers.push_back({});
			auto &layerData = layers.back();
			layerData.layerIndex = iLayer;
			for(auto iMipmap=decltype(numMipmaps){0u};iMipmap<numMipmaps;++iMipmap)
			{
				layerData.mipmaps.push_back({});
				auto &mipmapData = layerData.mipmaps.back();
				mipmapData.mipmapIndex = iMipmap;
				mipmapData.bufferOffset = layerMipmapOffsets.at(iLayer).at(iMipmap);

				auto extents = image.GetExtents(iMipmap);
				static_assert(sizeof(prosper::Extent2D) == sizeof(vk::Extent2D));
				mipmapData.extents = reinterpret_cast<vk::Extent2D&>(extents);
				mipmapData.bufferSize = extents.width *extents.height *sizePerPixel;
			}
		}
		for(auto &layerData : layers)
		{
			for(auto &mipmapData : layerData.mipmaps)
			{
				copyInfo.mipLevel = mipmapData.mipmapIndex;
				copyInfo.baseArrayLayer = layerData.layerIndex;
				copyInfo.bufferOffset = mipmapData.bufferOffset;
				setupCmd->RecordCopyImageToBuffer(copyInfo,*imgRead,ImageLayout::TransferSrcOptimal,*buf);
			}
		}
		context.FlushSetupCommandBuffer();

		/*// The dds library expects the image data in RGB form, so we have to do some conversions in some cases.
		std::optional<util::ImageBuffer::Format> imgBufFormat = {};
		switch(srcFormat)
		{
		case Anvil::Format::B8G8R8_UNORM:
		case Anvil::Format::B8G8R8A8_UNORM:
		imgBufFormat = util::ImageBuffer::Format::RGBA32; // TODO: dst format
		break;
		}
		if(imgBufFormat.has_value())
		{
		for(auto &layerData : layers)
		{
		for(auto &mipmapData : layerData.mipmaps)
		{
		std::vector<uint8_t> imgData {};
		imgData.resize(mipmapData.bufferSize);
		if(buf->Read(mipmapData.bufferOffset,mipmapData.bufferSize,imgData.data()))
		{
		// Swap red and blue channels, then write the new image data back to the buffer
		auto imgBuf = util::ImageBuffer::Create(imgData.data(),mipmapData.extents.width,mipmapData.extents.height,*imgBufFormat,true);
		imgBuf->SwapChannels(util::ImageBuffer::Channel::Red,util::ImageBuffer::Channel::Blue);
		buf->Write(mipmapData.bufferOffset,mipmapData.bufferSize,imgData.data());
		}
		}
		}
		}*/
	}
	auto numLayers = imgRead->GetLayerCount();
	auto numMipmaps = imgRead->GetMipmapCount();
	auto cubemap = imgRead->IsCubemap();
	auto extents = imgRead->GetExtents();
	if(buf != nullptr)
	{
		return uimg::save_texture(fileName,[imgRead,buf,&layerMipmapOffsets,sizePerPixel](uint32_t iLayer,uint32_t iMipmap,std::function<void(void)> &outDeleter) -> const uint8_t* {
			void *data;
			auto *memBlock = dynamic_cast<VlkBuffer&>(*buf)->get_memory_block(0u);
			auto offset = layerMipmapOffsets.at(iLayer).at(iMipmap);
			auto extents = imgRead->GetExtents(iMipmap);
			auto size = extents.width *extents.height *sizePerPixel;
			if(memBlock->map(offset,sizePerPixel,&data) == false)
				return nullptr;
			outDeleter = [memBlock]() {
				memBlock->unmap(); // Note: setMipmapData copies the data, so we don't need to keep it mapped
			};
			return static_cast<uint8_t*>(data);
		},extents.width,extents.height,get_byte_size(dstFormat),numLayers,numMipmaps,cubemap,texInfo,errorHandler);
	}
	return uimg::save_texture(fileName,[imgRead](uint32_t iLayer,uint32_t iMipmap,std::function<void(void)> &outDeleter) -> const uint8_t* {
		auto subresourceLayout = imgRead->GetSubresourceLayout(iLayer,iMipmap);
		if(subresourceLayout.has_value() == false)
			return nullptr;
		void *data;
		auto *memBlock = static_cast<prosper::VlkImage&>(*imgRead)->get_memory_block();
		if(memBlock->map(subresourceLayout->offset,subresourceLayout->size,&data) == false)
			return nullptr;
		outDeleter = [memBlock]() {
			memBlock->unmap(); // Note: setMipmapData copies the data, so we don't need to keep it mapped
		};
		return static_cast<uint8_t*>(data);
		},extents.width,extents.height,get_byte_size(dstFormat),numLayers,numMipmaps,cubemap,texInfo,errorHandler);
}
#pragma optimize("",on)
