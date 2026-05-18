// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.in_flight_indexed_buffer;

std::shared_ptr<prosper::InFlightIndexedBuffer> prosper::InFlightIndexedBuffer::Create(IResizableBuffer &buffer, size_t sizePerSubBuffer, uint32_t alignment, const void *data)
{
	auto &context = buffer.GetContext();
	auto numBufs = context.GetMaxNumberOfFramesInFlight();
	auto sizePerBuf = buffer.GetSize() / numBufs;
	if(alignment > 0)
		sizePerBuf = sizePerBuf - (sizePerBuf % alignment);
	IBuffer::Offset offset = 0;
	std::vector<std::shared_ptr<IBuffer>> frameInFlightBuffers;
	frameInFlightBuffers.resize(numBufs);
	for(auto &buf : frameInFlightBuffers) {
		buf = buffer.AllocateSubBuffer(offset, sizePerBuf, data);
		if(!buf)
			return nullptr;
		offset += sizePerBuf;
	}
	return std::shared_ptr<InFlightIndexedBuffer> {new InFlightIndexedBuffer {buffer.GetContext(), buffer, std::move(frameInFlightBuffers), sizePerSubBuffer, alignment}};
}
prosper::InFlightIndexedBuffer::InFlightIndexedBuffer(IPrContext &context, IResizableBuffer &baseBuffer, std::vector<std::shared_ptr<IBuffer>> &&frameInFlightBuffers, size_t sizePerSubBuffer, uint32_t alignment)
    : ContextObject {context}, m_baseBuffer {std::dynamic_pointer_cast<IResizableBuffer>(baseBuffer.shared_from_this())}, m_frameInFlightBuffers {std::move(frameInFlightBuffers)}, m_sizePerSubBuffer {sizePerSubBuffer}, m_alignment {alignment}
{
	m_alignedSizePerSubBuffer = util::get_aligned_size(sizePerSubBuffer, m_alignment);
	m_bufferInfos.resize(GetMaxSubBufferCount());
}

size_t prosper::InFlightIndexedBuffer::GetMaxSubBufferCount() const { return GetSize() / m_alignedSizePerSubBuffer; }

size_t prosper::InFlightIndexedBuffer::GetAllocatedBufferCount() const { return m_nextIndex - m_freeIndices.size(); }

std::optional<prosper::InFlightIndexedBuffer::Index> prosper::InFlightIndexedBuffer::Allocate(const void *baseData)
{
	std::optional<Index> index {};
	if(!m_freeIndices.empty()) {
		index = m_freeIndices.front();
		m_freeIndices.pop();
	}
	else {
		auto offset = GetOffset(m_nextIndex);
		auto size = m_alignedSizePerSubBuffer;
		auto maxSize = GetSize();
		if(offset + size > maxSize)
			return {}; // TODO: Re-allocate
		index = m_nextIndex++;
	}
	m_bufferInfos[*index].baseData = baseData;
	return index;
}
void prosper::InFlightIndexedBuffer::Free(Index index)
{
	m_bufferInfos[index].baseData = nullptr;
	if(index == m_nextIndex - 1) {
		--m_nextIndex;
		return;
	}
	m_freeIndices.push(index);
}

prosper::IBuffer::Offset prosper::InFlightIndexedBuffer::GetOffset(Index index) const { return index * m_alignedSizePerSubBuffer; }

size_t prosper::InFlightIndexedBuffer::GetSize() const { return m_frameInFlightBuffers.front()->GetSize(); }

prosper::IBuffer &prosper::InFlightIndexedBuffer::GetCurrentBuffer() const { return *m_frameInFlightBuffers[GetContext().GetFrameResourceIndex()]; }
prosper::IBuffer &prosper::InFlightIndexedBuffer::GetBuffer(uint32_t frameResourceIndex) const { return *m_frameInFlightBuffers[frameResourceIndex]; }

bool prosper::InFlightIndexedBuffer::UpdateDirtyBuffer(Index index)
{
	auto &context = GetContext();
	auto resourceIdx = context.GetFrameResourceIndex();
	auto resourceFlag = context.GetFrameResourceFlag();
	// If we're writing a sub-portion, we have to make sure the current buffer is up-to-date
	if(pragma::math::is_flag_set(m_bufferInfos[index].dirtyFrames, resourceFlag)) {
		auto prevResourceIdx = context.GetPreviousFrameResourceIndex(resourceIdx);
		auto &prevBuf = m_frameInFlightBuffers[prevResourceIdx];
		auto &curBuf = m_frameInFlightBuffers[resourceIdx];
		curBuf->Write(GetOffset(index), m_sizePerSubBuffer, static_cast<uint8_t *>(prevBuf->GetMappedDataPointer()) + GetOffset(index));
		return true;
	}
	return false;
}

prosper::BufferView prosper::InFlightIndexedBuffer::GetCurrentBufferView(Index index) const { return GetBufferView(index, GetContext().GetFrameResourceIndex()); }

prosper::BufferView prosper::InFlightIndexedBuffer::GetBufferView(Index index, uint8_t frameResourceIndex) const { return BufferView {*m_frameInFlightBuffers[frameResourceIndex], GetOffset(index), m_sizePerSubBuffer}; }

bool prosper::InFlightIndexedBuffer::Read(Index index, IBuffer::Offset offset, IBuffer::Size size, void *outData)
{
	UpdateDirtyBuffer(index);

	auto &context = GetContext();
	auto resourceIdx = context.GetFrameResourceIndex();
	offset += GetOffset(index);
	return m_frameInFlightBuffers[resourceIdx]->Read(offset, size, outData);
}

bool prosper::InFlightIndexedBuffer::Write(Index index, IBuffer::Offset offset, IBuffer::Size size, const void *data)
{
	if(offset != 0 || size != m_sizePerSubBuffer)
		UpdateDirtyBuffer(index);

	auto &context = GetContext();
	auto resourceIdx = context.GetFrameResourceIndex();
	auto resourceFlag = context.GetFrameResourceFlag();
	offset += GetOffset(index);
	auto &buf = m_frameInFlightBuffers[resourceIdx];
	if(!buf->Write(offset, size, data))
		return false;
	m_bufferInfos[index].dirtyFrames = (1u << m_frameInFlightBuffers.size()) - 1;
	pragma::math::set_flag(m_bufferInfos[index].dirtyFrames, resourceFlag, false);
	m_hasDirtyBuffers = true;
	return true;
}

void prosper::InFlightIndexedBuffer::UpdateDirtyBuffers()
{
	if(!m_hasDirtyBuffers)
		return;
	m_hasDirtyBuffers = false;
	auto &context = GetContext();
	auto resourceIdx = context.GetFrameResourceIndex();
	auto resourceFlag = context.GetFrameResourceFlag();
	auto prevResourceIdx = context.GetPreviousFrameResourceIndex(resourceIdx);
	auto &prevBuf = m_frameInFlightBuffers[prevResourceIdx];
	auto &curBuf = m_frameInFlightBuffers[resourceIdx];
	auto *prevData = static_cast<uint8_t *>(prevBuf->GetMappedDataPointer());
	size_t nextOffset = 0;
	for(Index i = 0; i < m_nextIndex; ++i) {
		auto &bufInfo = m_bufferInfos[i];

		auto offset = nextOffset;
		nextOffset += m_alignedSizePerSubBuffer;
		if(bufInfo.dirtyFrames == 0)
			continue;
		pragma::math::set_flag(bufInfo.dirtyFrames, resourceFlag, false);
		if(bufInfo.dirtyFrames != 0)
			m_hasDirtyBuffers = true;
		// Ideally copy from provided base data. If none was specified, copy from previous frame-in-flight data.
		// The latter only works if UpdateDirtyBuffers is called *every* frame, otherwise stale data may be used.
		auto *baseData = bufInfo.baseData;
		if(!baseData)
			baseData = prevData + offset;
		curBuf->Write(offset, m_sizePerSubBuffer, baseData);
	}
}

bool prosper::InFlightIndexedBuffer::EnsureCapacity(size_t capacity)
{
	// TODO
	/*auto &context = GetContext();
	auto numBuffers = context.GetMaxNumberOfFramesInFlight();
	auto totalCapacity = capacity *numBuffers;
	return m_baseBuffer->Resize(totalCapacity);*/
	return false;
}
