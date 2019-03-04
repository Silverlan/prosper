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

namespace Anvil
{
	struct BufferBarrier;
	class PrimaryCommandBuffer;
	class CommandBufferBase;
	class BaseDevice;
	class Image;
	struct ImageBarrier;
	class ImageView;
	class Sampler;
	class RenderPass;
	class RenderPassCreateInfo;
	class DescriptorSet;
	class DescriptorSetGroup;
	class DescriptorSetCreateInfo;
	struct MipmapRawData;
	class Framebuffer;
};

namespace uimg
{
	class Image;
	enum class ImageType : uint32_t;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class RenderTarget;
	class Texture;
	class Image;
	class ImageView;
	class RenderPass;
	class Sampler;
	class Framebuffer;
	class DescriptorSetGroup;
	class Buffer;
	namespace debug
	{
		DLLPROSPER void enable_debug_recorded_image_layout(bool b);
		DLLPROSPER bool is_debug_recorded_image_layout_enabled();
		DLLPROSPER void set_debug_validation_callback(const std::function<void(vk::DebugReportObjectTypeEXT,const std::string&)> &callback);
		DLLPROSPER void exec_debug_validation_callback(vk::DebugReportObjectTypeEXT objType,const std::string &msg);
		DLLPROSPER bool get_last_recorded_image_layout(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout &layout,uint32_t baseLayer=0u,uint32_t baseMipmap=0u);
		DLLPROSPER void set_last_recorded_image_layout(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout layout,uint32_t baseLayer=0u,uint32_t layerCount=std::numeric_limits<uint32_t>::max(),uint32_t baseMipmap=0u,uint32_t mipmapLevels=std::numeric_limits<uint32_t>::max());
	};
	namespace util
	{
		enum class MemoryFeatureFlags : uint32_t
		{
			None = 0u,
			DeviceLocal = 1u,
			HostCached = DeviceLocal<<1u,
			HostCoherent = HostCached<<1u,
			LazilyAllocated = HostCoherent<<1u,
			HostAccessable = LazilyAllocated<<1u,

			GPUBulk = DeviceLocal, // Usage: GPU memory that is rarely (if ever) updated by the CPU
			CPUToGPU = DeviceLocal | HostAccessable, // Usage: Memory that has to be frequently (e.g. every frame) updated by the CPU
			GPUToCPU = HostAccessable | HostCoherent | HostCached // Usage: Memory written to by the GPU, which needs to be readable by the CPU
		};
		enum class QueueFamilyFlags : uint32_t
		{
			None = 0u,
			Graphics = 1u,
			Compute = Graphics<<1u,
			DMA = Compute<<1u,
		};
		struct DLLPROSPER BufferCreateInfo
		{
			vk::DeviceSize size = 0ull;
			QueueFamilyFlags queueFamilyMask = QueueFamilyFlags::Graphics;
			enum class Flags : uint32_t
			{
				None = 0u,
				ConcurrentSharing = 1u, // Exclusive sharing is used if this flag is not set
				DontAllocateMemory = ConcurrentSharing<<1u,

				Sparse = DontAllocateMemory<<1u,
				SparseAliasedResidency = Sparse<<1u // Only has an effect if Sparse-flag is set
			};
			Flags flags = Flags::None;
			Anvil::BufferUsageFlags usageFlags = Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT;
			MemoryFeatureFlags memoryFeatures = MemoryFeatureFlags::GPUBulk; // Only used if AllocateMemory-flag is set
		};
		struct DLLPROSPER ImageCreateInfo
		{
			Anvil::ImageType type = Anvil::ImageType::_2D;
			uint32_t width = 0u;
			uint32_t height = 0u;
			Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;
			uint32_t layers = 1u;
			Anvil::ImageUsageFlags usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT;
			Anvil::SampleCountFlagBits samples = Anvil::SampleCountFlagBits::_1_BIT;
			Anvil::ImageTiling tiling = Anvil::ImageTiling::OPTIMAL;
			Anvil::ImageLayout postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;

			enum class Flags : uint32_t
			{
				None = 0u,
				Cubemap = 1u,
				ConcurrentSharing = Cubemap<<1u, // Exclusive sharing is used if this flag is not set
				FullMipmapChain = ConcurrentSharing<<1u,

				Sparse = FullMipmapChain<<1u,
				SparseAliasedResidency = Sparse<<1u // Only has an effect if Sparse-flag is set
			};
			Flags flags = Flags::None;
			QueueFamilyFlags queueFamilyMask = QueueFamilyFlags::Graphics;
			MemoryFeatureFlags memoryFeatures = MemoryFeatureFlags::GPUBulk;
		};
		struct DLLPROSPER ImageViewCreateInfo
		{
			std::optional<uint32_t> baseLayer = {};
			uint32_t levelCount = 1u;
			uint32_t baseMipmap = 0u;
			uint32_t mipmapLevels = std::numeric_limits<uint32_t>::max();
			Anvil::Format format = Anvil::Format::UNKNOWN;
			Anvil::ComponentSwizzle swizzleRed = Anvil::ComponentSwizzle::R;
			Anvil::ComponentSwizzle swizzleGreen = Anvil::ComponentSwizzle::G;
			Anvil::ComponentSwizzle swizzleBlue = Anvil::ComponentSwizzle::B;
			Anvil::ComponentSwizzle swizzleAlpha = Anvil::ComponentSwizzle::A;
		};

		struct DLLPROSPER RenderPassCreateInfo
		{
			struct DLLPROSPER AttachmentInfo
			{
				AttachmentInfo(
					Anvil::Format format=Anvil::Format::R8G8B8A8_UNORM,Anvil::ImageLayout initialLayout=Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
					Anvil::AttachmentLoadOp loadOp=Anvil::AttachmentLoadOp::DONT_CARE,Anvil::AttachmentStoreOp storeOp=Anvil::AttachmentStoreOp::STORE,
					Anvil::SampleCountFlagBits sampleCount=Anvil::SampleCountFlagBits::_1_BIT,Anvil::ImageLayout finalLayout=Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL
				);
				bool operator==(const AttachmentInfo &other) const;
				bool operator!=(const AttachmentInfo &other) const;
				Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;
				Anvil::SampleCountFlagBits sampleCount = Anvil::SampleCountFlagBits::_1_BIT;
				Anvil::AttachmentLoadOp loadOp = Anvil::AttachmentLoadOp::DONT_CARE;
				Anvil::AttachmentStoreOp storeOp = Anvil::AttachmentStoreOp::STORE;
				Anvil::ImageLayout initialLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
				Anvil::ImageLayout finalLayout = Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
			};
			struct DLLPROSPER SubPass
			{
				struct DLLPROSPER Dependency
				{
					Dependency(
						std::size_t sourceSubPassId,std::size_t destinationSubPassId,
						Anvil::PipelineStageFlags sourceStageMask,Anvil::PipelineStageFlags destinationStageMask,
						Anvil::AccessFlags sourceAccessMask,Anvil::AccessFlags destinationAccessMask
					);
					bool operator==(const Dependency &other) const;
					bool operator!=(const Dependency &other) const;
					std::size_t sourceSubPassId;
					std::size_t destinationSubPassId;
					Anvil::PipelineStageFlags sourceStageMask;
					Anvil::PipelineStageFlags destinationStageMask;
					Anvil::AccessFlags sourceAccessMask;
					Anvil::AccessFlags destinationAccessMask;
				};
				SubPass(const std::vector<std::size_t> &colorAttachments={},bool useDepthStencilAttachment=false,std::vector<Dependency> dependencies={});
				bool operator==(const SubPass &other) const;
				bool operator!=(const SubPass &other) const;
				std::vector<std::size_t> colorAttachments = {};
				bool useDepthStencilAttachment = false;
				std::vector<Dependency> dependencies = {};
			};
			RenderPassCreateInfo(const std::vector<AttachmentInfo> &attachments={},const std::vector<SubPass> &subPasses={});
			bool operator==(const RenderPassCreateInfo &other) const;
			bool operator!=(const RenderPassCreateInfo &other) const;
			std::vector<AttachmentInfo> attachments;
			std::vector<SubPass> subPasses;
		};

		DLLPROSPER std::shared_ptr<Image> create_image(Anvil::BaseDevice &dev,const ImageCreateInfo &createInfo,const uint8_t *data=nullptr);
		DLLPROSPER std::shared_ptr<Image> create_image(Anvil::BaseDevice &dev,const ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> &data);
		DLLPROSPER std::shared_ptr<ImageView> create_image_view(Anvil::BaseDevice &dev,const ImageViewCreateInfo &createInfo,std::shared_ptr<Image> img);
		DLLPROSPER std::shared_ptr<Buffer> create_buffer(Anvil::BaseDevice &dev,const BufferCreateInfo &createInfo,const void *data=nullptr);
		DLLPROSPER std::shared_ptr<RenderPass> create_render_pass(Anvil::BaseDevice &dev,std::unique_ptr<Anvil::RenderPassCreateInfo> renderPassInfo);
		DLLPROSPER std::shared_ptr<RenderPass> create_render_pass(Anvil::BaseDevice &dev,const RenderPassCreateInfo &renderPassInfo);
		DLLPROSPER std::shared_ptr<DescriptorSetGroup> create_descriptor_set_group(Anvil::BaseDevice &dev,std::unique_ptr<Anvil::DescriptorSetCreateInfo> descSetInfo);
		DLLPROSPER std::shared_ptr<Framebuffer> create_framebuffer(Anvil::BaseDevice &dev,uint32_t width,uint32_t height,uint32_t layers,const std::vector<Anvil::ImageView*> &attachments);

		DLLPROSPER void initialize_image(Anvil::BaseDevice &dev,const uimg::Image &imgSrc,Anvil::Image &img);
		DLLPROSPER Anvil::Format get_vk_format(uimg::ImageType imgType);

		struct DLLPROSPER BlitInfo
		{
			Anvil::ImageSubresourceLayers srcSubresourceLayer = {
				Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u
			};
			Anvil::ImageSubresourceLayers dstSubresourceLayer = {
				Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u
			};
		};

		struct DLLPROSPER CopyInfo
		{
			uint32_t width = std::numeric_limits<uint32_t>::max();
			uint32_t height = std::numeric_limits<uint32_t>::max();
			Anvil::ImageSubresourceLayers srcSubresource = {Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u};
			Anvil::ImageSubresourceLayers dstSubresource = {Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u};

			vk::Offset3D srcOffset = vk::Offset3D(0,0,0);
			vk::Offset3D dstOffset = vk::Offset3D(0,0,0);

			Anvil::ImageLayout srcImageLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
			Anvil::ImageLayout dstImageLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
		};

		struct DLLPROSPER BufferImageCopyInfo
		{
			vk::DeviceSize bufferOffset = 0ull;
			uint32_t width = 0u;
			uint32_t height = 0u;
			uint32_t mipLevel = 0u;
			uint32_t baseArrayLayer = 0u;
			uint32_t layerCount = 1u;
			Anvil::ImageAspectFlags aspectMask = Anvil::ImageAspectFlagBits::COLOR_BIT;
			Anvil::ImageLayout dstImageLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
		};

		struct DLLPROSPER PipelineBarrierInfo
		{
			Anvil::PipelineStageFlags srcStageMask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;
			Anvil::PipelineStageFlags dstStageMask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;
			std::vector<Anvil::BufferBarrier> bufferBarriers;
			std::vector<Anvil::ImageBarrier> imageBarriers;

			PipelineBarrierInfo()=default;
			PipelineBarrierInfo &operator=(const PipelineBarrierInfo&)=delete;
		};

		struct DLLPROSPER ImageSubresourceRange
		{
			ImageSubresourceRange()=default;
			ImageSubresourceRange(uint32_t baseLayer,uint32_t layerCount,uint32_t baseMipmapLevel,uint32_t mipmapCount)
				: baseArrayLayer(baseLayer),layerCount(layerCount),baseMipLevel(baseMipmapLevel),levelCount(mipmapCount)
			{}
			ImageSubresourceRange(uint32_t baseLayer,uint32_t layerCount,uint32_t baseMipmapLevel)
				: ImageSubresourceRange(baseLayer,layerCount,baseMipmapLevel,1u)
			{}
			ImageSubresourceRange(uint32_t baseLayer,uint32_t layerCount)
				: ImageSubresourceRange(baseLayer,layerCount,0u,std::numeric_limits<uint32_t>::max())
			{}
			ImageSubresourceRange(uint32_t baseLayer)
				: ImageSubresourceRange(baseLayer,1u)
			{}
			void ApplyRange(Anvil::ImageSubresourceRange &vkRange,Anvil::Image &img) const;
			uint32_t baseMipLevel = 0u;
			uint32_t levelCount = std::numeric_limits<uint32_t>::max();
			uint32_t baseArrayLayer = 0u;
			uint32_t layerCount = std::numeric_limits<uint32_t>::max();
		};

		struct DLLPROSPER ClearImageInfo
		{
			ImageSubresourceRange subresourceRange = {};
		};

		DLLPROSPER bool record_clear_image(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout layout,const std::array<float,4> &clearColor,const ClearImageInfo clearImageInfo={});
		DLLPROSPER bool record_clear_image(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout layout,float clearDepth,const ClearImageInfo clearImageInfo={});
		DLLPROSPER bool record_clear_attachment(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,const std::array<float,4> &clearColor,uint32_t attId=0u);
		DLLPROSPER bool record_clear_attachment(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,const std::array<float,4> &clearColor,uint32_t attId,uint32_t layerId);
		DLLPROSPER bool record_clear_attachment(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,float clearDepth);
		DLLPROSPER bool record_clear_attachment(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,float clearDepth,uint32_t layerId);
		DLLPROSPER bool record_copy_image(Anvil::CommandBufferBase &cmdBuffer,const CopyInfo &copyInfo,Anvil::Image &imgSrc,Anvil::Image &imgDst);
		DLLPROSPER bool record_copy_buffer_to_image(Anvil::CommandBufferBase &cmdBuffer,const BufferImageCopyInfo &copyInfo,Buffer &bufferSrc,Anvil::Image &imgDst);
		DLLPROSPER bool record_copy_image_to_buffer(Anvil::CommandBufferBase &cmdBuffer,const BufferImageCopyInfo &copyInfo,Anvil::Image &imgSrc,Anvil::ImageLayout srcImageLayout,Buffer &bufferDst);
		DLLPROSPER bool record_copy_buffer(Anvil::CommandBufferBase &cmdBuffer,const Anvil::BufferCopy &copyInfo,Buffer &bufferSrc,Buffer &bufferDst);
		DLLPROSPER bool record_update_buffer(Anvil::CommandBufferBase &cmdBuffer,Buffer &buffer,uint64_t offset,uint64_t size,const void *data,Anvil::PipelineStageFlags dstStageMask=PIPELINE_STAGE_SHADER_INPUT_FLAGS,Anvil::AccessFlags dstAccessMask=Anvil::AccessFlagBits::SHADER_READ_BIT);
		template<typename T>
			bool record_update_buffer(Anvil::CommandBufferBase &cmdBuffer,Buffer &buffer,uint64_t offset,const T &data,Anvil::PipelineStageFlags dstStageMask=PIPELINE_STAGE_SHADER_INPUT_FLAGS,Anvil::AccessFlags dstAccessMask=Anvil::AccessFlagBits::SHADER_READ_BIT);
		DLLPROSPER bool record_blit_image(Anvil::CommandBufferBase &cmdBuffer,const BlitInfo &blitInfo,Anvil::Image &imgSrc,Anvil::Image &imgDst);
		DLLPROSPER bool record_resolve_image(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &imgSrc,Anvil::Image &imgDst);
		// The source texture image will be copied to the destination image using a resolve (if it's a MSAA texture) or a blit
		DLLPROSPER bool record_blit_texture(Anvil::CommandBufferBase &cmdBuffer,prosper::Texture &texSrc,Anvil::Image &imgDst);
		DLLPROSPER bool record_generate_mipmaps(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout currentLayout,Anvil::AccessFlags srcAccessMask,Anvil::PipelineStageFlags srcStage);
		DLLPROSPER bool record_pipeline_barrier(Anvil::CommandBufferBase &cmdBuffer,const PipelineBarrierInfo &barrierInfo);
		// Records an image barrier. If no layer is specified, ALL layers of the image will be included in the barrier.
		DLLPROSPER bool record_image_barrier(
			Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::PipelineStageFlags srcStageMask,Anvil::PipelineStageFlags dstStageMask,
			Anvil::ImageLayout oldLayout,Anvil::ImageLayout newLayout,Anvil::AccessFlags srcAccessMask,Anvil::AccessFlags dstAccessMask,
			uint32_t baseLayer=std::numeric_limits<uint32_t>::max()
		);
		struct BarrierImageLayout;
		DLLPROSPER bool record_image_barrier(
			Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,
			const ImageSubresourceRange &subresourceRange={}
		);
		DLLPROSPER bool record_image_barrier(
			Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout srcLayout,Anvil::ImageLayout dstLayout,const ImageSubresourceRange &subresourceRange={}
		);
		DLLPROSPER bool record_buffer_barrier(
			Anvil::CommandBufferBase &cmdBuffer,Buffer &buf,Anvil::PipelineStageFlags srcStageMask,Anvil::PipelineStageFlags dstStageMask,
			Anvil::AccessFlags srcAccessMask,Anvil::AccessFlags dstAccessMask,vk::DeviceSize offset=0ull,vk::DeviceSize size=std::numeric_limits<vk::DeviceSize>::max()
		);
		DLLPROSPER bool record_set_viewport(Anvil::CommandBufferBase &cmdBuffer,uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u);
		DLLPROSPER bool record_set_scissor(Anvil::CommandBufferBase &cmdBuffer,uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u);
		// If no render pass is specified, the render target's render pass will be used
		DLLPROSPER bool record_begin_render_pass(Anvil::PrimaryCommandBuffer &cmdBuffer,prosper::RenderTarget &rt,uint32_t layerId,const vk::ClearValue *clearValue=nullptr,prosper::RenderPass *rp=nullptr);
		DLLPROSPER bool record_begin_render_pass(Anvil::PrimaryCommandBuffer &cmdBuffer,prosper::RenderTarget &rt,uint32_t layerId,const std::vector<vk::ClearValue> &clearValues,prosper::RenderPass *rp=nullptr);
		DLLPROSPER bool record_begin_render_pass(Anvil::PrimaryCommandBuffer &cmdBuffer,prosper::RenderTarget &rt,const vk::ClearValue *clearValue=nullptr,prosper::RenderPass *rp=nullptr);
		DLLPROSPER bool record_begin_render_pass(Anvil::PrimaryCommandBuffer &cmdBuffer,prosper::RenderTarget &rt,const std::vector<vk::ClearValue> &clearValues,prosper::RenderPass *rp=nullptr);
		DLLPROSPER bool record_end_render_pass(Anvil::PrimaryCommandBuffer &cmdBuffer);
		DLLPROSPER bool record_next_sub_pass(Anvil::PrimaryCommandBuffer &cmdBuffer);

		DLLPROSPER prosper::RenderTarget *get_current_render_pass_target(Anvil::PrimaryCommandBuffer &cmdBuffer);

		DLLPROSPER bool set_descriptor_set_binding_texture(Anvil::DescriptorSet &descSet,prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId);
		DLLPROSPER bool set_descriptor_set_binding_texture(Anvil::DescriptorSet &descSet,prosper::Texture &texture,uint32_t bindingIdx);
		DLLPROSPER bool set_descriptor_set_binding_array_texture(Anvil::DescriptorSet &descSet,prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex,uint32_t layerId);
		DLLPROSPER bool set_descriptor_set_binding_array_texture(Anvil::DescriptorSet &descSet,prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex);
		DLLPROSPER bool set_descriptor_set_binding_uniform_buffer(Anvil::DescriptorSet &descSet,prosper::Buffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max());
		DLLPROSPER bool set_descriptor_set_binding_dynamic_uniform_buffer(Anvil::DescriptorSet &descSet,prosper::Buffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max());
		DLLPROSPER bool set_descriptor_set_binding_storage_buffer(Anvil::DescriptorSet &descSet,prosper::Buffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max());

		DLLPROSPER Anvil::Image *get_descriptor_set_image(Anvil::DescriptorSet &descSet,uint32_t bindingIdx=0u,uint32_t arrayIndex=0u);

		struct DLLPROSPER BarrierImageLayout
		{
			BarrierImageLayout(Anvil::PipelineStageFlags pipelineStageFlags,Anvil::ImageLayout imageLayout,Anvil::AccessFlags accessFlags);
			BarrierImageLayout()=default;
			Anvil::PipelineStageFlags stageMask = {};
			Anvil::ImageLayout layout = {};
			Anvil::AccessFlags accessMask = {};
		};

		struct DLLPROSPER BufferBarrierInfo
		{
			Anvil::AccessFlags srcAccessMask = {};
			Anvil::AccessFlags dstAccessMask = {};
			vk::DeviceSize offset = 0ull;
			vk::DeviceSize size = std::numeric_limits<vk::DeviceSize>::max();
		};

		struct DLLPROSPER ImageBarrierInfo
		{
			Anvil::AccessFlags srcAccessMask = {};
			Anvil::AccessFlags dstAccessMask = {};
			Anvil::ImageLayout oldLayout = Anvil::ImageLayout::GENERAL;
			Anvil::ImageLayout newLayout = Anvil::ImageLayout::GENERAL;
			uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			ImageSubresourceRange subresourceRange = {};
		};

		DLLPROSPER Anvil::BufferBarrier create_buffer_barrier(const prosper::util::BufferBarrierInfo &barrierInfo,Buffer &buffer);
		DLLPROSPER Anvil::ImageBarrier create_image_barrier(Anvil::Image &img,const prosper::util::ImageBarrierInfo &barrierInfo);
		DLLPROSPER Anvil::ImageBarrier create_image_barrier(Anvil::Image &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,const ImageSubresourceRange &subresourceRange={});

		DLLPROSPER uint32_t calculate_buffer_alignment(Anvil::BaseDevice &dev,Anvil::BufferUsageFlags usageFlags);
		DLLPROSPER void calculate_mipmap_size(uint32_t w,uint32_t h,uint32_t *wMipmap,uint32_t *hMipmap,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_size(uint32_t v,uint32_t level);
		DLLPROSPER uint32_t calculate_mipmap_count(uint32_t w,uint32_t h);
		DLLPROSPER std::string to_string(vk::Result r);
		DLLPROSPER std::string to_string(vk::DebugReportFlagsEXT flags);
		DLLPROSPER std::string to_string(vk::DebugReportObjectTypeEXT type);
		DLLPROSPER std::string to_string(vk::DebugReportFlagBitsEXT flags);
		DLLPROSPER std::string to_string(Anvil::Format format);
		DLLPROSPER std::string to_string(Anvil::ShaderStageFlagBits shaderStage);
		DLLPROSPER std::string to_string(Anvil::ImageType type);
		DLLPROSPER std::string to_string(Anvil::ImageTiling tiling);
		DLLPROSPER std::string to_string(Anvil::ImageUsageFlags usage);
		DLLPROSPER std::string to_string(Anvil::ImageCreateFlags createFlags);
		DLLPROSPER std::string to_string(Anvil::ImageUsageFlagBits usage);
		DLLPROSPER std::string to_string(Anvil::ImageCreateFlagBits createFlags);
		DLLPROSPER std::string to_string(Anvil::ShaderStage stage);
		DLLPROSPER std::string to_string(vk::PhysicalDeviceType type);
		DLLPROSPER bool has_alpha(Anvil::Format format);
		DLLPROSPER bool is_depth_format(Anvil::Format format);
		DLLPROSPER bool is_compressed_format(Anvil::Format format);
		DLLPROSPER bool is_uncompressed_format(Anvil::Format format);
		DLLPROSPER uint32_t get_bit_size(Anvil::Format format);
		DLLPROSPER uint32_t get_byte_size(Anvil::Format format);
		DLLPROSPER Anvil::AccessFlags get_read_access_mask();
		DLLPROSPER Anvil::AccessFlags get_write_access_mask();
		DLLPROSPER Anvil::AccessFlags get_image_read_access_mask();
		DLLPROSPER Anvil::AccessFlags get_image_write_access_mask();
		DLLPROSPER Anvil::PipelineBindPoint get_pipeline_bind_point(Anvil::ShaderStageFlags shaderStages);
		DLLPROSPER Anvil::ImageAspectFlagBits get_aspect_mask(Anvil::Image &img);
		DLLPROSPER Anvil::ImageAspectFlagBits get_aspect_mask(Anvil::Format format);
		DLLPROSPER std::pair<const Anvil::MemoryType*,MemoryFeatureFlags> find_compatible_memory_type(Anvil::BaseDevice &dev,MemoryFeatureFlags featureFlags);

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
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::MemoryFeatureFlags);
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::QueueFamilyFlags);
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::BufferCreateInfo::Flags);
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::ImageCreateInfo::Flags);
#pragma warning(pop)

template<typename T>
	bool prosper::util::record_update_buffer(Anvil::CommandBufferBase &cmdBuffer,Buffer &buffer,uint64_t offset,const T &data,Anvil::PipelineStageFlags dstStageMask,Anvil::AccessFlags dstAccessMask)
{
	return record_update_buffer(cmdBuffer,buffer,offset,sizeof(data),&data,dstStageMask,dstAccessMask);
}

template<class T>
	std::shared_ptr<T> prosper::util::unique_ptr_to_shared_ptr(std::unique_ptr<T,std::function<void(T*)>> v)
{
	return (v == nullptr) ? std::shared_ptr<T>(nullptr) : std::shared_ptr<T>(v.release(),v.get_deleter());
}

#endif
