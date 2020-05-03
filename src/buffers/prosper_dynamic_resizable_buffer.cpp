/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "buffers/prosper_dynamic_resizable_buffer.hpp"
#include "vk_dynamic_resizable_buffer.hpp"
#include "vk_buffer.hpp"
#include "vk_context.hpp"
#include "prosper_util.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_context.hpp"
#include <wrappers/buffer.h>
#include <misc/memory_allocator.h>
#include <misc/buffer_create_info.h>
#include <wrappers/device.h>
#include <wrappers/memory_block.h>
#include <sstream>

using namespace prosper;

/*
static void test_dynamic_resizable_buffer()
{
	auto dev = GetDevice()->shared_from_this();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.size = 100;
	createInfo.usageFlags = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;// | prosper::util::MemoryFeatureFlags::Mappable;//prosper::util::MemoryFeatureFlags::HostCached | prosper::util::MemoryFeatureFlags::Mappable;
	auto maxTotalSize = 1'000;
	auto buf = prosper::util::create_dynamic_resizable_buffer(*this,createInfo,maxTotalSize);

	std::array<std::shared_ptr<prosper::Buffer>,10> buffers;
	for(auto &subBuf : buffers)
		subBuf = buf->AllocateBuffer(10,0);
	auto fPrint = [&buf](bool bPrintBinary=false) {
		std::stringstream strFilledData;
		std::stringstream strFreeData;
		std::stringstream strBinaryData;
		buf->DebugPrint(strFilledData,strFreeData,(bPrintBinary == true) ? &strBinaryData : nullptr);
		Con::cout<<"Used: "<<strFilledData.str()<<Con::endl;
		Con::cout<<"Free: "<<strFreeData.str()<<Con::endl;
		if(bPrintBinary == true)
			Con::cout<<"BinD: "<<strBinaryData.str()<<Con::endl;
		Con::cout<<Con::endl;
	};
	fPrint();
	//void DynamicResizableBuffer::DebugPrint(std::stringstream &strFilledData,std::stringstream strFreeData) const
	buffers.at(4) = nullptr; fPrint();
	buffers.at(4) = buf->AllocateBuffer(10,0); fPrint();
	buffers.at(4) = nullptr; fPrint();
	buffers.at(4) = buf->AllocateBuffer(5,0); fPrint();

	auto buf0 = buf->AllocateBuffer(5,0); fPrint();

	buffers.at(1) = nullptr; fPrint();
	buffers.at(2) = nullptr; fPrint();
	buffers.at(4) = nullptr; fPrint();
	buffers.at(3) = nullptr; fPrint();
	buffers.at(0) = nullptr; fPrint();

	buffers.back() = nullptr; fPrint();

	buffers.back() = buf->AllocateBuffer(3,0); fPrint();
	buffers.at(0) = buf->AllocateBuffer(7,4); fPrint();

	buf0 = nullptr; fPrint();
	for(auto &buf : buffers)
	{
		buf = nullptr;
		fPrint();
	}

	for(auto &subBuf : buffers)
		subBuf = buf->AllocateBuffer(10,0);

	fPrint();

	buffers.at(0) = nullptr; fPrint();
	buffers.at(2) = nullptr; fPrint();
	buffers.at(1) = nullptr; fPrint();

	std::array<uint8_t,10> data;
	for(auto i=decltype(data.size()){0};i<data.size();++i)
		data.at(i) = i;
	buffers.at(3)->Write(0ull,data.size(),data.data());
	std::fill(data.begin(),data.end(),0u);
	buf0 = buf->AllocateBuffer(80,0); fPrint(true);
		
	buffers.at(3)->Read(0ull,data.size(),data.data());

	auto buf1 = buf->AllocateBuffer(19,0); fPrint(true);

	std::cout<<"Complete!"<<std::endl;
}
*/

IDynamicResizableBuffer::IDynamicResizableBuffer(
	IPrContext &context,IBuffer &buffer,
	const prosper::util::BufferCreateInfo &createInfo,uint64_t maxTotalSize
)
	: IResizableBuffer{buffer,maxTotalSize}
{
	m_freeRanges.push_back({0ull,createInfo.size});
	m_alignment = context.GetBufferAlignment(createInfo.usageFlags);
}

void IDynamicResizableBuffer::InsertFreeMemoryRange(std::list<Range>::iterator itWhere,DeviceSize startOffset,DeviceSize size)
{
	if(itWhere == m_freeRanges.end())
	{
		if(m_freeRanges.empty() == false)
		{
			auto &range = m_freeRanges.back();
			if(range.startOffset +range.size == startOffset)
			{
				range.size += size;
				return;
			}
		}
		m_freeRanges.push_back({startOffset,size});
		return;
	}
	if(itWhere != m_freeRanges.begin())
	{
		auto itPrev = itWhere;
		--itPrev;
		if(itPrev->startOffset +itPrev->size == startOffset)
		{
			itPrev->size += size;
			if(itPrev->startOffset +itPrev->size == itWhere->startOffset)
			{
				itPrev->size += itWhere->size;
				m_freeRanges.erase(itWhere);
			}
			return;
		}
		m_freeRanges.insert(itWhere,{startOffset,size});
		return;
	}
	if(startOffset +size == itWhere->startOffset)
	{
		itWhere->startOffset = startOffset;
		itWhere->size += size;
		return;
	}
	m_freeRanges.push_front({startOffset,size});
}
void IDynamicResizableBuffer::MarkMemoryRangeAsFree(vk::DeviceSize startOffset,vk::DeviceSize size)
{
	if(size == 0ull)
		return;
	auto it = std::find_if(m_freeRanges.begin(),m_freeRanges.end(),[startOffset,size](const Range &range) {
		return startOffset < range.startOffset;
	});
	InsertFreeMemoryRange(it,startOffset,size);
}
std::list<IDynamicResizableBuffer::Range>::iterator IDynamicResizableBuffer::FindFreeRange(vk::DeviceSize size,uint32_t alignment)
{
	// Find the smallest available space that can fit the requested size (including alignment padding)
	auto itMin = m_freeRanges.end();
	auto minSize = std::numeric_limits<vk::DeviceSize>::max();
	for(auto it=m_freeRanges.begin();it!=m_freeRanges.end();++it)
	{
		auto &range = *it;
		auto reqSize = size +prosper::util::get_offset_alignment_padding(range.startOffset,alignment);
		if(reqSize > range.size || range.size > minSize)
			continue;
		minSize = range.size;
		itMin = it;
	}
	return itMin;
}

const std::vector<IBuffer*> &IDynamicResizableBuffer::GetAllocatedSubBuffers() const {return m_allocatedSubBuffers;}
uint64_t IDynamicResizableBuffer::GetFreeSize() const
{
	auto size = 0ull;
	for(auto &range : m_freeRanges)
		size += range.size;
	return size;
}
float IDynamicResizableBuffer::GetFragmentationPercent() const
{
	auto size = GetSize();
	auto fragmentSize = 0ull;
	for(auto &range : m_freeRanges)
	{
		if(range.startOffset +range.size < size)
			fragmentSize += range.size;
	}
	return fragmentSize /static_cast<double>(size);
}

void IDynamicResizableBuffer::DebugPrint(std::stringstream &strFilledData,std::stringstream &strFreeData,std::stringstream *bufferData) const
{
	std::vector<Range> allocatedRanges(m_allocatedSubBuffers.size());
	for(auto i=decltype(m_allocatedSubBuffers.size()){0};i<m_allocatedSubBuffers.size();++i)
	{
		auto *buf = m_allocatedSubBuffers.at(i);
		allocatedRanges.at(i) = {buf->GetStartOffset(),buf->GetSize()};
	}
	std::sort(allocatedRanges.begin(),allocatedRanges.end(),[](const Range &a,const Range &b) {
		return a.startOffset < b.startOffset;
	});
	auto size = GetSize();
	auto fPrintRangeData = [size](const auto &ranges,std::stringstream &ss) {
		auto itRange = ranges.begin();
		for(auto i=decltype(size){0};i<size;)
		{
			auto *range = (itRange != ranges.end()) ? &(*itRange) : nullptr;
			if(range == nullptr || i < range->startOffset)
			{
				ss<<"0";
				++i;
				continue;
			}
			if(i < range->startOffset +range->size)
			{
				if(i == (range->startOffset +range->size -1))
					ss<<"]";
				else if(i == range->startOffset)
					ss<<"[";
				else
					ss<<"X";
				++i;
				continue;
			}
			++itRange;
		}
	};
	fPrintRangeData(allocatedRanges,strFilledData);
	fPrintRangeData(m_freeRanges,strFreeData);

	if(bufferData == nullptr)
		return;
	std::vector<uint8_t> data(size);
	Read(0ull,data.size() *sizeof(data.front()),data.data());
	for(auto &v : data)
		(*bufferData)<<+v;
}

std::shared_ptr<IBuffer> IDynamicResizableBuffer::AllocateBuffer(vk::DeviceSize size,const void *data) {return AllocateBuffer(size,m_alignment,data);}
std::shared_ptr<IBuffer> IDynamicResizableBuffer::AllocateBuffer(vk::DeviceSize requestSize,uint32_t alignment,const void *data)
{
	if(requestSize == 0ull)
		return nullptr;
	auto offset = 0ull;
	auto bUseExistingSlot = false;
	auto it = FindFreeRange(requestSize,alignment);
	if(it != m_freeRanges.end())
	{
		offset = it->startOffset;
		auto rangeStartOffset = it->startOffset;
		auto rangeSize = it->size;
		m_freeRanges.erase(it);
		offset += prosper::util::get_offset_alignment_padding(offset,alignment);

		if(offset > rangeStartOffset)
			MarkMemoryRangeAsFree(rangeStartOffset,offset -rangeStartOffset); // Anterior range
		if(offset +requestSize < rangeStartOffset +rangeSize)
			MarkMemoryRangeAsFree(offset +requestSize,(rangeStartOffset +rangeSize) -(offset +requestSize)); // Posterior range
		
		bUseExistingSlot = true;
	}
	else
	{
		auto padding = prosper::util::get_offset_alignment_padding(m_baseSize,alignment);
		if(m_baseSize +requestSize +padding > m_maxTotalSize)
			return nullptr; // Total capacity has been reached
		// Re-allocate buffer; Increase previous size by additional factor
		auto oldSize = m_baseSize;
		m_baseSize += umath::max(m_createInfo.size,requestSize +padding);
		auto createInfo = m_createInfo;
		createInfo.size = m_baseSize;
		auto newBuffer = GetContext().CreateBuffer(createInfo);
		assert(newBuffer);
		std::vector<uint8_t> oldData(oldSize);
		Read(0ull,oldData.size(),oldData.data());
		
		for(auto *subBuffer : m_allocatedSubBuffers)
			dynamic_cast<VlkBuffer*>(subBuffer)->m_buffer = Anvil::Buffer::create(Anvil::BufferCreateInfo::create_no_alloc_child(&dynamic_cast<VlkBuffer&>(*newBuffer).GetAnvilBuffer(),subBuffer->GetStartOffset(),subBuffer->GetSize()));
		newBuffer->Write(0ull,oldData.size(),oldData.data());
		dynamic_cast<VlkBuffer*>(this)->SetBuffer(std::move(dynamic_cast<VlkBuffer*>(newBuffer.get())->m_buffer));
		newBuffer = nullptr;

		// Update free range
		auto bNewRange = true;
		if(m_freeRanges.empty() == false)
		{
			auto &rangeLast = m_freeRanges.back();
			if(rangeLast.startOffset +rangeLast.size == oldSize)
			{
				rangeLast.size += m_baseSize -rangeLast.startOffset;
				bNewRange = false;
			}
		}
		if(bNewRange == true)
			m_freeRanges.push_back({oldSize,m_baseSize -oldSize});
		//

		for(auto &f : m_reallocationCallbacks)
			f();
		return AllocateBuffer(requestSize,alignment,data);
	}

	auto pThis = std::dynamic_pointer_cast<IDynamicResizableBuffer>(shared_from_this());

	auto subBuffer = CreateSubBuffer(offset,requestSize,[pThis,requestSize](IBuffer &subBuffer) {
		auto it = std::find_if(pThis->m_allocatedSubBuffers.begin(),pThis->m_allocatedSubBuffers.end(),[&subBuffer](const IBuffer *subBufferOther) {
			return subBufferOther == &subBuffer;
		});
		pThis->m_allocatedSubBuffers.erase(it);
		pThis->MarkMemoryRangeAsFree(subBuffer.GetStartOffset(),requestSize);
	});
	assert(subBuffer);
	subBuffer->SetParent(*this);
	if(data != nullptr)
		subBuffer->Write(0ull,requestSize,data);
	if(m_allocatedSubBuffers.size() == m_allocatedSubBuffers.capacity())
		m_allocatedSubBuffers.reserve(m_allocatedSubBuffers.size() +50u);
	m_allocatedSubBuffers.push_back(subBuffer.get());
	return subBuffer;
}
