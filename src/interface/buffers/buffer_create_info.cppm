// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include "util_enum_flags.hpp"

export module pragma.prosper:buffer.buffer_create_info;

export import :enums;

export {
	namespace prosper::util {
		struct DLLPROSPER BufferCreateInfo {
			DeviceSize size = 0ull;
			QueueFamilyFlags queueFamilyMask = QueueFamilyFlags::GraphicsBit;
			enum class Flags : uint32_t {
				None = 0u,
				ConcurrentSharing = 1u, // Exclusive sharing is used if this flag is not set
				DontAllocateMemory = ConcurrentSharing << 1u,

				Sparse = DontAllocateMemory << 1u,
				SparseAliasedResidency = Sparse << 1u, // Only has an effect if Sparse-flag is set

				// The client may request that the server read from or write to the buffer while it is mapped. The client's pointer to the data store remains valid so long as the data store is mapped, even during execution of drawing or dispatch commands.
				Persistent = SparseAliasedResidency << 1u
			};
			Flags flags = Flags::None;
			BufferUsageFlags usageFlags = BufferUsageFlags::UniformBufferBit;
			MemoryFeatureFlags memoryFeatures = MemoryFeatureFlags::GPUBulk; // Only used if AllocateMemory-flag is set
		};
		using namespace umath::scoped_enum::bitwise;
	};
	REGISTER_ENUM_FLAGS(prosper::util::BufferCreateInfo::Flags)
}
