/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_ENUMS_HPP__
#define __PROSPER_ENUMS_HPP__

#include <mathutil/umath.h>

namespace prosper
{
	constexpr uint32_t QUEUE_FAMILY_IGNORED = ~0u;
	enum class QueueFamilyFlags : uint32_t
	{
		None = 0u,
		GraphicsBit = 1u,
		ComputeBit = GraphicsBit<<1u,
		DMABit = ComputeBit<<1u,
	};

	enum class BufferUsageFlags
	{
		// These map to Anvil/Vulkan enums
		None = 0,
		IndexBufferBit = 0x00000040,
		IndirectBufferBit = 0x00000100,
		StorageBufferBit = 0x00000020,
		StorageTexelBufferBit = 0x00000008,
		TransferDstBit = 0x00000002,
		TransferSrcBit = 0x00000001,
		UniformBufferBit = 0x00000010,
		UniformTexelBufferBit = 0x00000004,
		VertexBufferBit = 0x00000080
	};

	enum class MemoryFeatureFlags : uint32_t
	{
		// Note: These flags are merely hints and may be ignored or changed by the API if they're not supported by the GPU!
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

	enum class Filter : uint32_t
	{
		Nearest = 0,
		Linear = 1
	};

	enum class SamplerMipmapMode : uint32_t
	{
		Nearest = 0,
		Linear = 1
	};

	enum class SamplerAddressMode : uint32_t
	{
		Repeat = 0,
		MirroredRepeat = 1,
		ClampToEdge = 2,
		ClampToBorder = 3,
		MirrorClampToEdge = 4
	};

	enum class CompareOp : uint32_t
	{
		Never = 0,
		Less = 1,
		Equal = 2,
		LessOrEqual = 3,
		Greater = 4,
		NotEqual = 5,
		GreaterOrEqual = 6,
		Always = 7
	};

	enum class BorderColor : uint32_t
	{
		FloatTransparentBlack = 0,
		IntTransparentBlack = 1,
		FloatOpaqueBlack = 2,
		IntOpaqueBlack = 3,
		FloatOpaqueWhite = 4,
		IntOpaqueWhite = 5
	};

	enum class ImageType : uint8_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2
	};

	enum class ImageUsageFlags : uint32_t
	{
		None = 0,
		TransferDstBit = 0x00000002,
		TransferSrcBit = 0x00000001,
		SampledBit = 0x00000004,
		StorageBit = 0x00000008,
		ColorAttachmentBit = 0x00000010,
		DepthStencilAttachmentBit = 0x00000020,
		TransientAttachmentBit = 0x00000040,
		InputAttachmentBit = 0x00000080
	};

	enum class ImageTiling : uint8_t
	{
		Optimal = 0,
		Linear = 1
	};

	enum class Format : uint32_t
	{
		// Commented formats have poor coverage (<90%, see https://vulkan.gpuinfo.org/listformats.php)
		Unknown = 0,
		R4G4_UNorm_Pack8 = 1,
		R4G4B4A4_UNorm_Pack16 = 2,
		B4G4R4A4_UNorm_Pack16 = 3,
		R5G6B5_UNorm_Pack16 = 4,
		B5G6R5_UNorm_Pack16 = 5,
		R5G5B5A1_UNorm_Pack16 = 6,
		B5G5R5A1_UNorm_Pack16 = 7,
		A1R5G5B5_UNorm_Pack16 = 8,
		R8_UNorm = 9,
		R8_SNorm = 10,
		R8_UScaled_PoorCoverage = 11,
		R8_SScaled_PoorCoverage = 12,
		R8_UInt = 13,
		R8_SInt = 14,
		R8_SRGB = 15,
		R8G8_UNorm = 16,
		R8G8_SNorm = 17,
		R8G8_UScaled_PoorCoverage = 18,
		R8G8_SScaled_PoorCoverage = 19,
		R8G8_UInt = 20,
		R8G8_SInt = 21,
		R8G8_SRGB_PoorCoverage = 22,
		R8G8B8_UNorm_PoorCoverage = 23,
		R8G8B8_SNorm_PoorCoverage = 24,
		R8G8B8_UScaled_PoorCoverage = 25,
		R8G8B8_SScaled_PoorCoverage = 26,
		R8G8B8_UInt_PoorCoverage = 27,
		R8G8B8_SInt_PoorCoverage = 28,
		R8G8B8_SRGB_PoorCoverage = 29,
		B8G8R8_UNorm_PoorCoverage = 30,
		B8G8R8_SNorm_PoorCoverage = 31,
		B8G8R8_UScaled_PoorCoverage = 32,
		B8G8R8_SScaled_PoorCoverage = 33,
		B8G8R8_UInt_PoorCoverage = 34,
		B8G8R8_SInt_PoorCoverage = 35,
		B8G8R8_SRGB_PoorCoverage = 36, // Coverage unknown
		R8G8B8A8_UNorm = 37,
		R8G8B8A8_SNorm = 38,
		R8G8B8A8_UScaled_PoorCoverage = 39,
		R8G8B8A8_SScaled_PoorCoverage = 40,
		R8G8B8A8_UInt = 41,
		R8G8B8A8_SInt = 42,
		R8G8B8A8_SRGB = 43,
		B8G8R8A8_UNorm = 44,
		B8G8R8A8_SNorm = 45,
		B8G8R8A8_UScaled_PoorCoverage = 46,
		B8G8R8A8_SScaled_PoorCoverage = 47,
		B8G8R8A8_UInt = 48,
		B8G8R8A8_SInt = 49,
		B8G8R8A8_SRGB = 50,
		A8B8G8R8_UNorm_Pack32 = 51,
		A8B8G8R8_SNorm_Pack32 = 52,
		A8B8G8R8_UScaled_Pack32_PoorCoverage = 53,
		A8B8G8R8_SScaled_Pack32_PoorCoverage = 54,
		A8B8G8R8_UInt_Pack32 = 55,
		A8B8G8R8_SInt_Pack32 = 56,
		A8B8G8R8_SRGB_Pack32 = 57,
		A2R10G10B10_UNorm_Pack32 = 58,
		A2R10G10B10_SNorm_Pack32_PoorCoverage = 59,
		A2R10G10B10_UScaled_Pack32_PoorCoverage = 60,
		A2R10G10B10_SScaled_Pack32_PoorCoverage = 61,
		A2R10G10B10_UInt_Pack32 = 62,
		A2R10G10B10_SInt_Pack32_PoorCoverage = 63,
		A2B10G10R10_UNorm_Pack32 = 64,
		A2B10G10R10_SNorm_Pack32_PoorCoverage = 65,
		A2B10G10R10_UScaled_Pack32_PoorCoverage = 66,
		A2B10G10R10_SScaled_Pack32_PoorCoverage = 67,
		A2B10G10R10_UInt_Pack32 = 68,
		A2B10G10R10_SInt_Pack32_PoorCoverage = 69,
		R16_UNorm = 70,
		R16_SNorm = 71,
		R16_UScaled_PoorCoverage = 72,
		R16_SScaled_PoorCoverage = 73,
		R16_UInt = 74,
		R16_SInt = 75,
		R16_SFloat = 76,
		R16G16_UNorm = 77,
		R16G16_SNorm = 78,
		R16G16_UScaled_PoorCoverage = 79,
		R16G16_SScaled_PoorCoverage = 80,
		R16G16_UInt = 81,
		R16G16_SInt = 82,
		R16G16_SFloat = 83,
		R16G16B16_UNorm_PoorCoverage = 84,
		R16G16B16_SNorm_PoorCoverage = 85,
		R16G16B16_UScaled_PoorCoverage = 86,
		R16G16B16_SScaled_PoorCoverage = 87,
		R16G16B16_UInt_PoorCoverage = 88,
		R16G16B16_SInt_PoorCoverage = 89,
		R16G16B16_SFloat_PoorCoverage = 90,
		R16G16B16A16_UNorm = 91,
		R16G16B16A16_SNorm = 92,
		R16G16B16A16_UScaled_PoorCoverage = 93,
		R16G16B16A16_SScaled_PoorCoverage = 94,
		R16G16B16A16_UInt = 95,
		R16G16B16A16_SInt = 96,
		R16G16B16A16_SFloat = 97,
		R32_UInt = 98,
		R32_SInt = 99,
		R32_SFloat = 100,
		R32G32_UInt = 101,
		R32G32_SInt = 102,
		R32G32_SFloat = 103,
		R32G32B32_UInt = 104,
		R32G32B32_SInt = 105,
		R32G32B32_SFloat = 106,
		R32G32B32A32_UInt = 107,
		R32G32B32A32_SInt = 108,
		R32G32B32A32_SFloat = 109,
		R64_UInt_PoorCoverage = 110,
		R64_SInt_PoorCoverage = 111,
		R64_SFloat_PoorCoverage = 112,
		R64G64_UInt_PoorCoverage = 113,
		R64G64_SInt_PoorCoverage = 114,
		R64G64_SFloat_PoorCoverage = 115,
		R64G64B64_UInt_PoorCoverage = 116,
		R64G64B64_SInt_PoorCoverage = 117,
		R64G64B64_SFloat_PoorCoverage = 118,
		R64G64B64A64_UInt_PoorCoverage = 119,
		R64G64B64A64_SInt_PoorCoverage = 120,
		R64G64B64A64_SFloat_PoorCoverage = 121,
		B10G11R11_UFloat_Pack32 = 122,
		E5B9G9R9_UFloat_Pack32 = 123,
		D16_UNorm = 124,
		X8_D24_UNorm_Pack32_PoorCoverage = 125,
		D32_SFloat = 126,
		S8_UInt_PoorCoverage = 127,
		D16_UNorm_S8_UInt_PoorCoverage = 128,
		D24_UNorm_S8_UInt_PoorCoverage = 129,
		D32_SFloat_S8_UInt = 130,
		BC1_RGB_UNorm_Block = 131,
		BC1_RGB_SRGB_Block = 132,
		BC1_RGBA_UNorm_Block = 133,
		BC1_RGBA_SRGB_Block = 134,
		BC2_UNorm_Block = 135,
		BC2_SRGB_Block = 136,
		BC3_UNorm_Block = 137,
		BC3_SRGB_Block = 138,
		BC4_UNorm_Block = 139,
		BC4_SNorm_Block = 140,
		BC5_UNorm_Block = 141,
		BC5_SNorm_Block = 142,
		BC6H_UFloat_Block = 143,
		BC6H_SFloat_Block = 144,
		BC7_UNorm_Block = 145,
		BC7_SRGB_Block = 146,
		ETC2_R8G8B8_UNorm_Block_PoorCoverage = 147,
		ETC2_R8G8B8_SRGB_Block_PoorCoverage = 148,
		ETC2_R8G8B8A1_UNorm_Block_PoorCoverage = 149,
		ETC2_R8G8B8A1_SRGB_Block_PoorCoverage = 150,
		ETC2_R8G8B8A8_UNorm_Block_PoorCoverage = 151,
		ETC2_R8G8B8A8_SRGB_Block_PoorCoverage = 152,
		EAC_R11_UNorm_Block_PoorCoverage = 153,
		EAC_R11_SNorm_Block_PoorCoverage = 154,
		EAC_R11G11_UNorm_Block_PoorCoverage = 155,
		EAC_R11G11_SNorm_Block_PoorCoverage = 156,
		ASTC_4x4_UNorm_Block_PoorCoverage = 157,
		ASTC_4x4_SRGB_Block_PoorCoverage = 158,
		ASTC_5x4_UNorm_Block_PoorCoverage = 159,
		ASTC_5x4_SRGB_Block_PoorCoverage = 160,
		ASTC_5x5_UNorm_Block_PoorCoverage = 161,
		ASTC_5x5_SRGB_Block_PoorCoverage = 162,
		ASTC_6x5_UNorm_Block_PoorCoverage = 163,
		ASTC_6x5_SRGB_Block_PoorCoverage = 164,
		ASTC_6x6_UNorm_Block_PoorCoverage = 165,
		ASTC_6x6_SRGB_Block_PoorCoverage = 166,
		ASTC_8x5_UNorm_Block_PoorCoverage = 167,
		ASTC_8x5_SRGB_Block_PoorCoverage = 168,
		ASTC_8x6_UNorm_Block_PoorCoverage = 169,
		ASTC_8x6_SRGB_Block_PoorCoverage = 170,
		ASTC_8x8_UNorm_Block_PoorCoverage = 171,
		ASTC_8x8_SRGB_Block_PoorCoverage = 172,
		ASTC_10x5_UNorm_Block_PoorCoverage = 173,
		ASTC_10x5_SRGB_Block_PoorCoverage = 174,
		ASTC_10x6_UNorm_Block_PoorCoverage = 175,
		ASTC_10x6_SRGB_Block_PoorCoverage = 176,
		ASTC_10x8_UNorm_Block_PoorCoverage = 177,
		ASTC_10x8_SRGB_Block_PoorCoverage = 178,
		ASTC_10x10_UNorm_Block_PoorCoverage = 179,
		ASTC_10x10_SRGB_Block_PoorCoverage = 180,
		ASTC_12x10_UNorm_Block_PoorCoverage = 181,
		ASTC_12x10_SRGB_Block_PoorCoverage = 182,
		ASTC_12x12_UNorm_Block_PoorCoverage = 183,
		ASTC_12x12_SRGB_Block_PoorCoverage = 184
	};

	enum class SampleCountFlags : uint32_t
	{
		None = 0,
		e1Bit = 0x00000001,
		e2Bit = 0x00000002,
		e4Bit = 0x00000004,
		e8Bit = 0x00000008,
		e16Bit = 0x00000010,
		e32Bit = 0x00000020,
		e64Bit = 0x00000040
	};

	enum class ImageCreateFlags : uint32_t
	{
		None = 0,
		SparseBindingBit = 0x00000001,
		SparseResidencyBit = 0x00000002,
		SparseAliasedBit = 0x00000004,
		MutableFormatBit = 0x00000008,
		CubeCompatibleBit = 0x00000010,
		AliasBit = 0x00000400,
		SplitInstanceBindRegionsBit = 0x00000040,
		e2DArrayCompatibleBit = 0x00000020,
		BlockTexelViewCompatibleBit = 0x00000080,
		ExtendedUsageBit = 0x00000100,
		ProtectedBit = 0x00000800,
		DisjointBit = 0x00000200,
		CornerSampledNVBit = 0x00002000,
		SampleLocationsCompatibleDepthEXTBit = 0x00001000,
		// SubsampledEXTBit = 0x00004000
	};

	enum class SharingMode : uint8_t
	{
		Exclusive = 0,
		Concurrent = 1
	};

	enum class ImageAspectFlags : uint32_t
	{
		ColorBit = 0x00000001,
		DepthBit = 0x00000002,
		StencilBit = 0x00000004,
		MetadataBit = 0x00000008,
		Plane0Bit = 0x00000010,
		Plane1Bit = 0x00000020,
		Plane2Bit = 0x00000040
	};

	enum class ImageLayout : uint32_t
	{
		Undefined = 0,
		ColorAttachmentOptimal = 2,
		DepthStencilAttachmentOptimal = 3,
		DepthStencilReadOnlyOptimal = 4,
		General = 1,
		Preinitialized = 8,
		ShaderReadOnlyOptimal = 5,
		TransferDstOptimal = 7,
		TransferSrcOptimal = 6,

		/* Requires VK_KHR_swapchain */
		PresentSrcKHR = 1000001002
	};

	enum class AccessFlags : uint32_t
	{
		IndirectCommandReadBit = 0x00000001,
		IndexReadBit = 0x00000002,
		VertexAttributeReadBit = 0x00000004,
		UniformReadBit = 0x00000008,
		InputAttachmentReadBit = 0x00000010,
		ShaderReadBit = 0x00000020,
		ShaderWriteBit = 0x00000040,
		ColorAttachmentReadBit = 0x00000080,
		ColorAttachmentWriteBit = 0x00000100,
		DepthStencilAttachmentReadBit = 0x00000200,
		DepthStencilAttachmentWriteBit = 0x00000400,
		TransferReadBit = 0x00000800,
		TransferWriteBit = 0x00001000,
		HostReadBit = 0x00002000,
		HostWriteBit = 0x00004000,
		MemoryReadBit = 0x00008000,
		MemoryWriteBit = 0x00010000,
		// TransformFeedbackWriteEXT = 0x02000000,
		// TransformFeedbackCounterReadEXT = 0x04000000,
		// TransformFeedbackCounterWriteEXT = 0x08000000,
		// ConditionalRenderingReadEXT = 0x00100000,
		// CommandProcessReadNVX = 0x00020000,
		// CommandProcessWriteNVX = 0x00040000,
		// ColorAttachmentReadNoncoherentEXT = 0x00080000,
		// ShadingRateImageReadNV = 0x00800000,
		// AccelerationStructureReadNV = 0x00200000,
		// AccelerationStructureWriteNV = 0x00400000,
		// FragmentDensityMapReadEXT = 0x01000000
	};

	enum class PipelineStageFlags : uint32_t
	{
		None = 0,
		TopOfPipeBit = 0x00000001,
		DrawIndirectBit = 0x00000002,
		VertexInputBit = 0x00000004,
		VertexShaderBit = 0x00000008,
		TessellationControlShaderBit = 0x00000010,
		TessellationEvaluationShaderBit = 0x00000020,
		GeometryShaderBit = 0x00000040,
		FragmentShaderBit = 0x00000080,
		EarlyFragmentTestsBit = 0x00000100,
		LateFragmentTestsBit = 0x00000200,
		ColorAttachmentOutputBit = 0x00000400,
		ComputeShaderBit = 0x00000800,
		TransferBit = 0x00001000,
		BottomOfPipeBit = 0x00002000,
		HostBit = 0x00004000,
		AllGraphicsBit = 0x00008000,
		AllCommandsBit = 0x00010000,
		// TransformFeedbackEXT = 0x01000000,
		// ConditionalRenderingEXT = 0x00040000,
		// CommandProcessNVX = 0x00020000,
		// ShadingRateImageNV = 0x00400000,
		// RayTracingShaderNV = 0x00200000,
		// AccelerationStructureBuildNV = 0x02000000,
		// TaskShaderNV = 0x00080000,
		// MeshShaderNV = 0x00100000,
		// FragmentDensityProcessEXT = 0x00800000
	};

	enum class ImageViewType : uint8_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2,
		Cube = 3,
		e1DArray = 4,
		e2DArray = 5,
		CubeArray = 6
	};

	enum class ComponentSwizzle : uint8_t
	{
		Identity = 0,
		Zero = 1,
		One = 2,
		R = 3,
		G = 4,
		B = 5,
		A = 6
	};

	enum class ShaderStage : uint8_t
	{
		Compute = 0,
		Fragment = 1,
		Geometry = 2,
		TessellationControl = 3,
		TessellationEvaluation = 4,
		Vertex = 5,

		Count,
		Unknown = Count
	};

	enum class DescriptorType : uint32_t
	{
		Sampler = 0,
		CombinedImageSampler = 1,
		SampledImage = 2,
		StorageImage = 3,
		UniformTexelBuffer = 4,
		StorageTexelBuffer = 5,
		UniformBuffer = 6,
		StorageBuffer = 7,
		UniformBufferDynamic = 8,
		StorageBufferDynamic = 9,
		InputAttachment = 10,
		InlineUniformBlock = 1'000'138'000,

		Unknown = std::numeric_limits<uint32_t>::max()
	};

	enum class ShaderStageFlags : uint32_t
	{
		ComputeBit = 0x00000020,
		FragmentBit = 0x00000010,
		GeometryBit = 0x00000008,
		TessellationControlBit = 0x00000002,
		TessellationEvaluationBit = 0x00000004,
		VertexBit = 0x00000001,

		All = 0x7FFFFFFF,
		AllGraphics = 0x0000001F,

		NONE = 0
	};

	enum class PipelineBindPoint : uint8_t
	{
		Graphics = 0,
		Compute = 1
	};

	enum class AttachmentLoadOp : uint8_t
	{
		Load = 0,
		Clear = 1,
		DontCare = 2
	};

	enum class AttachmentStoreOp : uint8_t
	{
		Store = 0,
		DontCare = 1
	};

	enum class VertexInputRate : uint8_t
	{
		Vertex = 0,
		Instance = 1,

		Unknown = std::numeric_limits<uint8_t>::max()
	};

	enum class QueueFamilyType : uint8_t
	{
		Compute = 0,
		Transfer,
		Universal,

		Count
	};

	enum class IndexType : uint8_t
	{
		UInt16 = 0,
		UInt32 = 1
	};

	enum class StencilFaceFlags : uint8_t
	{
		None = 0,
		BackBit  = 0x00000002,
		FrontBit = 0x00000001,
		FrontAndBack = 0x00000003
	};

	enum class QueryResultFlags : uint8_t
	{
		None = 0,
		e64Bit = 0x00000001,
		WaitBit = 0x00000002,
		WithAvailabilityBit = 0x00000004,
		PartialBit = 0x00000008
	};

	enum class PresentModeKHR : uint32_t
	{
		Immediate = 0,
		Mailbox = 1,
		Fifo = 2,
		FifoRelaxed = 3,
		SharedDemandRefresh = 1000111000,
		SharedContinuousRefresh = 1000111001
	};

	enum class DescriptorBindingFlags : uint32_t
	{
		None = 0,
		UpdateAfterBindBit = 0x00000001,
		UpdateUnusedWhilePendingBit = 0x00000002,
		PartiallyBoundBit = 0x00000004,
		VariableDescriptorCountBit = 0x00000008
	};

	enum class PipelineCreateFlags : uint32_t
	{
		None = 0,
		AllowDerivativesBit = 2,
		DisableOptimizationBit = 1,
		DerivativeBit = 4
	};

	enum class DynamicState : uint8_t
	{
		Viewport = 0,
		Scissor = 1,
		LineWidth = 2,
		DepthBias = 3,
		BlendConstants = 4,
		DepthBounds = 5,
		StencilCompareMask = 6,
		StencilWriteMask = 7,
		StencilReference = 8
	};

	enum class CullModeFlags : uint8_t
	{
		None = 0,
		BackBit = 2,
		FrontBit = 1,

		FrontAndBack = 3,
	};

	enum class PolygonMode : uint8_t
	{
		Fill = 0,
		Line = 1,
		Point = 2
	};

	enum class FrontFace : uint8_t
	{
		CounterClockwise = 0,
		Clockwise = 1
	};

	enum class LogicOp : uint8_t
	{
		Clear = 0,
		And = 1,
		AndReverse = 2,
		Copy = 3,
		AndInverted = 4,
		NoOp = 5,
		Xor = 6,
		Or = 7,
		Nor = 8,
		Equivalent = 9,
		Invert = 10,
		OrReverse = 11,
		CopyInverted = 12,
		OrInverted = 13,
		Nand = 14,
		Set = 15
	};

	enum class PrimitiveTopology : uint8_t
	{
		PointList = 0,
		LineList = 1,
		LineStrip = 2,
		TriangleList = 3,
		TriangleStrip = 4,
		TriangleFan = 5,
		LineListWithAdjacency = 6,
		LineStripWithAdjacency = 7,
		TriangleListWithAdjacency = 8,
		TriangleStripWithAdjacency = 9,
		PatchList = 10
	};

	enum StencilOp : uint8_t
	{
		Keep = 0,
		Zero = 1,
		Replace = 2,
		IncrementAndClamp = 3,
		DecrementAndClamp = 4,
		Invert = 5,
		IncrementAndWrap = 6,
		DecrementAndWrap = 7
	};

	enum class BlendOp : uint8_t
	{
		Add = 0,
		Subtract = 1,
		ReverseSubtract = 2,
		Min = 3,
		Max = 4
	};

	enum class BlendFactor : uint8_t
	{
		Zero = 0,
		One = 1,
		SrcColor = 2,
		OneMinusSrcColor = 3,
		DstColor = 4,
		OneMinusDstColor = 5,
		SrcAlpha = 6,
		OneMinusSrcAlpha = 7,
		DstAlpha = 8,
		OneMinusDstAlpha = 9,
		ConstantColor = 10,
		OneMinusConstantColor = 11,
		ConstantAlpha = 12,
		OneMinusConstantAlpha = 13,
		SrcAlphaSaturate = 14,
		Src1Color = 15,
		OneMinusSrc1Color = 16,
		Src1Alpha = 17,
		OneMinusSrc1Alpha = 18
	};

	enum class ColorComponentFlags
	{
		None = 0,
		RBit = 1,
		GBit = 2,
		BBit = 4,
		ABit = 8,
	};

	struct MemoryRequirements
	{
		DeviceSize size = 0;
		DeviceSize alignment = 0;
		uint32_t memoryTypeBits = 0;
	};

	using PipelineID = uint32_t;
	using BindingIndex = uint32_t;
	using SampleMask = uint32_t;
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::MemoryFeatureFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::QueueFamilyFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::BufferUsageFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ImageUsageFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::SampleCountFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ImageCreateFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ImageAspectFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::AccessFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::PipelineStageFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ShaderStageFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::StencilFaceFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::QueryResultFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::DescriptorBindingFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::PipelineCreateFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::CullModeFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ColorComponentFlags)

#endif
