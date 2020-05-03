/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_BUFFER_HPP__
#define __PROSPER_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_buffer_create_info.hpp"
#include <mathutil/umath.h>
#include <memory>
#include <cinttypes>
#include <functional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class IUniformResizableBuffer;
	class VkUniformResizableBuffer;
	class IDynamicResizableBuffer;
	class VkDynamicResizableBuffer;

	class DLLPROSPER IBuffer
		: public ContextObject,
		public std::enable_shared_from_this<IBuffer>
	{
	public:
		using SubBufferIndex = uint32_t;
		using Offset = vk::DeviceSize;
		using SmallOffset = uint32_t;
		using Size = vk::DeviceSize;
		static const auto INVALID_INDEX = std::numeric_limits<SubBufferIndex>::max();
		static const auto INVALID_OFFSET = std::numeric_limits<Offset>::max();
		static const auto INVALID_SMALL_OFFSET = std::numeric_limits<SmallOffset>::max();
		IBuffer(const IBuffer&)=delete;
		IBuffer &operator=(const IBuffer&)=delete;

		virtual ~IBuffer() override;

		Offset GetStartOffset() const;
		Size GetSize() const;
		std::shared_ptr<IBuffer> GetParent();
		const std::shared_ptr<IBuffer> GetParent() const;

		bool Write(Offset offset,Size size,const void *data) const;
		bool Read(Offset offset,Size size,void *data) const;

		template<typename T>
			bool Write(Offset offset,const T &t) const;
		template<typename T>
			bool Read(Offset offset,T &tOut) const;

		void SetPermanentlyMapped(bool b);
		bool Map(Offset offset,Size size) const;
		bool Unmap() const;

		virtual std::shared_ptr<IBuffer> CreateSubBuffer(DeviceSize offset,DeviceSize size,const std::function<void(IBuffer&)> &onDestroyedCallback=nullptr)=0;

		const util::BufferCreateInfo &GetCreateInfo() const;
		SubBufferIndex GetBaseIndex() const;
		BufferUsageFlags GetUsageFlags() const;
	protected:
		friend IUniformResizableBuffer;
		friend IDynamicResizableBuffer;
		virtual void Initialize();
		virtual void OnRelease() override;

		virtual bool DoWrite(Offset offset,Size size,const void *data) const=0;
		virtual bool DoRead(Offset offset,Size size,void *data) const=0;
		virtual bool DoMap(Offset offset,Size size) const=0;
		virtual bool DoUnmap() const=0;

		IBuffer(IPrContext &context,const util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size);
		bool Map(Offset offset,Size size,BufferUsageFlags deviceUsageFlags,BufferUsageFlags hostUsageFlags) const;
		void SetParent(IBuffer &parent,SubBufferIndex baseIndex=INVALID_INDEX);

		struct MappedBuffer
		{
			std::shared_ptr<IBuffer> buffer = nullptr;
			Offset offset = 0ull;
		};
		mutable std::unique_ptr<MappedBuffer> m_mappedTmpBuffer = nullptr;

		std::shared_ptr<IBuffer> m_parent = nullptr;
		bool m_bPermanentlyMapped = false;

		util::BufferCreateInfo m_createInfo {};
		DeviceSize m_startOffset = 0;
		DeviceSize m_size = 0;
	private:
		SubBufferIndex m_baseIndex = INVALID_INDEX;
	};
};
#pragma warning(pop)

template<typename T>
	bool prosper::IBuffer::Write(Offset offset,const T &t) const
{
	return Write(offset,sizeof(t),&t);
}
template<typename T>
	bool prosper::IBuffer::Read(Offset offset,T &tOut) const
{
	return Read(offset,sizeof(tOut),&tOut);
}

#endif
