// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"
#include <memory>
#include <cinttypes>
#include <functional>
#include <optional>

export module pragma.prosper:buffer.buffer;

export import :buffer.buffer_create_info;
export import :context_object;
export import :structs;
import pragma.util;

#undef max

export {
	#pragma warning(push)
	#pragma warning(disable : 4251)
	namespace prosper {
		class IUniformResizableBuffer;
		class IDynamicResizableBuffer;

		class DLLPROSPER IBuffer : public ContextObject, public std::enable_shared_from_this<IBuffer> {
		public:
			enum class MapFlags : uint8_t { None = 0u, PersistentBit = 1u, ReadBit = PersistentBit << 1u, WriteBit = ReadBit << 1u, Unsynchronized = WriteBit << 1u };

			using SubBufferIndex = uint32_t;
			using Offset = DeviceSize;
			using SmallOffset = uint32_t;
			using Size = DeviceSize;
			static const auto INVALID_INDEX = std::numeric_limits<SubBufferIndex>::max();
			static const auto INVALID_OFFSET = std::numeric_limits<Offset>::max();
			static const auto INVALID_SMALL_OFFSET = std::numeric_limits<SmallOffset>::max();
			IBuffer(const IBuffer &) = delete;
			IBuffer &operator=(const IBuffer &) = delete;

			virtual ~IBuffer() override;

			Offset GetStartOffset() const;
			Size GetSize() const;
			std::shared_ptr<IBuffer> GetParent();
			const std::shared_ptr<IBuffer> GetParent() const;

			bool Write(Offset offset, Size size, const void *data) const;
			bool Read(Offset offset, Size size, void *data) const;

			template<typename T>
			bool Write(Offset offset, const T &t) const;
			template<typename T>
			bool Read(Offset offset, T &tOut) const;

			void SetPermanentlyMapped(bool b, MapFlags mapFlags);
			bool Map(Offset offset, Size size, MapFlags mapFlags, void **optOutMappedPtr = nullptr) const;
			bool Unmap() const;

			virtual std::shared_ptr<IBuffer> CreateSubBuffer(DeviceSize offset, DeviceSize size, const std::function<void(IBuffer &)> &onDestroyedCallback = nullptr) = 0;

			const util::BufferCreateInfo &GetCreateInfo() const;
			SubBufferIndex GetBaseIndex() const;
			BufferUsageFlags GetUsageFlags() const;
			CallbackHandle AddReallocationCallback(const std::function<void()> &fCallback);
			virtual const void *GetInternalHandle() const { return nullptr; }

			virtual void Bake() {};

			// For internal use only!
			virtual void Initialize();

			template<class T, typename = std::enable_if_t<std::is_base_of_v<IBuffer, T>>>
			T &GetAPITypeRef()
			{
				return *static_cast<T *>(m_apiTypePtr);
			}
			template<class T, typename = std::enable_if_t<std::is_base_of_v<IBuffer, T>>>
			const T &GetAPITypeRef() const
			{
				return const_cast<IBuffer *>(this)->GetAPITypeRef<T>();
			}

			// For debugging purposes only!
			IBuffer *GetMappedBuffer(Offset &outOffset);
		protected:
			friend IUniformResizableBuffer;
			friend IDynamicResizableBuffer;
			virtual void RecreateInternalSubBuffer(IBuffer &newParentBuffer) {}
			virtual void OnRelease() override;

			virtual bool DoWrite(Offset offset, Size size, const void *data) const = 0;
			virtual bool DoRead(Offset offset, Size size, void *data) const = 0;
			virtual bool DoMap(Offset offset, Size size, MapFlags mapFlags, void **optOutMappedPtr) const = 0;
			virtual bool DoUnmap() const = 0;

			IBuffer(IPrContext &context, const util::BufferCreateInfo &bufCreateInfo, DeviceSize startOffset, DeviceSize size);
			bool Map(Offset offset, Size size, BufferUsageFlags deviceUsageFlags, BufferUsageFlags hostUsageFlags, MapFlags mapFlags, void **optOutMappedPtr) const;
			void SetParent(IBuffer &parent, SubBufferIndex baseIndex = INVALID_INDEX);

			struct MappedBuffer {
				std::shared_ptr<IBuffer> buffer = nullptr;
				Offset offset = 0ull;
			};
			mutable std::unique_ptr<MappedBuffer> m_mappedTmpBuffer = nullptr;

			std::shared_ptr<IBuffer> m_parent = nullptr;
			std::optional<MapFlags> m_permanentlyMapped {};
			std::vector<CallbackHandle> m_reallocationCallbacks {};

			util::BufferCreateInfo m_createInfo {};
			DeviceSize m_startOffset = 0;
			DeviceSize m_size = 0;

			void *m_apiTypePtr = nullptr;
		private:
			SubBufferIndex m_baseIndex = INVALID_INDEX;
		};

		template<typename T>
		bool IBuffer::Write(Offset offset, const T &t) const
		{
			return Write(offset, sizeof(t), &t);
		}
		template<typename T>
		bool IBuffer::Read(Offset offset, T &tOut) const
		{
			return Read(offset, sizeof(tOut), &tOut);
		}
		using namespace umath::scoped_enum::bitwise;
	};

	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<prosper::IBuffer::MapFlags> : std::true_type {};
	}
	#pragma warning(pop)
}
