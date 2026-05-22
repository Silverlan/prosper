// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.in_flight_indexed_buffer;

export import :buffer.linear_buffer;
export import :buffer.buffer_view;

export {
	namespace prosper {
		class IResizableBuffer;
		class DLLPROSPER InFlightIndexedBuffer : public ContextObject {
		  public:
			using Index = uint32_t;

			static std::shared_ptr<InFlightIndexedBuffer> Create(IResizableBuffer &buffer, size_t sizePerSubBuffer, uint32_t alignment, const void *data = nullptr);

			bool IncreaseCapacity(std::optional<size_t> minRequiredSize = {});
			std::optional<Index> Allocate(const void *persistentDataPtr);
			void Free(Index index);
			IBuffer::Offset GetOffset(Index index) const;
			size_t GetSize() const;
			size_t GetMaxSubBufferCount() const;
			size_t GetAllocatedBufferCount() const;
			size_t GetStride() const { return m_alignedSizePerSubBuffer; }

			IBuffer &GetCurrentBuffer() const;
			IBuffer &GetBuffer(uint32_t frameResourceIndex) const;
			IResizableBuffer &GetBaseBuffer() const { return *m_baseBuffer; }

			bool SyncDataToGpu(Index index);
			BufferView GetCurrentBufferView(Index index) const;
			BufferView GetBufferView(Index index, uint8_t frameResourceIndex) const;
			void UpdateDirtyBuffers();
		  private:
			InFlightIndexedBuffer(IPrContext &context, IResizableBuffer &baseBuffer, std::vector<std::shared_ptr<IBuffer>> &&frameInFlightBuffers, size_t sizePerSubBuffer, uint32_t alignment, std::vector<uint8_t> &&initialSubBufferData);
			bool Read(Index index, IBuffer::Offset offset, IBuffer::Size size, void *outData);
			bool UpdateDirtyBuffer(Index index);
			struct ItemInfo {
				uint8_t dirtyFrames = 0;
				const void *baseData = nullptr;
			};
			std::queue<Index> m_freeIndices;
			std::shared_ptr<IResizableBuffer> m_baseBuffer;
			std::vector<std::shared_ptr<IBuffer>> m_frameInFlightBuffers;
			std::vector<ItemInfo> m_bufferInfos;
			std::vector<uint8_t> m_initialSubBufferData;
			uint32_t m_alignment = 0;
			bool m_hasDirtyBuffers = false;
			Index m_nextIndex = 0;
			size_t m_sizePerSubBuffer = 0;
			size_t m_alignedSizePerSubBuffer = 0;
		};
	};
}
