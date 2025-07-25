// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_BUFFER_CREATE_INFO_HPP__
#define __PROSPER_BUFFER_CREATE_INFO_HPP__

#include "prosper_definitions.hpp"
#include "prosper_enums.hpp"

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
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::BufferCreateInfo::Flags);

#endif
