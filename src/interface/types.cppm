// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


export module pragma.prosper:types;

export import std.compat;

export namespace prosper {
	class BasePipelineCreateInfo;
	class BlurSet;
	class CommandBuffer;
	class CommonBufferCache;
	class ComputePipelineCreateInfo;
	class ContextObject;
	class DescriptorArrayManager;
	class DescriptorSetBinding;
	class DescriptorSetBindingArrayTexture;
	class DescriptorSetBindingBaseBuffer;
	class DescriptorSetBindingDynamicStorageBuffer;
	class DescriptorSetBindingDynamicUniformBuffer;
	class DescriptorSetBindingStorageBuffer;
	class DescriptorSetBindingStorageImage;
	class DescriptorSetBindingTexture;
	class DescriptorSetBindingUniformBuffer;
	class DescriptorSetCreateInfo;
	class GraphicsPipelineCreateInfo;
	class IBuffer;
	class ICommandBuffer;
	class ICommandBufferPool;
	class IDescriptorSet;
	class IDescriptorSetGroup;
	class IDynamicResizableBuffer;
	class IEvent;
	class IFramebuffer;
	class IImage;
	class IImageView;
	class IPrimaryCommandBuffer;
	class IPrContext;
	class IQueryPool;
	class IRenderBuffer;
	class IRenderPass;
	class ISampler;
	class ISecondaryCommandBuffer;
	class IShaderPipelineLayout;
	class ISwapCommandBufferGroup;
	class IUniformResizableBuffer;
	class MSAATexture;
	class MtSwapCommandBufferGroup;
	class OcclusionQuery;
	class PipelineStatisticsQuery;
	class Profiler;
	class Query;
	class RayTracingPipelineCreateInfo;
	class RenderTarget;
	class Shader;
	class ShaderBaseImageProcessing;
	class ShaderBlurBase;
	class ShaderBlurH;
	class ShaderBlurV;
	class ShaderCopyImage;
	class ShaderGraphics;
	class ShaderManager;
	class ShaderPipelineLoader;
	class ShaderRect;
	class ShaderStageProgram;
	class ShaderGraphics;
	class ShaderCompute;
	class ShaderRaytracing;
	class StSwapCommandBufferGroup;
	class SwapBuffer;
	class SwapDescriptorSet;
	class Texture;
	class TimestampQuery;
	class TimerQuery;
	class VlkPipelineManager;
	class Window;

	enum class AttachmentLoadOp : uint8_t;
	enum class AttachmentStoreOp : uint8_t;
	enum class AttachmentType : uint8_t;
	enum class BlendFactor : uint8_t;
	enum class BlendOp : uint8_t;
	enum class BorderColor : uint32_t;
	enum class CompareOp : uint32_t;
	enum class ComponentSwizzle : uint8_t;
	enum class CullModeFlags : uint8_t;
	enum class DescriptorBindingFlags : uint32_t;
	enum class DescriptorType : uint32_t;
	enum class DynamicState : uint32_t;
	enum class FeatureSupport : uint8_t;
	enum class Filter : uint32_t;
	enum class Format : uint32_t;
	enum class FormatFeatureFlags : uint16_t;
	enum class FrontFace : uint8_t;
	enum class ImageAspectFlags : uint32_t;
	enum class ImageCreateFlags : uint32_t;
	enum class ImageLayout : uint32_t;
	enum class ImageTiling : uint8_t;
	enum class ImageType : uint8_t;
	enum class ImageUsageFlags : uint32_t;
	enum class IndexType : uint8_t;
	enum class LogicOp : uint8_t;
	enum class MemoryFeatureFlags : uint32_t;
	enum class MemoryPropertyFlags : uint32_t;
	enum class PhysicalDeviceType : uint8_t;
	enum class PipelineBindPoint : uint32_t;
	enum class PipelineCreateFlags : uint32_t;
	enum class PipelineStageFlags : uint32_t;
	enum class PolygonMode : uint8_t;
	enum class PresentModeKHR : uint32_t;
	enum class PrimitiveTopology : uint8_t;
	enum class QueryPipelineStatisticFlags : uint32_t;
	enum class QueryResultFlags : uint8_t;
	enum class QueryType : uint32_t;
	enum class QueueFamilyFlags : uint32_t;
	enum class QueueFamilyType : uint8_t;
	enum class Result : int32_t;
	enum class SampleCountFlags : uint32_t;
	enum class SamplerAddressMode : uint32_t;
	enum class SamplerMipmapMode : uint32_t;
	enum class ShaderStage : uint8_t;
	enum class ShaderStageFlags : uint32_t;
	enum class SharingMode : uint8_t;
	enum class StencilFaceFlags : uint8_t;
	enum class Vendor : uint32_t;

	enum StencilOp : uint8_t;
	struct Callbacks;
	struct ClearDepthStencilValue;
	struct DescriptorSetInfo;
	struct Extent2D;
	struct Extent3D;
	struct IndexBufferInfo;
	struct MemoryRequirements;
	struct Offset2D;
	struct Offset3D;
	struct PipelineInfo;
	struct PipelineLayout;
	struct PipelineStatistics;
	struct PushConstantRange;
	struct Rect2D;
	struct ShaderBindState;
	struct ShaderModuleStageEntryPoint;
	struct ShaderPipeline;
	struct ShaderStageData;
	struct SpecializationConstant;
	struct StencilOpState;
	struct SubresourceLayout;
	struct VertexInputAttribute;
	struct Viewport;
	struct WindowSettings;
	using BindingIndex = uint32_t;
	using FrameIndex = uint64_t;
	using PipelineID = uint32_t;
	using SampleMask = uint32_t;
	using ShaderIndex = uint32_t;
	using SubPassAttachmentID = uint32_t;
	using SubPassID = uint32_t;

	namespace detail {
		struct BufferUpdateInfo;
		struct DescriptorSetInfoBinding;
	}

	namespace glsl {
		struct Definitions;
		struct IncludeLine;
	}

	namespace util {
		class PreparedCommandBuffer;
		enum class DynamicStateFlags : uint32_t;
		struct BarrierImageLayout;
		struct BlitInfo;
		struct BufferBarrier;
		struct BufferBarrierInfo;
		struct BufferCopy;
		struct BufferCreateInfo;
		struct BufferImageCopyInfo;
		struct ClearImageInfo;
		struct CopyInfo;
		struct ImageBarrier;
		struct ImageBarrierInfo;
		struct ImageCreateInfo;
		struct ImageResolve;
		struct ImageSubresourceLayers;
		struct ImageSubresourceRange;
		struct ImageViewCreateInfo;
		struct Limits;
		struct PhysicalDeviceImageFormatProperties;
		struct PhysicalDeviceMemoryProperties;
		struct PipelineBarrierInfo;
		struct PreparedCommand;
		struct PreparedCommandArgumentMap;
		struct PreparedCommandBufferRecordState;
		struct PreparedCommandBufferUserData;
		struct RenderPassCreateInfo;
		struct RenderTargetCreateInfo;
		struct SamplerCreateInfo;
		struct ShaderInfo;
		struct SubresourceLayout;
		struct TextureCreateInfo;
		struct VendorDeviceInfo;
	}
}
