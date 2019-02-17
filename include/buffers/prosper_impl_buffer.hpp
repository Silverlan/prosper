/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_IMPL_BUFFER_HPP__
#define __PROSPER_IMPL_BUFFER_HPP__

#include "buffers/prosper_buffer.hpp"
#include "prosper_util.hpp"

namespace prosper
{
	namespace util
	{
		template<class TBuffer>
			std::shared_ptr<TBuffer> create_buffer(Anvil::BaseDevice &dev,const BufferCreateInfo &createInfo,const void *data);
	};
};
template<class TBuffer>
	std::shared_ptr<TBuffer> prosper::util::create_buffer(Anvil::BaseDevice &dev,const BufferCreateInfo &createInfo,const void *data)
{
	auto sharingMode = vk::SharingMode::eExclusive;
	if((createInfo.flags &BufferCreateInfo::Flags::ConcurrentSharing) != BufferCreateInfo::Flags::None)
		sharingMode = vk::SharingMode::eConcurrent;

	auto &context = Context::GetContext(dev);
	if((createInfo.flags &BufferCreateInfo::Flags::Sparse) != BufferCreateInfo::Flags::None)
	{
		auto sparseResidencyScope = (createInfo.flags &BufferCreateInfo::Flags::SparseAliasedResidency) != BufferCreateInfo::Flags::None ?
			Anvil::SparseResidencyScope::SPARSE_RESIDENCY_SCOPE_ALIASED :
			Anvil::SparseResidencyScope::SPARSE_RESIDENCY_SCOPE_NONALIASED
		;
		return std::static_pointer_cast<TBuffer>(Buffer::Create<TBuffer>(context,Anvil::Buffer::create_sparse(
			&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
			static_cast<VkSharingMode>(sharingMode),static_cast<VkBufferUsageFlags>(createInfo.usageFlags),sparseResidencyScope
		)));
	}
	if((createInfo.flags &BufferCreateInfo::Flags::DontAllocateMemory) == BufferCreateInfo::Flags::None)
	{
		return std::static_pointer_cast<TBuffer>(Buffer::Create<TBuffer>(context,Anvil::Buffer::create_nonsparse(
			&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
			static_cast<VkSharingMode>(sharingMode),static_cast<VkBufferUsageFlags>(createInfo.usageFlags),memory_feature_flags_to_anvil_flags(createInfo.memoryFeatures),
			data
		)));
	}
	return std::static_pointer_cast<TBuffer>(Buffer::Create<TBuffer>(context,Anvil::Buffer::create_nonsparse(
		&dev,static_cast<VkDeviceSize>(createInfo.size),queue_family_flags_to_anvil_queue_family(createInfo.queueFamilyMask),
		static_cast<VkSharingMode>(sharingMode),static_cast<VkBufferUsageFlags>(createInfo.usageFlags)
	)));
}

#endif
