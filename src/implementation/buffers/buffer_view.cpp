// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :buffer.buffer_view;

using namespace prosper;

BufferView::BufferView(IBuffer &buffer, IBuffer::Offset offset, IBuffer::Size size) : m_buffer {buffer}, m_offset {offset}, m_size {size} {}
BufferView::BufferView(IBuffer &buffer) : BufferView {buffer, 0, buffer.GetSize()} {}

bool BufferView::Write(IBuffer::Offset offset, IBuffer::Size size, const void *data)
{
	if(offset + size > m_size)
		return false;
	return m_buffer.Write(m_offset + offset, size, data);
}
bool BufferView::Read(IBuffer::Offset offset, IBuffer::Size size, void *outData)
{
	if(offset + size > m_size)
		return false;
	return m_buffer.Read(m_offset + offset, size, outData);
}
