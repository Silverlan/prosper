/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <wrappers/buffer.h>
#include <wrappers/memory_block.h>
#include <misc/buffer_create_info.h>

prosper::VlkBuffer::VlkBuffer(Context &context,const util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size,Anvil::BufferUniquePtr buf)
	: IBuffer{context,bufCreateInfo,startOffset,size},m_buffer{std::move(buf)}
{
	if(m_buffer != nullptr)
		prosper::debug::register_debug_object(m_buffer->get_buffer(),this,prosper::debug::ObjectType::Buffer);
}
prosper::VlkBuffer::~VlkBuffer()
{
	if(m_buffer != nullptr)
		prosper::debug::deregister_debug_object(m_buffer->get_buffer());
}
std::shared_ptr<prosper::VlkBuffer> prosper::VlkBuffer::Create(Context &context,Anvil::BufferUniquePtr buf,const util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size,const std::function<void(IBuffer&)> &onDestroyedCallback)
{
	if(buf == nullptr)
		return nullptr;
	auto r = std::shared_ptr<VlkBuffer>(new VlkBuffer(context,bufCreateInfo,startOffset,size,std::move(buf)),[onDestroyedCallback](VlkBuffer *buf) {
		buf->OnRelease();
		if(onDestroyedCallback != nullptr)
			onDestroyedCallback(*buf);
		buf->GetContext().ReleaseResource<VlkBuffer>(buf);
		});
	r->Initialize();
	return r;
}

std::shared_ptr<prosper::VlkBuffer> prosper::VlkBuffer::GetParent() {return std::dynamic_pointer_cast<VlkBuffer>(IBuffer::GetParent());}
const std::shared_ptr<prosper::VlkBuffer> prosper::VlkBuffer::GetParent() const {return const_cast<VlkBuffer*>(this)->GetParent();}

std::shared_ptr<prosper::IBuffer> prosper::VlkBuffer::CreateSubBuffer(DeviceSize offset,DeviceSize size,const std::function<void(IBuffer&)> &onDestroyedCallback)
{
	auto buf = Anvil::Buffer::create(Anvil::BufferCreateInfo::create_no_alloc_child(&GetAnvilBuffer(),offset,size));
	auto subBuffer = Create(GetContext(),std::move(buf),m_createInfo,(m_parent ? m_parent->GetStartOffset() : 0ull) +offset,size,onDestroyedCallback);
	subBuffer->SetParent(*this);
	return subBuffer;
}

Anvil::Buffer &prosper::VlkBuffer::GetAnvilBuffer() const {return *m_buffer;}
Anvil::Buffer &prosper::VlkBuffer::GetBaseAnvilBuffer() const {return m_parent ? GetParent()->GetAnvilBuffer() : GetAnvilBuffer();}
Anvil::Buffer &prosper::VlkBuffer::operator*() {return *m_buffer;}
const Anvil::Buffer &prosper::VlkBuffer::operator*() const {return const_cast<VlkBuffer*>(this)->operator*();}
Anvil::Buffer *prosper::VlkBuffer::operator->() {return m_buffer.get();}
const Anvil::Buffer *prosper::VlkBuffer::operator->() const {return const_cast<VlkBuffer*>(this)->operator->();}

bool prosper::VlkBuffer::DoWrite(Offset offset,Size size,const void *data) const {return m_buffer->get_memory_block(0u)->write(offset,size,data);}
bool prosper::VlkBuffer::DoRead(Offset offset,Size size,void *data) const {return m_buffer->get_memory_block(0u)->read(offset,size,data);}
bool prosper::VlkBuffer::DoMap(Offset offset,Size size) const {return m_buffer->get_memory_block(0u)->map(offset,size);}
bool prosper::VlkBuffer::DoUnmap() const {return m_buffer->get_memory_block(0u)->unmap();}

void prosper::VlkBuffer::SetBuffer(Anvil::BufferUniquePtr buf)
{
	auto bPermanentlyMapped = m_bPermanentlyMapped;
	SetPermanentlyMapped(false);

	if(m_buffer != nullptr)
		prosper::debug::deregister_debug_object(m_buffer->get_buffer());
	m_buffer = std::move(buf);
	if(m_buffer != nullptr)
		prosper::debug::register_debug_object(m_buffer->get_buffer(),this,prosper::debug::ObjectType::Buffer);

	if(bPermanentlyMapped == true)
		SetPermanentlyMapped(true);
}
