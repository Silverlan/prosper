/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_BUFFER_CREATE_INFO_HPP__
#define __PROSPER_BUFFER_CREATE_INFO_HPP__

#include "prosper_definitions.hpp"
#include "prosper_enums.hpp"

namespace prosper::util
{
	struct DLLPROSPER BufferCreateInfo
	{
		DeviceSize size = 0ull;
		QueueFamilyFlags queueFamilyMask = QueueFamilyFlags::GraphicsBit;
		enum class Flags : uint32_t
		{
			None = 0u,
			ConcurrentSharing = 1u, // Exclusive sharing is used if this flag is not set
			DontAllocateMemory = ConcurrentSharing<<1u,

			Sparse = DontAllocateMemory<<1u,
			SparseAliasedResidency = Sparse<<1u // Only has an effect if Sparse-flag is set
		};
		Flags flags = Flags::None;
		BufferUsageFlags usageFlags = BufferUsageFlags::UniformBufferBit;
		MemoryFeatureFlags memoryFeatures = MemoryFeatureFlags::GPUBulk; // Only used if AllocateMemory-flag is set
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::BufferCreateInfo::Flags);

#endif
