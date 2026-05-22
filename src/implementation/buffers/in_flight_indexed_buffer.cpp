// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.in_flight_indexed_buffer;

static std::vector<uint8_t> create_initial_data(size_t sizePerBuf, size_t sizePerSubBuf, uint32_t alignment, const void *data)
{
	std::vector<uint8_t> initialData;
	initialData.resize(sizePerBuf);
	if(data) {
		auto alignedSizePerSubBuf = prosper::util::get_aligned_size(sizePerSubBuf, alignment);
		auto numSubBufs = sizePerBuf / alignedSizePerSubBuf;
		size_t offset = 0;
		for(size_t i = 0; i < numSubBufs; ++i) {
			memcpy(initialData.data() + offset, data, sizePerSubBuf);
			offset += alignedSizePerSubBuf;
		}
	}
	return initialData;
}

static std::optional<std::vector<std::shared_ptr<prosper::IBuffer>>> allocate_frame_in_flight_buffers(prosper::IPrContext &context, prosper::IResizableBuffer &buffer, uint32_t alignment, const void *data, size_t sizePerSubBuf)
{
	auto numBufs = context.GetMaxNumberOfFramesInFlight();
	auto sizePerBuf = buffer.GetSize() / numBufs;
	if(alignment > 0)
		sizePerBuf = sizePerBuf - (sizePerBuf % alignment);
	prosper::IBuffer::Offset offset = 0;
	std::vector<std::shared_ptr<prosper::IBuffer>> frameInFlightBuffers;
	frameInFlightBuffers.resize(numBufs);

	auto initialData = create_initial_data(sizePerBuf, sizePerSubBuf, alignment, data);
	for(auto &buf : frameInFlightBuffers) {
		buf = buffer.AllocateSubBuffer(offset, sizePerBuf, initialData.data());
		if(!buf)
			return {};
		offset += sizePerBuf;
	}
	return frameInFlightBuffers;
}
std::shared_ptr<prosper::InFlightIndexedBuffer> prosper::InFlightIndexedBuffer::Create(IResizableBuffer &buffer, size_t sizePerSubBuffer, uint32_t alignment, const void *data)
{
	std::vector<uint8_t> initialSubBufferData;
	initialSubBufferData.resize(sizePerSubBuffer);
	if(data)
		memcpy(initialSubBufferData.data(), data, sizePerSubBuffer);

	auto &context = buffer.GetContext();
	auto frameInFlightBuffers = allocate_frame_in_flight_buffers(context, buffer, alignment, data, sizePerSubBuffer);
	if(!frameInFlightBuffers)
		return nullptr;
	return std::shared_ptr<InFlightIndexedBuffer> {new InFlightIndexedBuffer {buffer.GetContext(), buffer, std::move(*frameInFlightBuffers), sizePerSubBuffer, alignment, std::move(initialSubBufferData)}};
}
prosper::InFlightIndexedBuffer::InFlightIndexedBuffer(IPrContext &context, IResizableBuffer &baseBuffer, std::vector<std::shared_ptr<IBuffer>> &&frameInFlightBuffers, size_t sizePerSubBuffer, uint32_t alignment, std::vector<uint8_t> &&initialSubBufferData)
    : ContextObject {context}, m_baseBuffer {std::dynamic_pointer_cast<IResizableBuffer>(baseBuffer.shared_from_this())}, m_frameInFlightBuffers {std::move(frameInFlightBuffers)}, m_sizePerSubBuffer {sizePerSubBuffer}, m_alignment {alignment},
      m_initialSubBufferData {std::move(initialSubBufferData)}
{
	m_alignedSizePerSubBuffer = util::get_aligned_size(sizePerSubBuffer, m_alignment);
	m_bufferInfos.resize(GetMaxSubBufferCount());
}

size_t prosper::InFlightIndexedBuffer::GetMaxSubBufferCount() const { return GetSize() / m_alignedSizePerSubBuffer; }

size_t prosper::InFlightIndexedBuffer::GetAllocatedBufferCount() const { return m_nextIndex - m_freeIndices.size(); }

std::optional<prosper::InFlightIndexedBuffer::Index> prosper::InFlightIndexedBuffer::Allocate(const void *persistentDataPtr)
{
	std::optional<Index> index {};
	if(!m_freeIndices.empty()) {
		index = m_freeIndices.front();
		m_freeIndices.pop();
	}
	else {
		auto maxSubBuffers = GetMaxSubBufferCount();
		if(m_nextIndex >= maxSubBuffers) {
			if(!IncreaseCapacity())
				return {};
		}
		auto offset = GetOffset(m_nextIndex);
		auto size = m_alignedSizePerSubBuffer;
		auto maxSize = GetSize();
		if(offset + size > maxSize) {
			if(!IncreaseCapacity())
				return {};
		}
		index = m_nextIndex++;
	}
	m_bufferInfos[*index].baseData = persistentDataPtr;
	return index;
}
void prosper::InFlightIndexedBuffer::Free(Index index)
{
	m_bufferInfos[index].baseData = nullptr;
	m_bufferInfos[index].dirtyFrames = 0;
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
		auto &curBuf = m_frameInFlightBuffers[resourceIdx];
		curBuf->Write(GetOffset(index), m_sizePerSubBuffer, m_bufferInfos[index].baseData);
		pragma::math::set_flag(m_bufferInfos[index].dirtyFrames, resourceFlag, false);
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

bool prosper::InFlightIndexedBuffer::SyncDataToGpu(Index index)
{
	size_t offset = 0;
	size_t size = m_sizePerSubBuffer;
	auto *data = m_bufferInfos[index].baseData;

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
	auto &curBuf = m_frameInFlightBuffers[resourceIdx];
	size_t nextOffset = 0;
	for(Index i = 0; i < m_nextIndex; ++i) {
		auto &bufInfo = m_bufferInfos[i];

		auto offset = nextOffset;
		nextOffset += m_alignedSizePerSubBuffer;
		if(bufInfo.dirtyFrames == 0)
			continue;
		if(pragma::math::is_flag_set(bufInfo.dirtyFrames, resourceFlag)) {
			pragma::math::set_flag(bufInfo.dirtyFrames, resourceFlag, false);
			auto *baseData = bufInfo.baseData;
			curBuf->Write(offset, m_sizePerSubBuffer, baseData);
		}
		if(bufInfo.dirtyFrames != 0)
			m_hasDirtyBuffers = true;
	}
}

bool prosper::InFlightIndexedBuffer::IncreaseCapacity(std::optional<size_t> minRequiredSize)
{
	auto &context = GetContext();
	context.WaitIdle(true);
	auto numBufs = context.GetMaxNumberOfFramesInFlight();
	auto curSize = m_baseBuffer->GetSize();
	auto newSize = curSize * 2;
	if(minRequiredSize)
		*minRequiredSize *= numBufs;
	while(minRequiredSize && newSize < *minRequiredSize)
		newSize *= 2;
	m_frameInFlightBuffers.clear();
	if(!m_baseBuffer->Resize(newSize, false))
		throw std::runtime_error {"Failed to re-allocate in-flight indexed buffer"};
	auto newFrameInFlightBuffers = allocate_frame_in_flight_buffers(context, *m_baseBuffer, m_alignment, m_initialSubBufferData.data(), m_sizePerSubBuffer);
	if(!newFrameInFlightBuffers)
		throw std::runtime_error {"Could not allocate in-flight indexed sub-buffer"};

	m_frameInFlightBuffers = std::move(*newFrameInFlightBuffers);
	m_bufferInfos.resize(GetMaxSubBufferCount());
	for(auto &bufInfo : m_bufferInfos) {
		if(!bufInfo.baseData)
			continue;
		bufInfo.dirtyFrames = (1u << m_frameInFlightBuffers.size()) - 1;
	}
	m_hasDirtyBuffers = true;
	return true;
}
