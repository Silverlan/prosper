// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:buffer.dynamic_resizable_buffer;

export import :buffer.resizable_buffer;

export {
	namespace prosper {
		class IPrContext;
		class IBuffer;
		class DLLPROSPER IDynamicResizableBuffer : public IResizableBuffer {
		  public:
			std::shared_ptr<IBuffer> AllocateBuffer(DeviceSize size, const void *data = nullptr);
			std::shared_ptr<IBuffer> AllocateBuffer(DeviceSize size, uint32_t alignment, const void *data, bool reallocateIfNoSpaceAvailable = true);
			void DebugPrint(std::stringstream &strFilledData, std::stringstream &strFreeData, std::stringstream *bufferData = nullptr) const;
			const std::vector<IBuffer *> &GetAllocatedSubBuffers() const;
			uint64_t GetFreeSize() const;
			float GetFragmentationPercent() const;
			uint32_t GetAlignment() const { return m_alignment; }
		  protected:
			struct Range {
				DeviceSize startOffset;
				DeviceSize size;
			};
			IDynamicResizableBuffer(IPrContext &context, IBuffer &buffer, const util::BufferCreateInfo &createInfo, uint64_t maxTotalSize);
			void InsertFreeMemoryRange(std::list<Range>::iterator itWhere, DeviceSize startOffset, DeviceSize size);
			void MarkMemoryRangeAsFree(DeviceSize startOffset, DeviceSize size);
			std::list<Range>::iterator FindFreeRange(DeviceSize size, uint32_t alignment);
			std::vector<IBuffer *> m_allocatedSubBuffers;
			std::list<Range> m_freeRanges;
			mutable std::recursive_mutex m_bufferMutex;
			uint32_t m_alignment = 0u;
		};
	};
}
