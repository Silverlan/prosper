/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "buffers/prosper_swap_buffer.hpp"
#include "buffers/prosper_uniform_resizable_buffer.hpp"
#include "buffers/prosper_dynamic_resizable_buffer.hpp"
#include "prosper_window.hpp"

std::shared_ptr<prosper::SwapBuffer> prosper::SwapBuffer::Create(Window &window, IUniformResizableBuffer &buffer, const void *data)
{
	auto numBuffers = window.GetSwapchainImageCount();
	std::vector<std::shared_ptr<IBuffer>> buffers;
	buffers.reserve(numBuffers);
	for(auto i = decltype(numBuffers) {0u}; i < numBuffers; ++i)
		buffers.push_back(buffer.AllocateBuffer(data));
	return std::shared_ptr<SwapBuffer> {new SwapBuffer {window, std::move(buffers)}};
}

std::shared_ptr<prosper::SwapBuffer> prosper::SwapBuffer::Create(Window &window, IDynamicResizableBuffer &buffer, DeviceSize size, uint32_t instanceCount)
{
	auto numBuffers = window.GetSwapchainImageCount();
	auto alignedSize = util::get_aligned_size(size, buffer.GetAlignment());
	std::vector<std::shared_ptr<IBuffer>> buffers;
	buffers.reserve(numBuffers);
	for(auto i = decltype(numBuffers) {0u}; i < numBuffers; ++i)
		buffers.push_back(buffer.AllocateBuffer(alignedSize * instanceCount));
	return std::shared_ptr<SwapBuffer> {new SwapBuffer {window, std::move(buffers)}};
}

prosper::SwapBuffer::SwapBuffer(Window &window, std::vector<std::shared_ptr<IBuffer>> &&buffers) : m_buffers {std::move(buffers)}, m_window {window.shared_from_this()}, m_windowPtr {&window}
{
	assert(!m_buffers.empty());
	assert(m_buffers[0]->GetContext().GetSwapchainImageCount() == m_buffers.size());
}
prosper::IBuffer *prosper::SwapBuffer::operator->()
{
	if(m_window.expired())
		return nullptr;
	auto idx = m_windowPtr->GetLastAcquiredSwapchainImageIndex();
	return m_buffers[idx].get();
}
prosper::IBuffer &prosper::SwapBuffer::operator*() { return *operator->(); }
uint32_t prosper::SwapBuffer::GetCurrentBufferIndex() const { return m_currentBufferIndex; }
uint32_t prosper::SwapBuffer::GetCurrentBufferFlag() const { return 1u << GetCurrentBufferIndex(); }
prosper::IBuffer &prosper::SwapBuffer::GetBuffer(SubBufferIndex idx)
{
	assert(idx < m_buffers.size());
	return *m_buffers[idx];
}
prosper::IBuffer &prosper::SwapBuffer::GetBuffer() const { return *m_buffers[GetCurrentBufferIndex()]; }
prosper::IBuffer &prosper::SwapBuffer::Update(IBuffer::Offset offset, IBuffer::Size size, const void *data, bool flagAsDirty)
{
	if(m_window.expired())
		throw std::runtime_error {"Invalid swap buffer window!"};
	auto prevIdx = m_currentBufferIndex;
	m_currentBufferIndex = m_windowPtr->GetLastAcquiredSwapchainImageIndex();

	if(flagAsDirty) {
		auto &buffers = m_buffers;
		for(auto i = decltype(buffers.size()) {0u}; i < buffers.size(); ++i)
			m_buffersDirty |= 1 << i;
	}

	auto flag = GetCurrentBufferFlag();
	if((m_buffersDirty & flag) != 0) {
		m_buffersDirty &= ~flag;
		// Data will only be updated if necessary
		GetBuffer().Write(offset, size, data);
	}
	return GetBuffer();
}
bool prosper::SwapBuffer::IsDirty() const { return m_buffersDirty != 0; }
