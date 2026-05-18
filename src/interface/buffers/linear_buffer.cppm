// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.linear_buffer;

export import :buffer.buffer;
export import :types;

export {
	namespace prosper {
		class IPrContext;
		class IBuffer;
		class DLLPROSPER LinearBuffer : public ContextObject, public std::enable_shared_from_this<LinearBuffer> {
		public:
			using BufferOffset = size_t;

			static std::shared_ptr<LinearBuffer> Create(IBuffer &buffer, uint32_t alignment = 0);
			~LinearBuffer() override;

			std::optional<BufferOffset> Allocate(size_t size, const void *data);
			[[nodiscard]] IBuffer &GetBaseBuffer() const { return *m_baseBuffer; }
			void ResetOffset();
		private:
			LinearBuffer(IBuffer &buffer, uint32_t alignment);
			std::shared_ptr<IBuffer> m_baseBuffer;
			BufferOffset m_currentOffset = 0;
			uint32_t m_alignment = 0;
		};
	};
}
