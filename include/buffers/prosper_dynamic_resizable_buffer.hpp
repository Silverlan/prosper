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

namespace prosper
{
	class IPrContext;
	class IBuffer;
	class DLLPROSPER IDynamicResizableBuffer
		: public IResizableBuffer
	{
	public:
		std::shared_ptr<IBuffer> AllocateBuffer(DeviceSize size,const void *data=nullptr);
		std::shared_ptr<IBuffer> AllocateBuffer(DeviceSize size,uint32_t alignment,const void *data,bool reallocateIfNoSpaceAvailable=true);
		void DebugPrint(std::stringstream &strFilledData,std::stringstream &strFreeData,std::stringstream *bufferData=nullptr) const;
		const std::vector<IBuffer*> &GetAllocatedSubBuffers() const;
		uint64_t GetFreeSize() const;
		float GetFragmentationPercent() const;
		uint32_t GetAlignment() const {return m_alignment;}
	protected:
		struct Range
		{
			DeviceSize startOffset;
			DeviceSize size;
		};
		IDynamicResizableBuffer(
			IPrContext &context,IBuffer &buffer,const util::BufferCreateInfo &createInfo,uint64_t maxTotalSize
		);
		void InsertFreeMemoryRange(std::list<Range>::iterator itWhere,DeviceSize startOffset,DeviceSize size);
		void MarkMemoryRangeAsFree(DeviceSize startOffset,DeviceSize size);
		std::list<Range>::iterator FindFreeRange(DeviceSize size,uint32_t alignment);
		std::vector<IBuffer*> m_allocatedSubBuffers;
		std::list<Range> m_freeRanges;
		mutable std::mutex m_bufferMutex;
		uint32_t m_alignment = 0u;
	};
};

#endif
