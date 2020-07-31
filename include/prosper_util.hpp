/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_UTIL_HPP__
#define __PROSPER_UTIL_HPP__

#include <mathutil/umath.h>
#include <functional>
#include <optional>
#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_enums.hpp"
#include "prosper_structs.hpp"

namespace uimg
{
	class ImageBuffer;
	struct TextureInfo;
};

namespace Anvil
{
	struct MemoryType;
	class BaseDevice;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class ICommandBuffer;
	class IPrimaryCommandBuffer;
	class RenderTarget;
	class Texture;
	class IImage;
	class IFramebuffer;
	class ImageView;
	class IRenderPass;
	class ISampler;
	class IFramebuffer;
	class IDescriptorSetGroup;
	class IDescriptorSet;
	enum class Vendor : uint32_t;
	enum class PhysicalDeviceType : uint8_t;
	namespace util
	{
		DLLPROSPER Format get_prosper_format(const uimg::ImageBuffer &imgBuffer);
		DLLPROSPER prosper::util::ImageCreateInfo get_image_create_info(const uimg::ImageBuffer &imgBuffer,bool cubemap=false);

		DLLPROSPER BufferBarrier create_buffer_barrier(const util::BufferBarrierInfo &barrierInfo,IBuffer &buffer);
		DLLPROSPER ImageBarrier create_image_barrier(IImage &img,const util::ImageBarrierInfo &barrierInfo);
		DLLPROSPER ImageBarrier create_image_barrier(IImage &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,const ImageSubresourceRange &subresourceRange={});

		DLLPROSPER void calculate_mipmap_size(uint32_t w,uint32_t h,uint32_t *wMipmap,uint32_t *hMipmap,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_size(uint32_t v,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_count(uint32_t w,uint32_t h);
		DLLPROSPER std::string to_string(Result r);
		DLLPROSPER std::string to_string(DebugReportFlags flags);
		DLLPROSPER std::string to_string(DebugReportObjectTypeEXT type);
		DLLPROSPER std::string to_string(Format format);
		DLLPROSPER std::string to_string(ShaderStageFlags shaderStage);
		DLLPROSPER std::string to_string(ImageType type);
		DLLPROSPER std::string to_string(ImageTiling tiling);
		DLLPROSPER std::string to_string(ImageUsageFlags usage);
		DLLPROSPER std::string to_string(ImageCreateFlags createFlags);
		DLLPROSPER std::string to_string(ShaderStage stage);
		DLLPROSPER std::string to_string(PipelineStageFlags stage);
		DLLPROSPER std::string to_string(ImageLayout layout);
		DLLPROSPER std::string to_string(PhysicalDeviceType type);
		DLLPROSPER std::string to_string(Vendor type);
		DLLPROSPER std::string to_string(SampleCountFlags samples);
		DLLPROSPER bool has_alpha(Format format);
		DLLPROSPER bool is_depth_format(Format format);
		DLLPROSPER bool is_compressed_format(Format format);
		DLLPROSPER bool is_uncompressed_format(Format format);
		DLLPROSPER bool is_8bit_format(Format format);
		DLLPROSPER bool is_16bit_format(Format format);
		DLLPROSPER bool is_32bit_format(Format format);
		DLLPROSPER bool is_64bit_format(Format format);
		DLLPROSPER uint32_t get_bit_size(Format format);
		DLLPROSPER uint32_t get_byte_size(Format format);
		DLLPROSPER uint32_t get_block_size(Format format);
		DLLPROSPER uint32_t get_component_count(Format format);
		DLLPROSPER prosper::AccessFlags get_read_access_mask();
		DLLPROSPER prosper::AccessFlags get_write_access_mask();
		DLLPROSPER prosper::AccessFlags get_image_read_access_mask();
		DLLPROSPER prosper::AccessFlags get_image_write_access_mask();
		DLLPROSPER prosper::PipelineBindPoint get_pipeline_bind_point(prosper::ShaderStageFlags shaderStages);
		DLLPROSPER ImageAspectFlags get_aspect_mask(IImage &img);
		DLLPROSPER ImageAspectFlags get_aspect_mask(Format format);
		DLLPROSPER void apply_image_subresource_range(const prosper::util::ImageSubresourceRange &srcRange,prosper::util::ImageSubresourceRange &vkRange,prosper::IImage &img);
		//get_queue_family_index
		DLLPROSPER bool save_texture(const std::string &fileName,prosper::IImage &image,const uimg::TextureInfo &texInfo,const std::function<void(const std::string&)> &errorHandler=nullptr);

		// Returns the padding required to align the offset with the specified alignment
		DLLPROSPER uint32_t get_offset_alignment_padding(uint32_t offset,uint32_t alignment);
		// Returns the size with padding required to align with the specified alignment
		DLLPROSPER uint32_t get_aligned_size(uint32_t size,uint32_t alignment);

		DLLPROSPER void get_image_layout_transition_access_masks(prosper::ImageLayout oldLayout,prosper::ImageLayout newLayout,prosper::AccessFlags &readAccessMask,prosper::AccessFlags &writeAccessMask);

		struct DLLPROSPER VendorDeviceInfo
		{
			uint32_t apiVersion = 0;
			std::string deviceName;
			PhysicalDeviceType deviceType;
			uint32_t deviceId;
			uint32_t driverVersion;
			Vendor vendor;
		};

		struct DLLPROSPER Limits
		{
			float maxSamplerAnisotropy = 0.f;
			uint32_t maxSurfaceImageCount = 0;
			uint32_t maxImageArrayLayers = 0;
			DeviceSize maxStorageBufferRange = 0;
		};

		struct DLLPROSPER PhysicalDeviceImageFormatProperties
		{
			SampleCountFlags sampleCount;
		};

		struct DLLPROSPER PhysicalDeviceMemoryProperties
		{
			std::vector<DeviceSize> heapSizes;
		};

		template<class T>
			std::shared_ptr<T> unique_ptr_to_shared_ptr(std::unique_ptr<T,std::function<void(T*)>> v);
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::ImageCreateInfo::Flags);
#pragma warning(pop)

template<class T>
	std::shared_ptr<T> prosper::util::unique_ptr_to_shared_ptr(std::unique_ptr<T,std::function<void(T*)>> v)
{
	return (v == nullptr) ? std::shared_ptr<T>(nullptr) : std::shared_ptr<T>(v.release(),v.get_deleter());
}

#endif
