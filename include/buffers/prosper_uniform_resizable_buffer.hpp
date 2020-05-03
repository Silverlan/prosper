/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_UNIFORM_RESIZABLE_BUFFER_HPP__
#define __PROSPER_UNIFORM_RESIZABLE_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "buffers/prosper_resizable_buffer.hpp"
#include <memory>
#include <queue>
#include <cinttypes>
#include <functional>

namespace Anvil
{
	class BaseDevice;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class VkUniformResizableBuffer;
	namespace util
	{
		struct BufferCreateInfo;
		DLLPROSPER std::shared_ptr<VkUniformResizableBuffer> create_uniform_resizable_buffer(
			IPrContext &context,BufferCreateInfo createInfo,uint64_t bufferInstanceSize,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage=1.f,const void *data=nullptr
		);
	};

	class DLLPROSPER IUniformResizableBuffer
		: public IResizableBuffer
	{
	public:
		std::shared_ptr<IBuffer> AllocateBuffer(const void *data=nullptr);

		uint64_t GetInstanceSize() const;
		friend std::shared_ptr<VkUniformResizableBuffer> util::create_uniform_resizable_buffer(
			IPrContext &context,util::BufferCreateInfo createInfo,uint64_t bufferInstanceSize,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data
		);

		const std::vector<IBuffer*> &GetAllocatedSubBuffers() const;
		uint64_t GetAssignedMemory() const;
		uint32_t GetTotalInstanceCount() const;
	protected:
		IUniformResizableBuffer(
			IPrContext &context,IBuffer &buffer,
			uint64_t bufferInstanceSize,
			uint64_t alignedBufferBaseSize,uint64_t maxTotalSize,uint32_t alignment
		);
		uint64_t m_bufferInstanceSize = 0ull; // Size of each sub-buffer
		uint64_t m_assignedMemory = 0ull;
		uint32_t m_alignment = 0u;
		std::vector<IBuffer*> m_allocatedSubBuffers;
		std::queue<uint64_t> m_freeOffsets;
	};
};
#pragma warning(pop)

#endif
