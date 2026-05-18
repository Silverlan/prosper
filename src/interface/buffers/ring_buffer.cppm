// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.ring_buffer;

export import :buffer.linear_buffer;

export {
	namespace prosper {
		class IResizableBuffer;
		class DLLPROSPER FrameScopedRingBuffer : public ContextObject {
		  public:
			static std::shared_ptr<FrameScopedRingBuffer> Create(IResizableBuffer &buffer);

			bool EnsureCapacity(size_t capacity);
			std::optional<LinearBuffer::BufferOffset> Allocate(size_t size, const void *data);
		  private:
			FrameScopedRingBuffer(IPrContext &context, IResizableBuffer &baseBuffer);
			std::shared_ptr<LinearBuffer> m_baseBuffer;
			IBuffer::Offset m_curOffset = 0;
			FrameIndex m_lastFrameIndex = std::numeric_limits<FrameIndex>::max();
		};
	};
}
