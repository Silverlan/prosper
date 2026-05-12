// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.frame_scoped_buffer;

std::shared_ptr<prosper::FrameScopedBuffer> prosper::FrameScopedBuffer::Create(IUniformResizableBuffer &buffer)
{
	auto subBuf = buffer.AllocateBuffer();
	if(!subBuf)
		return nullptr;
	return std::shared_ptr<FrameScopedBuffer> {new FrameScopedBuffer {buffer, subBuf}};
}
prosper::FrameScopedBuffer::~FrameScopedBuffer() { m_frameInFlightBuffers.clear(); }

prosper::FrameScopedBuffer::FrameScopedBuffer(IUniformResizableBuffer &parentBuffer, std::shared_ptr<IBuffer> &buffer) : ContextObject {parentBuffer.GetContext()}, m_parentBuffer {parentBuffer} { m_frameInFlightBuffers.push_back(buffer); }
prosper::IBuffer &prosper::FrameScopedBuffer::GetCurrentBuffer() const
{
	if(m_bufferMode == BufferMode::Static)
		return *m_frameInFlightBuffers.front();
	return *m_frameInFlightBuffers[GetContext().GetFrameResourceIndex()];
}
prosper::IBuffer &prosper::FrameScopedBuffer::GetBuffer(uint32_t frameResourceIndex) const
{
	if(m_bufferMode == BufferMode::Static)
		return *m_frameInFlightBuffers.front();
	return *m_frameInFlightBuffers[frameResourceIndex];
}
void prosper::FrameScopedBuffer::ChangeBufferMode(BufferMode bufferMode)
{
	if(bufferMode == m_bufferMode)
		return;
	m_bufferMode = bufferMode;

	auto &context = GetContext();
	switch(bufferMode) {
	case BufferMode::Static:
		{
			auto resourceIndex = context.GetFrameResourceIndex();
			auto prevResourceIndex = (resourceIndex == 0) ? (context.GetMaxNumberOfFramesInFlight() - 1) : (resourceIndex - 1);
			if(prevResourceIndex != 0) {
				// We'll only keep the first buffer in the table, so we'll swap it out with whatever buffer was used last
				// to make sure the data is up-to-date and so we don't need to copy anything.
				std::swap(m_frameInFlightBuffers.front(), m_frameInFlightBuffers[prevResourceIndex]);
			}

			for(size_t i = 1; i < m_frameInFlightBuffers.size(); ++i)
				context.KeepResourceAliveUntilPresentationComplete(m_frameInFlightBuffers[i]);
			m_frameInFlightBuffers.resize(1);
			m_dirtyFrameInFlightBuffers = 0;
			break;
		}
	case BufferMode::Dynamic:
		{
			m_frameInFlightBuffers.resize(context.GetMaxNumberOfFramesInFlight());
			auto &baseBuf = m_frameInFlightBuffers.front();
			// Keep the old buffer around temporarily in case it is still in use
			context.KeepResourceAliveUntilPresentationComplete(baseBuf);
			m_parentBuffer.EnsureFreeCapacity(m_parentBuffer.GetAllocatedInstanceCount() + m_frameInFlightBuffers.size());
			auto *srcData = baseBuf->GetMappedDataPointer();
			for(size_t i = 0; i < m_frameInFlightBuffers.size(); ++i) {
				auto subBuf = m_parentBuffer.AllocateBuffer(srcData);
				m_frameInFlightBuffers[i] = subBuf;

				// Since we used EnsureCapacity, AllocateBuffer should not have resized the internal buffers and the mapped data pointer
				// should still be the same, but we'll double-check to be sure.
				if(context.IsValidationEnabled() && baseBuf->GetMappedDataPointer() != srcData)
					context.ValidationCallback(DebugMessageSeverityFlags::ErrorBit, "Allocation in frame-scoped buffer resulted in internal buffer re-allocation.");
				assert(baseBuf->GetMappedDataPointer() == srcData);
			}
			m_dirtyFrameInFlightBuffers = (1u << m_frameInFlightBuffers.size()) - 1;
			break;
		}
	}
}
prosper::IBuffer *prosper::FrameScopedBuffer::operator->() { return &GetCurrentBuffer(); }
prosper::IBuffer &prosper::FrameScopedBuffer::operator*() { return *operator->(); }
void prosper::FrameScopedBuffer::Update()
{
	if(m_bufferMode == BufferMode::Static || m_dirtyFrameInFlightBuffers == 0)
		return;
	auto &context = GetContext();
	auto resourceIndex = context.GetFrameResourceIndex();
	auto resourceFlag = static_cast<uint8_t>(1u << resourceIndex);
	if(!pragma::math::is_flag_set(m_dirtyFrameInFlightBuffers, resourceFlag))
		return;
	// Copy data from previous buffer
	auto prevResourceIndex = (resourceIndex == 0) ? (context.GetMaxNumberOfFramesInFlight() - 1) : (resourceIndex - 1);
	auto &prevBuf = *m_frameInFlightBuffers[prevResourceIndex];
	Write(0u, prevBuf.GetSize(), prevBuf.GetMappedDataPointer());
}
std::optional<prosper::FrameScopedBuffer::BufferChange> prosper::FrameScopedBuffer::Write(IBuffer::Offset offset, IBuffer::Size size, const void *data)
{
	auto *curDataPtr = static_cast<uint8_t *>(GetCurrentBuffer().GetMappedDataPointer());
	if(std::memcmp(curDataPtr + offset, data, size) == 0)
		return {};

	auto &context = GetContext();
	auto curFrame = context.GetLastFrameId(); // TODO: This should be updated immediately after present
	auto numFramesPassedSinceLastChange = curFrame - m_lastFrameDataChange;
	auto maxFramesInFlight = context.GetMaxNumberOfFramesInFlight();
	auto change = BufferChange::NoChange;
	if(numFramesPassedSinceLastChange > 0) { // If delta frames is 0, we already updated the buffer this frame and don't need to update again
		if(m_bufferMode == BufferMode::Static) {
			if(numFramesPassedSinceLastChange < maxFramesInFlight) {
				// Data was just changed last frame,  we'll have to switch to dynamic buffer mode.
				ChangeBufferMode(BufferMode::Dynamic);
				change = BufferChange::ToDynamic;
			}
		}
		else if(numFramesPassedSinceLastChange > FRAME_COOLDOWN_THRESHOLD) {
			// If the data hasn't changed in a while, chances are it will stay that way for a while, so we can switch back to static buffer mode.
			ChangeBufferMode(BufferMode::Static);
			change = BufferChange::ToStatic;
		}
	}

	m_lastFrameDataChange = curFrame;
	auto &buf = GetCurrentBuffer();
	buf.Write(offset, size, data);
	if(offset == 0 && size == buf.GetSize())
		pragma::math::set_flag(m_dirtyFrameInFlightBuffers, context.GetFrameResourceFlag(), false);
	return change;
}
