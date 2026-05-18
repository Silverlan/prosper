// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.ring_buffer;

std::shared_ptr<prosper::FrameScopedRingBuffer> prosper::FrameScopedRingBuffer::Create(IResizableBuffer &buffer) { return std::shared_ptr<FrameScopedRingBuffer> {new FrameScopedRingBuffer {buffer.GetContext(), buffer}}; }
prosper::FrameScopedRingBuffer::FrameScopedRingBuffer(IPrContext &context, IResizableBuffer &baseBuffer) : ContextObject {context}/*,
m_baseBuffer {std::dynamic_pointer_cast<IResizableBuffer>(baseBuffer.shared_from_this())}*/
{
	//ReallocateInternalBuffers();
}

bool prosper::FrameScopedRingBuffer::EnsureCapacity(size_t capacity)
{
	/*auto &context = GetContext();
	auto numBuffers = context.GetMaxNumberOfFramesInFlight();
	auto totalCapacity = capacity *numBuffers;
	return m_baseBuffer->Resize(totalCapacity);*/
	return false;
}

std::optional<prosper::LinearBuffer::BufferOffset> prosper::FrameScopedRingBuffer::Allocate(size_t size, const void *data)
{
	/*if(size > m_baseBuffer->GetBaseBuffer().GetSize())
		return {};
	//m_baseBuffer-

	auto &context = GetContext();
	auto resourceIdx = context.GetFrameResourceIndex();
	auto curFrameId = context.GetLastFrameId();
	if(curFrameId != m_lastFrameIndex) {
		m_lastFrameIndex = curFrameId;
		m_frameRingBuffers[resourceIdx]->ResetOffset();
	}
	return m_frameRingBuffers[resourceIdx]->Allocate(size, data);*/
	return {};
}
