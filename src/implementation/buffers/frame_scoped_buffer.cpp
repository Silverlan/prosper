// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.frame_scoped_buffer;

std::shared_ptr<prosper::FrameScopedBuffer> prosper::FrameScopedBuffer::Create(IUniformResizableBuffer &buffer, const void *persistentDataPtr)
{
	auto subBuf = buffer.AllocateBuffer();
	if(!subBuf)
		return nullptr;
	return std::shared_ptr<FrameScopedBuffer> {new FrameScopedBuffer {buffer, subBuf, persistentDataPtr}};
}
prosper::FrameScopedBuffer::~FrameScopedBuffer() { m_frameInFlightBuffers.clear(); }

prosper::FrameScopedBuffer::FrameScopedBuffer(IUniformResizableBuffer &parentBuffer, std::shared_ptr<IBuffer> &buffer, const void *persistentDataPtr) : ContextObject {parentBuffer.GetContext()}, m_parentBuffer {parentBuffer}, m_cpuData {persistentDataPtr}
{
	m_frameInFlightBuffers.push_back(buffer);
}
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
			for(size_t i = 0; i < m_frameInFlightBuffers.size(); ++i)
				context.KeepResourceAliveUntilPresentationComplete(m_frameInFlightBuffers[i]);
			m_frameInFlightBuffers.clear();
			m_frameInFlightBuffers.push_back(m_parentBuffer.AllocateBuffer(m_cpuData));
			m_dirtyFrameInFlightBuffers = 0;
			break;
		}
	case BufferMode::Dynamic:
		{
			m_frameInFlightBuffers.resize(context.GetMaxNumberOfFramesInFlight());
			auto baseBuf = m_frameInFlightBuffers.front();
			// Keep the old buffer around temporarily in case it is still in use
			context.KeepResourceAliveUntilPresentationComplete(baseBuf);
			m_parentBuffer.EnsureFreeCapacity(m_parentBuffer.GetAllocatedInstanceCount() + m_frameInFlightBuffers.size());
			for(size_t i = 0; i < m_frameInFlightBuffers.size(); ++i) {
				auto subBuf = m_parentBuffer.AllocateBuffer(m_cpuData);
				m_frameInFlightBuffers[i] = subBuf;
			}
			m_dirtyFrameInFlightBuffers = (1u << m_frameInFlightBuffers.size()) - 1;
			break;
		}
	}
}
prosper::IBuffer *prosper::FrameScopedBuffer::operator->() { return &GetCurrentBuffer(); }
prosper::IBuffer &prosper::FrameScopedBuffer::operator*() { return *operator->(); }
void prosper::FrameScopedBuffer::UpdateCurrentBuffer()
{
	if(m_bufferMode == BufferMode::Static)
		return;
	auto &context = GetContext();
	auto resourceIndex = context.GetFrameResourceIndex();
	auto resourceFlag = context.GetFrameResourceFlag();
	if(!pragma::math::is_flag_set(m_dirtyFrameInFlightBuffers, resourceFlag))
		return;
	auto &buf = GetCurrentBuffer();
	buf.Write(0, m_parentBuffer.GetInstanceSize(), m_cpuData);
	pragma::math::set_flag(m_dirtyFrameInFlightBuffers, resourceFlag, false);
}
bool prosper::FrameScopedBuffer::CollapseToSingle()
{
	if(m_bufferMode == BufferMode::Static)
		return false;
	auto &context = GetContext();
	auto curFrame = context.GetLastFrameId();
	auto numFramesPassedSinceLastChange = curFrame - m_lastFrameDataChange;
	if(numFramesPassedSinceLastChange > FRAME_COOLDOWN_THRESHOLD) {
		// If the data hasn't changed in a while, chances are it will stay that way for a while, so we can switch back to static buffer mode.
		ChangeBufferMode(BufferMode::Static);
	}
	return true;
}
prosper::FrameScopedBuffer::BufferChange prosper::FrameScopedBuffer::SyncDataToGpu() { return Write(0, m_parentBuffer.GetInstanceSize(), m_cpuData); }
prosper::FrameScopedBuffer::BufferChange prosper::FrameScopedBuffer::Write(IBuffer::Offset offset, IBuffer::Size size, const void *data)
{
	//auto *curDataPtr = static_cast<uint8_t *>(GetCurrentBuffer().GetMappedDataPointer());
	//if(std::memcmp(curDataPtr + offset, data, size) == 0)
	//	return {};

	auto &context = GetContext();
	auto curFrame = context.GetLastFrameId();
	auto change = BufferChange::NoChange;
	if(m_bufferMode == BufferMode::Static) {
		ChangeBufferMode(BufferMode::Dynamic);
		change = BufferChange::ToDynamic;
	}

	m_lastFrameDataChange = curFrame;
	m_dirtyFrameInFlightBuffers = (1u << m_frameInFlightBuffers.size()) - 1;

	auto &buf = GetCurrentBuffer();
	buf.Write(offset, size, data);
	if(offset == 0 && size == buf.GetSize())
		pragma::math::set_flag(m_dirtyFrameInFlightBuffers, context.GetFrameResourceFlag(), false);
	return change;
}
