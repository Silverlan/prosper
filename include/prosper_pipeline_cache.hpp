/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PIPELINE_CACHE_HPP__
#define __PROSPER_PIPELINE_CACHE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/pipeline_cache.h>

namespace Anvil
{
	class Buffer;
};

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class DLLPROSPER PipelineCache
		: public ContextObject,
		public std::enable_shared_from_this<PipelineCache>
	{
	public:
		PipelineCache()=delete;
		PipelineCache(const PipelineCache&)=delete;
		enum class LoadError : uint8_t
		{
			Ok = 0u,
			FileNotFound,
			InvalidFormat,
			UnsupportedCacheVersion,
			IncompatibleVendor,
			IncompatibleDevice,
			IncompatiblePipelineCacheId
		};

		static Anvil::PipelineCacheUniquePtr Create(Anvil::BaseDevice &dev);
		static Anvil::PipelineCacheUniquePtr Load(Anvil::BaseDevice &dev,const std::string &fileName,LoadError &outErr);
		static bool Save(Anvil::PipelineCache &cache,const std::string &fileName);

#pragma pack(push,1)
		struct Header
		{
			uint32_t size;
			vk::PipelineCacheHeaderVersion version;
			decltype(vk::PhysicalDeviceProperties::vendorID) vendorId;
			decltype(vk::PhysicalDeviceProperties::deviceID) deviceId;
			decltype(vk::PhysicalDeviceProperties::pipelineCacheUUID) uuid;

			// See https://vulkan.lunarg.com/doc/view/1.0.26.0/linux/vkspec.chunked/ch09s06.html
			static_assert(sizeof(size) == 4u);
			static_assert(sizeof(version) == 4u);
			static_assert(sizeof(vendorId) == 4u);
			static_assert(sizeof(deviceId) == 4u);
			static_assert(sizeof(uuid) == VK_UUID_SIZE);
		};
#pragma pack(pop)
		static_assert(offsetof(Header,size) == 0u);
		static_assert(offsetof(Header,version) == 4u);
		static_assert(offsetof(Header,vendorId) == 8u);
		static_assert(offsetof(Header,deviceId) == 12u);
		static_assert(offsetof(Header,uuid) == 16u);
	};
};
#pragma warning(pop)

#endif
