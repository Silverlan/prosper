// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.swap_buffer;

export import :buffer.buffer;

export {
	namespace prosper {
		class Window;
		class IUniformResizableBuffer;
		class IDynamicResizableBuffer;
		class DLLPROSPER SwapBuffer : public std::enable_shared_from_this<SwapBuffer> {
		  public:
			using SubBufferIndex = uint32_t;
			static std::shared_ptr<SwapBuffer> Create(Window &window, IUniformResizableBuffer &buffer, const void *data = nullptr);
			static std::shared_ptr<SwapBuffer> Create(Window &window, IDynamicResizableBuffer &buffer, DeviceSize size, uint32_t instanceCount);
			IBuffer &GetBuffer(SubBufferIndex idx);
			const IBuffer &GetBuffer(SubBufferIndex idx) const { return const_cast<SwapBuffer *>(this)->GetBuffer(idx); }
			IBuffer &GetBuffer() const;
			bool IsDirty() const;

			IBuffer &Update(IBuffer::Offset offset, IBuffer::Size size, const void *data, bool flagAsDirty = true);

			IBuffer *operator->();
			const IBuffer *operator->() const { return const_cast<SwapBuffer *>(this)->operator->(); }
			IBuffer &operator*();
			const IBuffer &operator*() const { return const_cast<SwapBuffer *>(this)->operator*(); }
		  private:
			SwapBuffer(Window &window, std::vector<std::shared_ptr<IBuffer>> &&buffers);
			uint32_t GetCurrentBufferIndex() const;
			uint32_t GetCurrentBufferFlag() const;
			std::vector<std::shared_ptr<IBuffer>> m_buffers;
			uint32_t m_currentBufferIndex = 0;
			mutable uint8_t m_buffersDirty = 0;
			std::weak_ptr<Window> m_window {};
			Window *m_windowPtr = nullptr;
		};
	};
}
