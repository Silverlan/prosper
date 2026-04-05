// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.resizable_buffer;

using namespace prosper;

IResizableBuffer::IResizableBuffer(IBuffer &parent) : IBuffer {parent.GetContext(), parent.GetCreateInfo(), parent.GetStartOffset(), parent.GetSize()}, m_baseSize {parent.GetCreateInfo().size} {}

void IResizableBuffer::AddReallocationCallback(const std::function<void()> &fCallback) { m_onReallocCallbacks.push_back(fCallback); }

void IResizableBuffer::ReallocateMemory() { ReallocateMemory(m_baseSize); }

bool IResizableBuffer::ReallocateMemory(size_t requiredSize)
{
	if(!IsResizable())
		return false;
	auto &context = GetContext();
	if(m_reallocationBehavior == ReallocationBehavior::DeviceWaitIdle)
		context.WaitIdle();
	context.Log("Reallocating prosper buffer '" + GetDebugName() + "' of size " + pragma::util::get_pretty_bytes(m_baseSize) + " to " + pragma::util::get_pretty_bytes(requiredSize) + "...");

	// Re-allocate buffer; Double current size to avoid frequent re-allocation
	auto oldSize = m_baseSize;
	while(m_baseSize < requiredSize)
		m_baseSize *= 2;
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
	return true;
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
