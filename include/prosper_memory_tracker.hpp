/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_MEMORY_TRACKER_HPP__
#define __PROSPER_MEMORY_TRACKER_HPP__

#include "prosper_definitions.hpp"
#include <vector>
#include <memory>

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class IBuffer;
	class DLLPROSPER MemoryTracker
	{
	public:
		struct DLLPROSPER Resource
		{
			enum class TypeFlags : uint8_t
			{
				None = 0u,
				ImageBit = 1u,
				BufferBit = ImageBit<<1u,
				UniformBufferBit = BufferBit<<1u,
				DynamicBufferBit = UniformBufferBit<<1u,
				StandAloneBufferBit = DynamicBufferBit<<1u,

				Any = std::numeric_limits<uint8_t>::max()
			};
			void *resource;
			TypeFlags typeFlags;
			Anvil::MemoryBlock *GetMemoryBlock(uint32_t i) const;
			uint32_t GetMemoryBlockCount() const;
		};
		static MemoryTracker &GetInstance();

		bool GetMemoryStats(prosper::IPrContext &context,uint32_t memType,uint64_t &allocatedSize,uint64_t &totalSize,Resource::TypeFlags typeFlags=Resource::TypeFlags::Any) const;
		void GetResources(uint32_t memType,std::vector<const Resource*> &outResources,Resource::TypeFlags typeFlags=Resource::TypeFlags::Any) const;
		const std::vector<Resource> &GetResources() const;

		void AddResource(IBuffer &buffer);
		void AddResource(IImage &buffer);
		void RemoveResource(IBuffer &buffer);
		void RemoveResource(IImage &buffer);
	private:
		void RemoveResource(void *resource);
		MemoryTracker()=default;
		std::vector<Resource> m_resources = {};
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::MemoryTracker::Resource::TypeFlags)
#pragma warning(pop)

#endif
