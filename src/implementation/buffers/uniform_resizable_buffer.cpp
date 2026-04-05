// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.uniform_resizable_buffer;

using namespace prosper;

IUniformResizableBuffer::IUniformResizableBuffer(IPrContext &context, IBuffer &buffer, uint64_t bufferInstanceSize, uint64_t alignedBufferBaseSize, uint64_t maxTotalSize, uint32_t alignment)
    : IResizableBuffer {buffer, maxTotalSize}, m_bufferInstanceSize {bufferInstanceSize}, m_alignment {alignment}
{
	m_createInfo.size = alignedBufferBaseSize;

	auto numMaxBuffers = buffer.GetCreateInfo().size / bufferInstanceSize;
	m_allocatedSubBuffers.resize(numMaxBuffers, nullptr);
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
	return util::get_aligned_size(m_bufferInstanceSize, m_alignment);
}

const std::vector<IBuffer *> &IUniformResizableBuffer::GetAllocatedSubBuffers() const { return m_allocatedSubBuffers; }
uint64_t IUniformResizableBuffer::GetAssignedMemory() const
{
	std::unique_lock lock {m_bufferMutex};
	return m_assignedMemory;
}
uint32_t IUniformResizableBuffer::GetTotalInstanceCount() const { return m_allocatedSubBuffers.size(); }

bool IUniformResizableBuffer::EnsureCapacity(uint32_t instanceCount)
{
	std::scoped_lock lock {m_bufferMutex};
	auto baseAlignedInstanceSize = util::get_aligned_size(m_bufferInstanceSize, m_alignment);
	auto alignedInstanceSize = baseAlignedInstanceSize * instanceCount;
	auto requiredSize = m_assignedMemory + alignedInstanceSize;
	if(requiredSize > m_maxTotalSize)
		return false; // Total capacity has been reached
	ReallocateMemory(requiredSize);

	auto numMaxBuffers = m_baseSize / baseAlignedInstanceSize;
	m_allocatedSubBuffers.resize(numMaxBuffers, nullptr);

	RunReallocationCallbacks();
	return true;
}

std::shared_ptr<IBuffer> IUniformResizableBuffer::AllocateBuffer(const void *data)
{
	std::unique_lock lock {m_bufferMutex};
	auto offset = 0ull;
	auto bUseExistingSlot = false;
	auto baseAlignedInstanceSize = util::get_aligned_size(m_bufferInstanceSize, m_alignment);
	auto alignedInstanceSize = baseAlignedInstanceSize; // *numInstances;
	if(m_freeOffsets.empty() == false) {
		offset = m_freeOffsets.front();
		m_freeOffsets.pop();
		bUseExistingSlot = true;
	}
	else {
		if(m_assignedMemory + alignedInstanceSize > m_baseSize && EnsureCapacity(1) == false)
			return nullptr;
		offset = m_assignedMemory;
	}

	auto idx = offset / baseAlignedInstanceSize;
	auto pThis = std::dynamic_pointer_cast<IUniformResizableBuffer>(shared_from_this());
	auto subBuffer = CreateSubBuffer(offset, m_bufferInstanceSize, [pThis, idx](IBuffer &subBuffer) {
		std::unique_lock lock {pThis->m_bufferMutex};
		pThis->m_freeOffsets.push(subBuffer.GetStartOffset());
		pThis->m_allocatedSubBuffers.at(idx) = nullptr;
	});
	assert(subBuffer);
	subBuffer->SetParent(*this, idx);
	if(data != nullptr)
		subBuffer->Write(0ull, m_bufferInstanceSize, data);
	m_allocatedSubBuffers.at(idx) = subBuffer.get();
	if(bUseExistingSlot == false)
		m_assignedMemory += alignedInstanceSize;
	return subBuffer;
}
