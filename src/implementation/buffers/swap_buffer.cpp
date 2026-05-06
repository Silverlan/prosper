// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.swap_buffer;

std::shared_ptr<prosper::SwapBuffer> prosper::SwapBuffer::Create(IPrContext &context, IUniformResizableBuffer &buffer, const void *data)
{
	auto numBuffers = context.GetMaxNumberOfFramesInFlight();
	std::vector<std::shared_ptr<IBuffer>> buffers;
	buffers.reserve(numBuffers);
	for(auto i = decltype(numBuffers) {0u}; i < numBuffers; ++i) {
		auto subBuf = buffer.AllocateBuffer(data);
		if(!subBuf)
			return nullptr;
		buffers.push_back(subBuf);
	}
	return std::shared_ptr<SwapBuffer> {new SwapBuffer {context, std::move(buffers)}};
}

std::shared_ptr<prosper::SwapBuffer> prosper::SwapBuffer::Create(IPrContext &context, IDynamicResizableBuffer &buffer, DeviceSize size, uint32_t instanceCount)
{
	auto numBuffers = context.GetMaxNumberOfFramesInFlight();
	auto alignedSize = util::get_aligned_size(size, buffer.GetAlignment());
	std::vector<std::shared_ptr<IBuffer>> buffers;
	buffers.reserve(numBuffers);
	for(auto i = decltype(numBuffers) {0u}; i < numBuffers; ++i)
		buffers.push_back(buffer.AllocateBuffer(alignedSize * instanceCount));
	return std::shared_ptr<SwapBuffer> {new SwapBuffer {context, std::move(buffers)}};
}
std::shared_ptr<prosper::SwapBuffer> prosper::SwapBuffer::Create(IPrContext &context, std::vector<std::shared_ptr<IBuffer>> &&buffers) { return std::shared_ptr<SwapBuffer> {new SwapBuffer {context, std::move(buffers)}}; }

prosper::SwapBuffer::SwapBuffer(IPrContext &context, std::vector<std::shared_ptr<IBuffer>> &&buffers) : ContextObject{context}, m_buffers {std::move(buffers)}
{
	assert(!m_buffers.empty());
	assert(window.GetSwapchainImageCount() == m_buffers.size());
}
prosper::IBuffer *prosper::SwapBuffer::operator->()
{
	auto idx = GetContext().GetFrameResourceIndex();
	return m_buffers[idx].get();
}
prosper::IBuffer &prosper::SwapBuffer::operator*() { return *operator->(); }
uint8_t prosper::SwapBuffer::GetCurrentBufferIndex() const { return GetContext().GetFrameResourceIndex(); }
uint32_t prosper::SwapBuffer::GetCurrentBufferFlag() const { return 1u << GetCurrentBufferIndex(); }
prosper::IBuffer &prosper::SwapBuffer::GetBuffer(SubBufferIndex idx)
{
	assert(idx < m_buffers.size());
	return *m_buffers[idx];
}
prosper::IBuffer &prosper::SwapBuffer::GetCurrentBuffer() const { return *m_buffers[GetCurrentBufferIndex()]; }
prosper::IBuffer &prosper::SwapBuffer::Write(IBuffer::Offset offset, IBuffer::Size size, const void *data, bool flagAsDirty)
{
	if(flagAsDirty) {
		auto &buffers = m_buffers;
		for(auto i = decltype(buffers.size()) {0u}; i < buffers.size(); ++i)
			m_buffersDirty |= 1 << i;
	}

	auto flag = GetCurrentBufferFlag();
	auto &buf = GetCurrentBuffer();
	if((m_buffersDirty & flag) != 0) {
		m_buffersDirty &= ~flag;
		// Data will only be updated if necessary
		buf.Write(offset, size, data);
	}
	return buf;
}
bool prosper::SwapBuffer::IsDirty() const { return m_buffersDirty != 0; }
