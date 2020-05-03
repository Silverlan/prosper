/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "buffers/prosper_uniform_resizable_buffer.hpp"
#include "prosper_util.hpp"
#include "buffers/prosper_buffer.hpp"
#include "vk_uniform_resizable_buffer.hpp"
#include "vk_buffer.hpp"
#include "prosper_context.hpp"
#include <wrappers/buffer.h>
#include <misc/memory_allocator.h>
#include <misc/buffer_create_info.h>
#include <wrappers/memory_block.h>

using namespace prosper;

IUniformResizableBuffer::IUniformResizableBuffer(
	IPrContext &context,IBuffer &buffer,
	uint64_t bufferInstanceSize,uint64_t alignedBufferBaseSize,
	uint64_t maxTotalSize,uint32_t alignment
)
	: IResizableBuffer{buffer,maxTotalSize},m_bufferInstanceSize{bufferInstanceSize},m_alignment{alignment}
{
	m_createInfo.size = alignedBufferBaseSize;

	auto numMaxBuffers = buffer.GetCreateInfo().size /bufferInstanceSize;
	m_allocatedSubBuffers.resize(numMaxBuffers,nullptr);
}

uint64_t IUniformResizableBuffer::GetInstanceSize() const {return m_bufferInstanceSize;}

const std::vector<IBuffer*> &IUniformResizableBuffer::GetAllocatedSubBuffers() const {return m_allocatedSubBuffers;}
uint64_t IUniformResizableBuffer::GetAssignedMemory() const {return m_assignedMemory;}
uint32_t IUniformResizableBuffer::GetTotalInstanceCount() const {return m_allocatedSubBuffers.size();}

std::shared_ptr<IBuffer> IUniformResizableBuffer::AllocateBuffer(const void *data)
{
	auto offset = 0ull;
	auto bUseExistingSlot = false;
	auto baseAlignedInstanceSize = prosper::util::get_aligned_size(m_bufferInstanceSize,m_alignment);
	auto alignedInstanceSize = baseAlignedInstanceSize;// *numInstances;
	if(m_freeOffsets.empty() == false)
	{
		offset = m_freeOffsets.front();
		m_freeOffsets.pop();
		bUseExistingSlot = true;
	}
	else
	{
		if(m_assignedMemory +alignedInstanceSize > m_baseSize)
		{
			if(m_assignedMemory +alignedInstanceSize > m_maxTotalSize)
				return nullptr; // Total capacity has been reached
			// Re-allocate buffer; Increase previous size by additional factor
			auto oldSize = m_baseSize;
			m_baseSize += m_createInfo.size;
			auto createInfo = m_createInfo;
			createInfo.size = m_baseSize;
			auto newBuffer = GetContext().CreateBuffer(createInfo);
			assert(newBuffer);
			std::vector<uint8_t> data(oldSize);
			Read(0ull,data.size(),data.data());
			for(auto *subBuffer : m_allocatedSubBuffers)
			{
				if(subBuffer == nullptr)
					continue;
				dynamic_cast<VlkBuffer*>(subBuffer)->SetBuffer(Anvil::Buffer::create(Anvil::BufferCreateInfo::create_no_alloc_child(&dynamic_cast<VlkBuffer*>(newBuffer.get())->GetAnvilBuffer(),subBuffer->GetStartOffset(),subBuffer->GetSize())));
			}
			newBuffer->Write(0ull,data.size(),data.data());
			dynamic_cast<VlkBuffer*>(this)->SetBuffer(std::move(dynamic_cast<VlkBuffer*>(newBuffer.get())->m_buffer));
			newBuffer = nullptr;
			auto numMaxBuffers = createInfo.size /baseAlignedInstanceSize;
			m_allocatedSubBuffers.resize(numMaxBuffers,nullptr);

			for(auto &f : m_reallocationCallbacks)
				f();
		}
		offset = m_assignedMemory;
	}

	auto idx = offset /baseAlignedInstanceSize;
	auto pThis = std::dynamic_pointer_cast<VkUniformResizableBuffer>(shared_from_this());
	auto subBuffer = CreateSubBuffer(offset,m_bufferInstanceSize,[pThis,idx](IBuffer &subBuffer) {
		pThis->m_freeOffsets.push(subBuffer.GetStartOffset());
		pThis->m_allocatedSubBuffers.at(idx) = nullptr;
	});
	assert(subBuffer);
	subBuffer->SetParent(*this,idx);
	if(data != nullptr)
		subBuffer->Write(0ull,m_bufferInstanceSize,data);
	m_allocatedSubBuffers.at(idx) = subBuffer.get();
	if(bUseExistingSlot == false)
		m_assignedMemory += alignedInstanceSize;
	return subBuffer;
}
