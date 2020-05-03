/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_STRUCTS_HPP__
#define __PROSPER_STRUCTS_HPP__

#include "prosper_definitions.hpp"
#include "prosper_enums.hpp"
#include <limits>
#include <cinttypes>
#include <optional>

#undef max

namespace prosper
{
	struct DLLPROSPER Extent2D
	{
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct DLLPROSPER Extent3D
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	struct DLLPROSPER Offset3D
	{
		Offset3D(int32_t x=0,int32_t y=0,int32_t z=0)
			: x{x},y{y},z{z}
		{}

		int32_t x;
		int32_t y;
		int32_t z;
	};

	class IImage;
	class IBuffer;
	class ISampler;
	class IImageView;
	namespace util
	{
		struct DLLPROSPER BufferCopy
		{
			DeviceSize srcOffset = 0;
			DeviceSize dstOffset = 0;
			DeviceSize size = 0;
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
			uint32_t baseMipLevel = 0u;
			uint32_t levelCount = std::numeric_limits<uint32_t>::max();
			uint32_t baseArrayLayer = 0u;
			uint32_t layerCount = std::numeric_limits<uint32_t>::max();
		};

		struct DLLPROSPER ClearImageInfo
		{
			ImageSubresourceRange subresourceRange = {};
		};

		struct DLLPROSPER ImageSubresourceLayers
		{
			ImageSubresourceLayers(ImageAspectFlags aspectMask=ImageAspectFlags::ColorBit,uint32_t mipLevel=0,uint32_t baseArrayLayer=0,uint32_t layerCount=1)
				: aspectMask{aspectMask},mipLevel{mipLevel},baseArrayLayer{baseArrayLayer},layerCount{layerCount}
			{}
			ImageAspectFlags aspectMask = ImageAspectFlags::ColorBit;
			uint32_t mipLevel = 0;
			uint32_t baseArrayLayer = 0;
			uint32_t layerCount = 1;
		};

		struct DLLPROSPER CopyInfo
		{
			uint32_t width = std::numeric_limits<uint32_t>::max();
			uint32_t height = std::numeric_limits<uint32_t>::max();
			ImageSubresourceLayers srcSubresource = {ImageAspectFlags::ColorBit,0u,0u,1u};
			ImageSubresourceLayers dstSubresource = {ImageAspectFlags::ColorBit,0u,0u,1u};

			vk::Offset3D srcOffset = vk::Offset3D(0,0,0);
			vk::Offset3D dstOffset = vk::Offset3D(0,0,0);

			ImageLayout srcImageLayout = ImageLayout::TransferSrcOptimal;
			ImageLayout dstImageLayout = ImageLayout::TransferDstOptimal;
		};

		struct DLLPROSPER BufferImageCopyInfo
		{
			vk::DeviceSize bufferOffset = 0ull;
			std::optional<uint32_t> width = {};
			std::optional<uint32_t> height = {};
			uint32_t mipLevel = 0u;
			uint32_t baseArrayLayer = 0u;
			uint32_t layerCount = 1u;
			ImageAspectFlags aspectMask = ImageAspectFlags::ColorBit;
			ImageLayout dstImageLayout = ImageLayout::TransferDstOptimal;
		};

		struct DLLPROSPER BlitInfo
		{
			ImageSubresourceLayers srcSubresourceLayer = {
				ImageAspectFlags::ColorBit,0u,0u,1u
			};
			ImageSubresourceLayers dstSubresourceLayer = {
				ImageAspectFlags::ColorBit,0u,0u,1u
			};

			std::array<int32_t,2> offsetSrc = {0,0};
			std::optional<Extent2D> extentsSrc = {};

			std::array<int32_t,2> offsetDst = {0,0};
			std::optional<Extent2D> extentsDst = {};
		};

		struct DLLPROSPER BufferBarrier
		{
			AccessFlags dstAccessMask;
			AccessFlags srcAccessMask;

			IBuffer *buffer;
			uint32_t dstQueueFamilyIndex;
			DeviceSize offset;
			DeviceSize size;
			uint32_t srcQueueFamilyIndex;

			BufferBarrier(
				AccessFlags in_source_access_mask,
				AccessFlags in_destination_access_mask,
				uint32_t in_src_queue_family_index,
				uint32_t in_dst_queue_family_index,
				IBuffer *in_buffer_ptr,
				DeviceSize in_offset,
				DeviceSize in_size
			)
				: srcAccessMask{in_source_access_mask},dstAccessMask{in_destination_access_mask},
				srcQueueFamilyIndex{in_src_queue_family_index},dstQueueFamilyIndex{in_dst_queue_family_index},
				buffer{in_buffer_ptr},offset{in_offset},size{in_size}
			{}
		};

		struct DLLPROSPER ImageBarrier
		{
			AccessFlags dstAccessMask;
			AccessFlags srcAccessMask;

			uint32_t dstQueueFamilyIndex;
			IImage *image;
			ImageLayout newLayout;
			ImageLayout oldLayout;
			uint32_t srcQueueFamilyIndex;
			ImageSubresourceRange subresourceRange;

			ImageBarrier(
				AccessFlags in_source_access_mask,
				AccessFlags in_destination_access_mask,
				ImageLayout in_old_layout,
				ImageLayout in_new_layout,
				uint32_t in_src_queue_family_index,
				uint32_t in_dst_queue_family_index,
				IImage *in_image_ptr,
				ImageSubresourceRange in_image_subresource_range
			)
				: srcAccessMask{in_source_access_mask},dstAccessMask{in_destination_access_mask},oldLayout{in_old_layout},
				newLayout{in_new_layout},srcQueueFamilyIndex{in_src_queue_family_index},dstQueueFamilyIndex{in_dst_queue_family_index},
				image{in_image_ptr},subresourceRange{in_image_subresource_range}
			{}
		};

		struct DLLPROSPER PipelineBarrierInfo
		{
			PipelineStageFlags srcStageMask = PipelineStageFlags::AllCommandsBit;
			PipelineStageFlags dstStageMask = PipelineStageFlags::AllCommandsBit;
			std::vector<BufferBarrier> bufferBarriers;
			std::vector<ImageBarrier> imageBarriers;

			PipelineBarrierInfo()=default;
			PipelineBarrierInfo &operator=(const PipelineBarrierInfo&)=delete;
		};

		struct DLLPROSPER BarrierImageLayout
		{
			BarrierImageLayout(PipelineStageFlags pipelineStageFlags,ImageLayout imageLayout,AccessFlags accessFlags);
			BarrierImageLayout()=default;
			PipelineStageFlags stageMask = {};
			ImageLayout layout = {};
			AccessFlags accessMask = {};
		};

		struct DLLPROSPER BufferBarrierInfo
		{
			AccessFlags srcAccessMask = {};
			AccessFlags dstAccessMask = {};
			vk::DeviceSize offset = 0ull;
			vk::DeviceSize size = std::numeric_limits<vk::DeviceSize>::max();
		};

		struct DLLPROSPER ImageBarrierInfo
		{
			AccessFlags srcAccessMask = {};
			AccessFlags dstAccessMask = {};
			ImageLayout oldLayout = ImageLayout::General;
			ImageLayout newLayout = ImageLayout::General;
			uint32_t srcQueueFamilyIndex = QUEUE_FAMILY_IGNORED;
			uint32_t dstQueueFamilyIndex = QUEUE_FAMILY_IGNORED;

			ImageSubresourceRange subresourceRange = {};
		};

		struct DLLPROSPER ImageResolve
		{
			ImageSubresourceLayers src_subresource;
			Offset3D src_offset;
			ImageSubresourceLayers dst_subresource;
			Offset3D dst_offset;
			Extent3D extent;
		};

		struct DLLPROSPER SubresourceLayout
		{
			DeviceSize offset;
			DeviceSize size;
			DeviceSize row_pitch;
			DeviceSize array_pitch;
			DeviceSize depth_pitch;
		};

		struct DLLPROSPER ImageCreateInfo
		{
			ImageType type = ImageType::e2D;
			uint32_t width = 0u;
			uint32_t height = 0u;
			Format format = Format::R8G8B8A8_UNorm;
			uint32_t layers = 1u;
			ImageUsageFlags usage = ImageUsageFlags::ColorAttachmentBit;
			SampleCountFlags samples = SampleCountFlags::e1Bit;
			ImageTiling tiling = ImageTiling::Optimal;
			ImageLayout postCreateLayout = ImageLayout::ColorAttachmentOptimal;

			enum class Flags : uint32_t
			{
				None = 0u,
				Cubemap = 1u,
				ConcurrentSharing = Cubemap<<1u, // Exclusive sharing is used if this flag is not set
				FullMipmapChain = ConcurrentSharing<<1u,

				Sparse = FullMipmapChain<<1u,
				SparseAliasedResidency = Sparse<<1u, // Only has an effect if Sparse-flag is set

				AllocateDiscreteMemory = SparseAliasedResidency<<1u,
				DontAllocateMemory = AllocateDiscreteMemory<<1u
			};
			Flags flags = Flags::None;
			QueueFamilyFlags queueFamilyMask = QueueFamilyFlags::GraphicsBit;
			MemoryFeatureFlags memoryFeatures = MemoryFeatureFlags::GPUBulk;
		};

		struct DLLPROSPER TextureCreateInfo
		{
			TextureCreateInfo(IImageView &imgView,ISampler *smp=nullptr);
			TextureCreateInfo()=default;
			std::shared_ptr<ISampler> sampler = nullptr;
			std::shared_ptr<IImageView> imageView = nullptr;

			enum class Flags : uint32_t
			{
				None = 0u,
				Resolvable = 1u, // If this flag is set, a MSAATexture instead of a regular texture will be created, unless the image has a sample count of 1
				CreateImageViewForEachLayer = Resolvable<<1u
			};
			Flags flags = Flags::None;
		};

		struct DLLPROSPER ImageViewCreateInfo
		{
			std::optional<uint32_t> baseLayer = {};
			uint32_t levelCount = 1u;
			uint32_t baseMipmap = 0u;
			uint32_t mipmapLevels = std::numeric_limits<uint32_t>::max();
			Format format = Format::Unknown;
			ComponentSwizzle swizzleRed = ComponentSwizzle::R;
			ComponentSwizzle swizzleGreen = ComponentSwizzle::G;
			ComponentSwizzle swizzleBlue = ComponentSwizzle::B;
			ComponentSwizzle swizzleAlpha = ComponentSwizzle::A;
		};

		struct DLLPROSPER SamplerCreateInfo
		{
			Filter minFilter = Filter::Linear;
			Filter magFilter = Filter::Linear;
			SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
			SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
			SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
			SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
			float mipLodBias = 0.f;
			float maxAnisotropy = std::numeric_limits<float>::max();
			bool compareEnable = false;
			CompareOp compareOp = CompareOp::Less;
			float minLod = 0.f;
			float maxLod = std::numeric_limits<float>::max();
			BorderColor borderColor = BorderColor::FloatTransparentBlack;
			bool useUnnormalizedCoordinates = false;
		};

		struct DLLPROSPER RenderTargetCreateInfo
		{
			bool useLayerFramebuffers = false;
		};

		struct DLLPROSPER RenderPassCreateInfo
		{
			struct DLLPROSPER AttachmentInfo
			{
				AttachmentInfo(
					prosper::Format format=prosper::Format::R8G8B8A8_UNorm,prosper::ImageLayout initialLayout=prosper::ImageLayout::ColorAttachmentOptimal,
					prosper::AttachmentLoadOp loadOp=prosper::AttachmentLoadOp::DontCare,prosper::AttachmentStoreOp storeOp=prosper::AttachmentStoreOp::Store,
					prosper::SampleCountFlags sampleCount=prosper::SampleCountFlags::e1Bit,prosper::ImageLayout finalLayout=prosper::ImageLayout::ShaderReadOnlyOptimal
				);
				bool operator==(const AttachmentInfo &other) const;
				bool operator!=(const AttachmentInfo &other) const;
				prosper::Format format = prosper::Format::R8G8B8A8_UNorm;
				prosper::SampleCountFlags sampleCount = prosper::SampleCountFlags::e1Bit;
				prosper::AttachmentLoadOp loadOp = prosper::AttachmentLoadOp::DontCare;
				prosper::AttachmentStoreOp storeOp = prosper::AttachmentStoreOp::Store;
				prosper::ImageLayout initialLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				prosper::ImageLayout finalLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;
			};
			struct DLLPROSPER SubPass
			{
				struct DLLPROSPER Dependency
				{
					Dependency(
						std::size_t sourceSubPassId,std::size_t destinationSubPassId,
						prosper::PipelineStageFlags sourceStageMask,prosper::PipelineStageFlags destinationStageMask,
						prosper::AccessFlags sourceAccessMask,prosper::AccessFlags destinationAccessMask
					);
					bool operator==(const Dependency &other) const;
					bool operator!=(const Dependency &other) const;
					std::size_t sourceSubPassId;
					std::size_t destinationSubPassId;
					prosper::PipelineStageFlags sourceStageMask;
					prosper::PipelineStageFlags destinationStageMask;
					prosper::AccessFlags sourceAccessMask;
					prosper::AccessFlags destinationAccessMask;
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
	};

	class DLLPROSPER DescriptorSetCreateInfo
	{
	public:
		struct DLLPROSPER Binding
		{
			Binding()=default;
			Binding(
				uint32_t descriptorArraySize,
				DescriptorType descriptorType,
				ShaderStageFlags stageFlags,
				const ISampler * const* immutableSamplerPtrs,
				DescriptorBindingFlags flags
			)
				: descriptorArraySize{descriptorArraySize},descriptorType{descriptorType},
				stageFlags{stageFlags},flags{flags}
			{
				if(immutableSamplerPtrs != nullptr)
				{
					for(uint32_t n_sampler = 0;
						n_sampler < descriptorArraySize;
						++n_sampler)
					{
						immutableSamplers.push_back(immutableSamplerPtrs[n_sampler]);
					}
				}
			}
			uint32_t descriptorArraySize = 0;
			DescriptorType descriptorType = DescriptorType::Unknown;
			DescriptorBindingFlags flags {};
			std::vector<const ISampler*> immutableSamplers {};
			ShaderStageFlags stageFlags {};
			bool operator==(const Binding &binding) const
			{
				return binding.descriptorArraySize == descriptorArraySize &&
					binding.descriptorType == descriptorType &&
					binding.flags == flags &&
					binding.immutableSamplers == immutableSamplers &&
					binding.stageFlags == stageFlags;
			}
			bool operator!=(const Binding &binding) const {return !operator==(binding);}
		};

		bool GetBindingPropertiesByBindingIndex(
			uint32_t bindingIndex,
			DescriptorType *outOptDescriptorType=nullptr,
			uint32_t *outOptDescriptorArraySize=nullptr,
			ShaderStageFlags *outOptStageFlags=nullptr,
			bool *outOptImmutableSamplersEnabled=nullptr,
			DescriptorBindingFlags *outOptFlags=nullptr
		) const;
		bool GetBindingPropertiesByIndexNumber(
			uint32_t nBinding,
			DescriptorType *outOptDescriptorType=nullptr,
			uint32_t *outOptDescriptorArraySize=nullptr,
			ShaderStageFlags *outOptStageFlags=nullptr,
			bool *outOptImmutableSamplersEnabled=nullptr,
			DescriptorBindingFlags *outOptFlags=nullptr
		);
		uint32_t GetBindingCount() const {return static_cast<uint32_t>(m_bindings.size());}
	private:
		using BindingIndexToBindingMap = std::map<BindingIndex,Binding>;

		DescriptorSetCreateInfo();

		BindingIndexToBindingMap m_bindings {};

		uint32_t m_numVariableDescriptorCountBinding;
		uint32_t m_variableDescriptorCountBindingSize;
	};
};

#endif
