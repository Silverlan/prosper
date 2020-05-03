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
#include <misc/types.h>

namespace uimg
{
	class ImageBuffer;
	struct TextureInfo;
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
	namespace debug
	{
		DLLPROSPER void enable_debug_recorded_image_layout(bool b);
		DLLPROSPER bool is_debug_recorded_image_layout_enabled();
		DLLPROSPER void set_debug_validation_callback(const std::function<void(vk::DebugReportObjectTypeEXT,const std::string&)> &callback);
		DLLPROSPER void exec_debug_validation_callback(vk::DebugReportObjectTypeEXT objType,const std::string &msg);
		DLLPROSPER bool get_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout &layout,uint32_t baseLayer=0u,uint32_t baseMipmap=0u);
		DLLPROSPER void set_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout layout,uint32_t baseLayer=0u,uint32_t layerCount=std::numeric_limits<uint32_t>::max(),uint32_t baseMipmap=0u,uint32_t mipmapLevels=std::numeric_limits<uint32_t>::max());
	};
	namespace util
	{
		DLLPROSPER bool get_current_render_pass_target(prosper::IPrimaryCommandBuffer &cmdBuffer,prosper::IRenderPass **outRp=nullptr,prosper::IImage **outImg=nullptr,prosper::IFramebuffer **outFb=nullptr,prosper::RenderTarget **outRt=nullptr);

		DLLPROSPER BufferBarrier create_buffer_barrier(const util::BufferBarrierInfo &barrierInfo,IBuffer &buffer);
		DLLPROSPER ImageBarrier create_image_barrier(IImage &img,const util::ImageBarrierInfo &barrierInfo);
		DLLPROSPER ImageBarrier create_image_barrier(IImage &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,const ImageSubresourceRange &subresourceRange={});

		DLLPROSPER uint32_t calculate_buffer_alignment(Anvil::BaseDevice &dev,BufferUsageFlags usageFlags);
		DLLPROSPER void calculate_mipmap_size(uint32_t w,uint32_t h,uint32_t *wMipmap,uint32_t *hMipmap,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_size(uint32_t v,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_count(uint32_t w,uint32_t h);
		DLLPROSPER std::string to_string(vk::Result r);
		DLLPROSPER std::string to_string(vk::DebugReportFlagsEXT flags);
		DLLPROSPER std::string to_string(vk::DebugReportObjectTypeEXT type);
		DLLPROSPER std::string to_string(vk::DebugReportFlagBitsEXT flags);
		DLLPROSPER std::string to_string(Format format);
		DLLPROSPER std::string to_string(ShaderStageFlags shaderStage);
		DLLPROSPER std::string to_string(ImageType type);
		DLLPROSPER std::string to_string(ImageTiling tiling);
		DLLPROSPER std::string to_string(ImageUsageFlags usage);
		DLLPROSPER std::string to_string(ImageCreateFlags createFlags);
		DLLPROSPER std::string to_string(ShaderStage stage);
		DLLPROSPER std::string to_string(PipelineStageFlags stage);
		DLLPROSPER std::string to_string(ImageLayout layout);
		DLLPROSPER std::string to_string(vk::PhysicalDeviceType type);
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
		DLLPROSPER Anvil::AccessFlags get_read_access_mask();
		DLLPROSPER Anvil::AccessFlags get_write_access_mask();
		DLLPROSPER Anvil::AccessFlags get_image_read_access_mask();
		DLLPROSPER Anvil::AccessFlags get_image_write_access_mask();
		DLLPROSPER Anvil::PipelineBindPoint get_pipeline_bind_point(Anvil::ShaderStageFlags shaderStages);
		DLLPROSPER ImageAspectFlags get_aspect_mask(IImage &img);
		DLLPROSPER ImageAspectFlags get_aspect_mask(Format format);
		DLLPROSPER std::pair<const Anvil::MemoryType*,MemoryFeatureFlags> find_compatible_memory_type(Anvil::BaseDevice &dev,MemoryFeatureFlags featureFlags);
		DLLPROSPER bool save_texture(const std::string &fileName,prosper::IImage &image,const uimg::TextureInfo &texInfo,const std::function<void(const std::string&)> &errorHandler=nullptr);

		// Clamps the specified size in bytes to a percentage of the total available GPU memory
		uint64_t clamp_gpu_memory_size(Anvil::BaseDevice &dev,uint64_t size,float percentageOfGPUMemory,MemoryFeatureFlags featureFlags);

		// Returns the padding required to align the offset with the specified alignment
		DLLPROSPER uint32_t get_offset_alignment_padding(uint32_t offset,uint32_t alignment);
		// Returns the size with padding required to align with the specified alignment
		DLLPROSPER uint32_t get_aligned_size(uint32_t size,uint32_t alignment);

		DLLPROSPER void get_image_layout_transition_access_masks(Anvil::ImageLayout oldLayout,Anvil::ImageLayout newLayout,Anvil::AccessFlags &readAccessMask,Anvil::AccessFlags &writeAccessMask);

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
