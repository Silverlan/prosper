/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_BUFFER_HPP__
#define __PR_PROSPER_VK_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "buffers/prosper_buffer.hpp"
#include "buffers/prosper_buffer_create_info.hpp"
#include <wrappers/buffer.h>
#include <mathutil/umath.h>
#include <memory>
#include <cinttypes>
#include <functional>

namespace Anvil
{
	class Buffer;
};

namespace prosper
{
	class VkUniformResizableBuffer;
	class VkDynamicResizableBuffer;
	class DLLPROSPER VlkBuffer
		: virtual public IBuffer
	{
	public:
		static std::shared_ptr<VlkBuffer> Create(Context &context,Anvil::BufferUniquePtr buf,const util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size,const std::function<void(IBuffer&)> &onDestroyedCallback=nullptr);

		virtual ~VlkBuffer() override;

		std::shared_ptr<VlkBuffer> GetParent();
		const std::shared_ptr<VlkBuffer> GetParent() const;
		virtual std::shared_ptr<IBuffer> CreateSubBuffer(DeviceSize offset,DeviceSize size,const std::function<void(IBuffer&)> &onDestroyedCallback=nullptr) override;

		Anvil::Buffer &GetAnvilBuffer() const;
		Anvil::Buffer &GetBaseAnvilBuffer() const;
		Anvil::Buffer &operator*();
		const Anvil::Buffer &operator*() const;
		Anvil::Buffer *operator->();
		const Anvil::Buffer *operator->() const;
	protected:
		friend IDynamicResizableBuffer;
		friend VkDynamicResizableBuffer;
		friend IUniformResizableBuffer;
		friend VkUniformResizableBuffer;
		VlkBuffer(Context &context,const util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
		virtual bool DoWrite(Offset offset,Size size,const void *data) const override;
		virtual bool DoRead(Offset offset,Size size,void *data) const override;
		virtual bool DoMap(Offset offset,Size size) const override;
		virtual bool DoUnmap() const override;

		std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> m_buffer = nullptr;
	private:
		void SetBuffer(std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf);
	};
};

#endif
