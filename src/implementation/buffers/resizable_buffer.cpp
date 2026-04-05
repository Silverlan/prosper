// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.resizable_buffer;

using namespace prosper;

IResizableBuffer::IResizableBuffer(IBuffer &parent, uint64_t maxTotalSize) : IBuffer {parent.GetContext(), parent.GetCreateInfo(), parent.GetStartOffset(), parent.GetSize()}, m_maxTotalSize {maxTotalSize}, m_baseSize {parent.GetCreateInfo().size} {}

void IResizableBuffer::AddReallocationCallback(const std::function<void()> &fCallback) { m_onReallocCallbacks.push_back(fCallback); }

void IResizableBuffer::ReallocateMemory() { ReallocateMemory(m_baseSize); }

void IResizableBuffer::ReallocateMemory(size_t requiredSize)
{
	auto &context = GetContext();
	if(m_reallocationBehavior == ReallocationBehavior::DeviceWaitIdle)
		context.WaitIdle();

	// Re-allocate buffer; Double current size to avoid frequent re-allocation
	auto oldSize = m_baseSize;
	while(m_baseSize < requiredSize)
		m_baseSize *= 2;
	m_baseSize = pragma::math::min(m_baseSize, m_maxTotalSize);
	auto createInfo = m_createInfo;
	createInfo.size = m_baseSize;
	auto newBuffer = context.CreateBuffer(createInfo);
	assert(newBuffer);
	std::vector<uint8_t> oldData(oldSize);
	Read(0ull, oldData.size(), oldData.data());
	if(m_reallocationBehavior == ReallocationBehavior::SafelyFreeOldBuffer)
		ReleaseBufferSafely();

	for(auto *subBuffer : m_allocatedSubBuffers)
		subBuffer->RecreateInternalSubBuffer(*newBuffer);
	newBuffer->Write(0ull, oldData.size(), oldData.data());
	MoveInternalBuffer(*newBuffer);
	m_size = m_baseSize;
	newBuffer = nullptr;
}

void IResizableBuffer::RunReallocationCallbacks()
{
	for(auto *subBuffer : m_allocatedSubBuffers) {
		if(!subBuffer)
			continue;
		subBuffer->CallReallocationCallbacks();
	}

	for(auto &f : m_onReallocCallbacks)
		f();
}
