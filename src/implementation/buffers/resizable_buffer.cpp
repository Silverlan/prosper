// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.resizable_buffer;

using namespace prosper;

IBaseResizableBuffer::IBaseResizableBuffer(IBuffer &parent) : IBuffer {parent.GetContext(), parent.GetCreateInfo(), parent.GetStartOffset(), parent.GetSize()}, m_baseSize {parent.GetCreateInfo().size} {}

void IBaseResizableBuffer::AddReallocationCallback(const std::function<void()> &fCallback) { m_onReallocCallbacks.push_back(fCallback); }

void IBaseResizableBuffer::ReallocateMemory() { ReallocateMemory(m_baseSize); }

bool IBaseResizableBuffer::Resize(size_t size)
{
	if(!IsResizable())
		return false;
	auto &context = GetContext();
	if(m_reallocationBehavior == ReallocationBehavior::DeviceWaitIdle)
		context.WaitIdle();
	context.Log("Reallocating prosper buffer '" + GetDebugName() + "' of size " + pragma::util::get_pretty_bytes(m_baseSize) + " to " + pragma::util::get_pretty_bytes(size) + "...");

	auto oldSize = m_baseSize;
	m_baseSize = size;
	auto createInfo = m_createInfo;
	createInfo.size = m_baseSize;
	auto newBuffer = context.CreateBuffer(createInfo);
	assert(newBuffer);
	std::vector<uint8_t> oldData(oldSize);
	Read(0ull, oldData.size(), oldData.data());
	if(m_reallocationBehavior == ReallocationBehavior::SafelyFreeOldBuffer)
		ReleaseBufferSafely();

	for(auto *subBuffer : m_allocatedSubBuffers) {
		if(!subBuffer)
			continue;
		subBuffer->RecreateInternalSubBuffer(*newBuffer);
	}
	newBuffer->Write(0ull, oldData.size(), oldData.data());
	MoveInternalBuffer(*newBuffer);
	m_size = m_baseSize;
	newBuffer = nullptr;
	return true;
}

bool IBaseResizableBuffer::ReallocateMemory(size_t requiredSize)
{
	// Re-allocate buffer; Double current size to avoid frequent re-allocation
	auto oldSize = m_baseSize;
	auto newSize = m_baseSize;
	while(newSize < requiredSize)
		newSize *= 2;
	return Resize(newSize);
}

void IBaseResizableBuffer::RunReallocationCallbacks()
{
	for(auto *subBuffer : m_allocatedSubBuffers) {
		if(!subBuffer)
			continue;
		subBuffer->CallReallocationCallbacks();
	}

	for(auto &f : m_onReallocCallbacks)
		f();
}

IResizableBuffer::IResizableBuffer(IBuffer &parent) : IBaseResizableBuffer {parent} {}
bool IResizableBuffer::Resize(size_t newSize) { return IBaseResizableBuffer::Resize(newSize); }
std::shared_ptr<IBuffer> IResizableBuffer::AllocateSubBuffer(Offset offset, DeviceSize size, const void *data)
{
	if(offset + size > m_baseSize)
		return nullptr;
	size_t bufIdx = m_allocatedSubBuffers.size();
	auto it = std::find_if(m_allocatedSubBuffers.begin(), m_allocatedSubBuffers.end(), [](IBuffer *ptr) { return !ptr; });
	if(it != m_allocatedSubBuffers.end())
		bufIdx = (it - m_allocatedSubBuffers.begin());
	else
		m_allocatedSubBuffers.resize(bufIdx + 1);

	auto subBuffer = CreateSubBuffer(offset, size, [this, bufIdx](IBuffer &subBuffer) { m_allocatedSubBuffers[bufIdx] = nullptr; });
	if(!subBuffer)
		return nullptr;
	subBuffer->SetParent(*this, bufIdx);
	if(data != nullptr)
		subBuffer->Write(0ull, size, data);
	m_allocatedSubBuffers[bufIdx] = subBuffer.get();
	return subBuffer;
}
