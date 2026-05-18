// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.buffer_view;

export import :buffer.buffer;

export {
	namespace prosper {
		class DLLPROSPER BufferView {
		  public:
			BufferView(IBuffer &buffer, IBuffer::Offset offset, IBuffer::Size size);
			BufferView(IBuffer &buffer);

			bool Write(IBuffer::Offset offset, IBuffer::Size size, const void *data);
			bool Read(IBuffer::Offset offset, IBuffer::Size size, void *outData);

			IBuffer::Offset GetOffset() const { return m_offset; }
			IBuffer::Size GetSize() const { return m_size; }

			IBuffer *operator->() { return &m_buffer; }
			IBuffer &operator*() { return m_buffer; }
		  private:
			IBuffer &m_buffer;
			IBuffer::Offset m_offset = 0;
			IBuffer::Size m_size = 0;
		};
	};
}
