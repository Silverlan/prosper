// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.linear_buffer;

std::shared_ptr<prosper::LinearBuffer> prosper::LinearBuffer::Create(IBuffer &buffer, uint32_t alignment) { return std::shared_ptr<LinearBuffer> {new LinearBuffer {buffer, alignment}}; }
prosper::LinearBuffer::~LinearBuffer() {}

prosper::LinearBuffer::LinearBuffer(IBuffer &buffer, uint32_t alignment) : ContextObject {buffer.GetContext()}, m_alignment {alignment}, m_baseBuffer {buffer.shared_from_this()} {}

std::optional<prosper::LinearBuffer::BufferOffset> prosper::LinearBuffer::Allocate(size_t size, const void *data)
{
	auto maxSize = m_baseBuffer->GetSize();
	if(m_currentOffset + size > maxSize) {
		if(size > maxSize)
			return {}; // TODO: Re-allocate?
		// Wrap back to beginning
		m_currentOffset = 0;
	}

	auto offset = m_currentOffset;
	m_baseBuffer->Write(offset, size, data);

	m_currentOffset = util::get_aligned_size(m_currentOffset + size, m_alignment);
	return offset;
}
void prosper::LinearBuffer::ResetOffset() { m_currentOffset = 0; }
