// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.swap_buffer;

export import :buffer.buffer;

export {
	namespace prosper {
		class IPrContext;
		class IUniformResizableBuffer;
		class IDynamicResizableBuffer;
		class DLLPROSPER SwapBuffer : public ContextObject, public std::enable_shared_from_this<SwapBuffer> {
		  public:
			using SubBufferIndex = uint32_t;
			static std::shared_ptr<SwapBuffer> Create(IPrContext &context, IUniformResizableBuffer &buffer, const void *data = nullptr);
			static std::shared_ptr<SwapBuffer> Create(IPrContext &context, IDynamicResizableBuffer &buffer, DeviceSize size, uint32_t instanceCount);
			static std::shared_ptr<SwapBuffer> Create(IPrContext &context, std::vector<std::shared_ptr<IBuffer>> &&buffers);
			IBuffer &GetBuffer(SubBufferIndex idx);
			const IBuffer &GetBuffer(SubBufferIndex idx) const { return const_cast<SwapBuffer *>(this)->GetBuffer(idx); }
			IBuffer &GetCurrentBuffer() const;
			bool IsDirty() const;

			IBuffer &Write(IBuffer::Offset offset, IBuffer::Size size, const void *data, bool flagAsDirty = true);

			IBuffer *operator->();
			const IBuffer *operator->() const { return const_cast<SwapBuffer *>(this)->operator->(); }
			IBuffer &operator*();
			const IBuffer &operator*() const { return const_cast<SwapBuffer *>(this)->operator*(); }
		  private:
			SwapBuffer(IPrContext &context, std::vector<std::shared_ptr<IBuffer>> &&buffers);
			uint8_t GetCurrentBufferIndex() const;
			uint32_t GetCurrentBufferFlag() const;
			std::vector<std::shared_ptr<IBuffer>> m_buffers;
			mutable uint8_t m_buffersDirty = 0;
		};
	};
}
