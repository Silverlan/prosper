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
	class UniformResizableBuffer;
	class DynamicResizableBuffer;
	class DLLPROSPER Buffer
		: public ContextObject,
		public std::enable_shared_from_this<Buffer>
	{
	public:
		static std::shared_ptr<Buffer> Create(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf,const std::function<void(Buffer&)> &onDestroyedCallback=nullptr);
		virtual ~Buffer() override;
		Anvil::Buffer &GetAnvilBuffer() const;
		Anvil::Buffer &GetBaseAnvilBuffer() const;
		Anvil::Buffer &operator*();
		const Anvil::Buffer &operator*() const;
		Anvil::Buffer *operator->();
		const Anvil::Buffer *operator->() const;

		vk::DeviceSize GetStartOffset() const;
		vk::DeviceSize GetSize() const;
		std::shared_ptr<Buffer> GetParent();
		const std::shared_ptr<Buffer> GetParent() const;

		bool Write(vk::DeviceSize offset,vk::DeviceSize size,const void *data) const;
		bool Read(vk::DeviceSize offset,vk::DeviceSize size,void *data) const;

		template<typename T>
			bool Write(vk::DeviceSize offset,const T &t) const;
		template<typename T>
			bool Read(vk::DeviceSize offset,T &tOut) const;

		void SetPermanentlyMapped(bool b);
		bool Map(vk::DeviceSize offset,vk::DeviceSize size) const;
		bool Unmap() const;

		uint32_t GetBaseIndex() const;
		Anvil::BufferUsageFlags GetUsageFlags() const;
	protected:
		friend UniformResizableBuffer;
		friend DynamicResizableBuffer;
		virtual void Initialize();
		bool Map(vk::DeviceSize offset,vk::DeviceSize size,Anvil::BufferUsageFlags deviceUsageFlags,Anvil::BufferUsageFlags hostUsageFlags) const;

		Buffer(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
		std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> m_buffer = nullptr;

		struct MappedBuffer
		{
			std::shared_ptr<Buffer> buffer = nullptr;
			vk::DeviceSize offset = 0ull;
		};
		mutable std::unique_ptr<MappedBuffer> m_mappedTmpBuffer = nullptr;

		std::weak_ptr<Buffer> m_parent = {};
		bool m_bPermanentlyMapped = false;
	private:
		void SetParent(const std::shared_ptr<Buffer> &parent,uint32_t baseIndex=std::numeric_limits<uint32_t>::max());
		void SetBuffer(std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
		uint32_t m_baseIndex = std::numeric_limits<uint32_t>::max();
	};
};
#pragma warning(pop)

template<typename T>
	bool prosper::Buffer::Write(vk::DeviceSize offset,const T &t) const
{
	return Write(offset,sizeof(t),&t);
}
template<typename T>
	bool prosper::Buffer::Read(vk::DeviceSize offset,T &tOut) const
{
	return Read(offset,sizeof(tOut),&tOut);
}

#endif
