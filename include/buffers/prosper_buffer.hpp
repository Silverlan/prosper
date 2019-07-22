/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_BUFFER_HPP__
#define __PROSPER_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <memory>
#include <cinttypes>
#include <functional>

namespace Anvil
{
	class Buffer;
};

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Buffer;
	namespace util
	{
		DLLPROSPER std::shared_ptr<Buffer> create_sub_buffer(Buffer &parentBuffer,vk::DeviceSize offset,vk::DeviceSize size,const std::function<void(Buffer&)> &onDestroyedCallback);
	};
	class UniformResizableBuffer;
	class DynamicResizableBuffer;
	class DLLPROSPER Buffer
		: public ContextObject,
		public std::enable_shared_from_this<Buffer>
	{
	public:
		using SubBufferIndex = uint32_t;
		using Offset = vk::DeviceSize;
		using SmallOffset = uint32_t;
		using Size = vk::DeviceSize;
		static const auto INVALID_INDEX = std::numeric_limits<SubBufferIndex>::max();
		static const auto INVALID_OFFSET = std::numeric_limits<Offset>::max();
		static const auto INVALID_SMALL_OFFSET = std::numeric_limits<SmallOffset>::max();

		static std::shared_ptr<Buffer> Create(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf,const std::function<void(Buffer&)> &onDestroyedCallback=nullptr);
		virtual ~Buffer() override;
		Anvil::Buffer &GetAnvilBuffer() const;
		Anvil::Buffer &GetBaseAnvilBuffer() const;
		Anvil::Buffer &operator*();
		const Anvil::Buffer &operator*() const;
		Anvil::Buffer *operator->();
		const Anvil::Buffer *operator->() const;

		Offset GetStartOffset() const;
		Size GetSize() const;
		std::shared_ptr<Buffer> GetParent();
		const std::shared_ptr<Buffer> GetParent() const;

		bool Write(Offset offset,Size size,const void *data) const;
		bool Read(Offset offset,Size size,void *data) const;

		template<typename T>
			bool Write(Offset offset,const T &t) const;
		template<typename T>
			bool Read(Offset offset,T &tOut) const;

		void SetPermanentlyMapped(bool b);
		bool Map(Offset offset,Size size) const;
		bool Unmap() const;

		SubBufferIndex GetBaseIndex() const;
		Anvil::BufferUsageFlags GetUsageFlags() const;
	protected:
		friend UniformResizableBuffer;
		friend DynamicResizableBuffer;
		virtual void Initialize();
		bool Map(Offset offset,Size size,Anvil::BufferUsageFlags deviceUsageFlags,Anvil::BufferUsageFlags hostUsageFlags) const;

		Buffer(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
		std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> m_buffer = nullptr;

		struct MappedBuffer
		{
			std::shared_ptr<Buffer> buffer = nullptr;
			Offset offset = 0ull;
		};
		mutable std::unique_ptr<MappedBuffer> m_mappedTmpBuffer = nullptr;

		std::weak_ptr<Buffer> m_parent = {};
		bool m_bPermanentlyMapped = false;
	private:
		friend std::shared_ptr<Buffer> prosper::util::create_sub_buffer(Buffer &parentBuffer,vk::DeviceSize offset,vk::DeviceSize size,const std::function<void(Buffer&)> &onDestroyedCallback);
		void SetParent(const std::shared_ptr<Buffer> &parent,SubBufferIndex baseIndex=INVALID_INDEX);
		void SetBuffer(std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
		SubBufferIndex m_baseIndex = INVALID_INDEX;
	};
};
#pragma warning(pop)

template<typename T>
	bool prosper::Buffer::Write(Offset offset,const T &t) const
{
	return Write(offset,sizeof(t),&t);
}
template<typename T>
	bool prosper::Buffer::Read(Offset offset,T &tOut) const
{
	return Read(offset,sizeof(tOut),&tOut);
}

#endif
