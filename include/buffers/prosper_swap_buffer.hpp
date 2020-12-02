/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SWAP_BUFFER_HPP__
#define __PROSPER_SWAP_BUFFER_HPP__

#include "prosper_buffer.hpp"

namespace prosper
{
	class IUniformResizableBuffer;
	class IDynamicResizableBuffer;
	class DLLPROSPER SwapBuffer
		: public std::enable_shared_from_this<SwapBuffer>
	{
	public:
		using SubBufferIndex = uint32_t;
		static std::shared_ptr<SwapBuffer> Create(IUniformResizableBuffer &buffer,const void *data=nullptr);
		static std::shared_ptr<SwapBuffer> Create(IDynamicResizableBuffer &buffer,DeviceSize size,uint32_t instanceCount);
		IBuffer &GetBuffer(SubBufferIndex idx);
		const IBuffer &GetBuffer(SubBufferIndex idx) const {return const_cast<SwapBuffer*>(this)->GetBuffer(idx);}
		IBuffer &GetBuffer() const;
		bool IsDirty() const;
		
		IBuffer &Update(IBuffer::Offset offset,IBuffer::Size size,const void *data,bool flagAsDirty=true);

		IBuffer *operator->();
		const IBuffer *operator->() const {return const_cast<SwapBuffer*>(this)->operator->();}
		IBuffer &operator*();
		const IBuffer &operator*() const {return const_cast<SwapBuffer*>(this)->operator*();}
	private:
		SwapBuffer(std::vector<std::shared_ptr<IBuffer>> &&buffers);
		uint32_t GetCurrentBufferIndex() const;
		uint32_t GetCurrentBufferFlag() const;
		std::vector<std::shared_ptr<IBuffer>> m_buffers;
		uint32_t m_currentBufferIndex = 0;
		mutable uint8_t m_buffersDirty = 0;
	};
};

#endif
