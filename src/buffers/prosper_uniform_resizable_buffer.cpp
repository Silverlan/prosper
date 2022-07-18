/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "buffers/prosper_uniform_resizable_buffer.hpp"
#include "prosper_util.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_context.hpp"
#include <cassert>

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

uint64_t IUniformResizableBuffer::GetInstanceSize() const
{
	std::unique_lock lock {m_bufferMutex}; // TODO: Why do we need this lock?
	return m_bufferInstanceSize;
}

uint32_t IUniformResizableBuffer::GetAlignment() const
{
	std::unique_lock lock {m_bufferMutex}; // TODO: Why do we need this lock?
	return m_alignment;
}
uint64_t IUniformResizableBuffer::GetStride() const
{
	std::unique_lock lock {m_bufferMutex}; // TODO: Why do we need this lock?
	return prosper::util::get_aligned_size(m_bufferInstanceSize,m_alignment);
}

const std::vector<IBuffer*> &IUniformResizableBuffer::GetAllocatedSubBuffers() const {return m_allocatedSubBuffers;}
uint64_t IUniformResizableBuffer::GetAssignedMemory() const
{
	std::unique_lock lock {m_bufferMutex};
	return m_assignedMemory;
}
uint32_t IUniformResizableBuffer::GetTotalInstanceCount() const {return m_allocatedSubBuffers.size();}

std::shared_ptr<IBuffer> IUniformResizableBuffer::AllocateBuffer(const void *data)
{
	std::unique_lock lock {m_bufferMutex};
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
			GetContext().WaitIdle();
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
				subBuffer->RecreateInternalSubBuffer(*newBuffer);
			}
			newBuffer->Write(0ull,data.size(),data.data());
			MoveInternalBuffer(*newBuffer);
			newBuffer = nullptr;
			auto numMaxBuffers = createInfo.size /baseAlignedInstanceSize;
			m_allocatedSubBuffers.resize(numMaxBuffers,nullptr);

			for(auto *subBuffer : m_allocatedSubBuffers)
			{
				if(subBuffer == nullptr)
					continue;
				for(auto &cb : subBuffer->m_reallocationCallbacks)
				{
					if(cb.IsValid() == false)
						continue;
					cb();
				}
			}

			for(auto &f : m_onReallocCallbacks)
				f();
		}
		offset = m_assignedMemory;
	}

	auto idx = offset /baseAlignedInstanceSize;
	auto pThis = std::dynamic_pointer_cast<IUniformResizableBuffer>(shared_from_this());
	auto subBuffer = CreateSubBuffer(offset,m_bufferInstanceSize,[pThis,idx](IBuffer &subBuffer) {
		std::unique_lock lock {pThis->m_bufferMutex};
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
