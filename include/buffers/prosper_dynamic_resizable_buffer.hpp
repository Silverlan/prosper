/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DYNAMIC_RESIZABLE_BUFFER_HPP__
#define __PROSPER_DYNAMIC_RESIZABLE_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "buffers/prosper_resizable_buffer.hpp"
#include <memory>
#include <queue>
#include <cinttypes>
#include <functional>
#include <list>

namespace Anvil
{
	class BaseDevice;
	class Buffer;
};

namespace prosper
{
	class IPrContext;
	class IBuffer;
	namespace util
	{
		struct BufferCreateInfo;
		DLLPROSPER std::shared_ptr<VkDynamicResizableBuffer> create_dynamic_resizable_buffer(
			IPrContext &context,BufferCreateInfo createInfo,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage=1.f,const void *data=nullptr
		);
	};

	class DLLPROSPER IDynamicResizableBuffer
		: public IResizableBuffer
	{
	public:
		std::shared_ptr<IBuffer> AllocateBuffer(vk::DeviceSize size,const void *data=nullptr);
		std::shared_ptr<IBuffer> AllocateBuffer(vk::DeviceSize size,uint32_t alignment,const void *data);

		friend std::shared_ptr<VkDynamicResizableBuffer> util::create_dynamic_resizable_buffer(
			IPrContext &context,util::BufferCreateInfo createInfo,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data
		);

		void DebugPrint(std::stringstream &strFilledData,std::stringstream &strFreeData,std::stringstream *bufferData=nullptr) const;
		const std::vector<IBuffer*> &GetAllocatedSubBuffers() const;
		uint64_t GetFreeSize() const;
		float GetFragmentationPercent() const;
	protected:
		struct Range
		{
			vk::DeviceSize startOffset;
			vk::DeviceSize size;
		};
		IDynamicResizableBuffer(
			IPrContext &context,IBuffer &buffer,const util::BufferCreateInfo &createInfo,uint64_t maxTotalSize
		);
		void InsertFreeMemoryRange(std::list<Range>::iterator itWhere,vk::DeviceSize startOffset,vk::DeviceSize size);
		void MarkMemoryRangeAsFree(vk::DeviceSize startOffset,vk::DeviceSize size);
		std::list<Range>::iterator FindFreeRange(vk::DeviceSize size,uint32_t alignment);
		std::vector<IBuffer*> m_allocatedSubBuffers;
		std::list<Range> m_freeRanges;
		uint32_t m_alignment = 0u;
	};
};

#endif
