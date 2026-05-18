// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.frame_scoped_buffer;

export import :buffer.buffer;

export {
	namespace prosper {
		class IPrContext;
		class IUniformResizableBuffer;
		class IDynamicResizableBuffer;
		class DLLPROSPER FrameScopedBuffer : public ContextObject, public std::enable_shared_from_this<FrameScopedBuffer> {
		  public:
			static constexpr uint64_t FRAME_COOLDOWN_THRESHOLD = 80;
			enum class BufferMode : uint8_t {
				Static = 0,
				Dynamic,
			};
			// If a buffer change has occurred, descriptor sets will have to be updated
			// accordingly
			enum class BufferChange : uint8_t {
				NoChange = 0,
				ToDynamic,
				ToStatic,
			};
			static std::shared_ptr<FrameScopedBuffer> Create(IUniformResizableBuffer &buffer);
			~FrameScopedBuffer() override;

			BufferMode GetBufferMode() const { return m_bufferMode; }

			IBuffer &GetCurrentBuffer() const;
			IBuffer &GetBuffer(uint32_t frameResourceIndex) const;
			std::optional<BufferChange> UpdateBufferMode(IBuffer::Offset offset, IBuffer::Size size, const void *data);
			std::optional<BufferChange> Write(IBuffer::Offset offset, IBuffer::Size size, const void *data);
			void Update();

			IBuffer *operator->();
			const IBuffer *operator->() const { return const_cast<FrameScopedBuffer *>(this)->operator->(); }
			IBuffer &operator*();
			const IBuffer &operator*() const { return const_cast<FrameScopedBuffer *>(this)->operator*(); }
		  private:
			FrameScopedBuffer(IUniformResizableBuffer &parentBuffer, std::shared_ptr<IBuffer> &buffer);
			void ChangeBufferMode(BufferMode bufferMode);
			IUniformResizableBuffer &m_parentBuffer;
			BufferMode m_bufferMode = BufferMode::Static;
			std::vector<std::shared_ptr<IBuffer>> m_frameInFlightBuffers;
			uint8_t m_dirtyFrameInFlightBuffers = 0;
			uint64_t m_lastFrameDataChange = 0;
		};
	};
}
