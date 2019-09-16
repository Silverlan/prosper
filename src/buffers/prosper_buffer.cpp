/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "buffers/prosper_buffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_memory_tracker.hpp"
#include <wrappers/buffer.h>
#include <wrappers/memory_block.h>
#include <misc/buffer_create_info.h>
#include <iostream>

using namespace prosper;

#pragma optimize("",off)
std::shared_ptr<Buffer> Buffer::Create(Context &context,Anvil::BufferUniquePtr buf,const std::function<void(Buffer&)> &onDestroyedCallback)
{
	if(buf == nullptr)
		return nullptr;
	auto r = std::shared_ptr<Buffer>(new Buffer(context,std::move(buf)),[onDestroyedCallback](Buffer *buf) {
		if(onDestroyedCallback != nullptr)
			onDestroyedCallback(*buf);
		buf->GetContext().ReleaseResource<Buffer>(buf);
	});
	r->Initialize();
	return r;
}

Buffer::Buffer(Context &context,Anvil::BufferUniquePtr buf)
	: ContextObject(context),std::enable_shared_from_this<Buffer>(),m_buffer(std::move(buf))
{
	if(m_buffer != nullptr)
		prosper::debug::register_debug_object(m_buffer->get_buffer(),this,prosper::debug::ObjectType::Buffer);
}

Buffer::~Buffer()
{
	if(m_buffer != nullptr)
		prosper::debug::deregister_debug_object(m_buffer->get_buffer());
	MemoryTracker::GetInstance().RemoveResource(*this);

	if(m_bPermanentlyMapped == true)
		Unmap();
}

void Buffer::SetPermanentlyMapped(bool b)
{
	if(m_bPermanentlyMapped == b)
		return;
	m_bPermanentlyMapped = b;
	if(b == true)
		Map(0ull,GetSize());
	else
		Unmap();
}
void Buffer::SetParent(const std::shared_ptr<Buffer> &parent,SubBufferIndex baseIndex)
{
	m_parent = parent;
	m_baseIndex = baseIndex;
}
void Buffer::SetBuffer(Anvil::BufferUniquePtr buf)
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
Anvil::Buffer &Buffer::GetAnvilBuffer() const {return *m_buffer;}
Anvil::Buffer &Buffer::GetBaseAnvilBuffer() const {return (m_parent.expired() == false) ? m_parent.lock()->GetAnvilBuffer() : GetAnvilBuffer();}
Anvil::Buffer &Buffer::operator*() {return *m_buffer;}
const Anvil::Buffer &Buffer::operator*() const {return const_cast<Buffer*>(this)->operator*();}
Anvil::Buffer *Buffer::operator->() {return m_buffer.get();}
const Anvil::Buffer *Buffer::operator->() const {return const_cast<Buffer*>(this)->operator->();}

Buffer::Offset Buffer::GetStartOffset() const {return (*this)->get_create_info_ptr()->get_start_offset();}
Buffer::Size Buffer::GetSize() const {return m_buffer->get_create_info_ptr()->get_size();}
std::shared_ptr<Buffer> Buffer::GetParent() {return m_parent.lock();}
const std::shared_ptr<Buffer> Buffer::GetParent() const {return const_cast<Buffer*>(this)->GetParent();}

bool Buffer::Write(Offset offset,Size size,const void *data) const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Write(GetStartOffset() +offset,size,data);
	if(m_mappedTmpBuffer != nullptr)
	{
		auto &context = const_cast<Context&>(GetContext());
		auto &buf = m_mappedTmpBuffer->buffer;
		if(buf->Write(0ull,size,data) == false)
			return false;
		Anvil::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.src_offset = 0ull;
		copyInfo.dst_offset = offset;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = prosper::util::record_copy_buffer(setupCmd->GetAnvilCommandBuffer(),copyInfo,*buf,*const_cast<Buffer*>(this));
		context.FlushSetupCommandBuffer();
		return r;
	}
	if((m_buffer->get_memory_block(0u)->get_create_info_ptr()->get_memory_features() &Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
	{
		if(Map(offset,size,Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT,Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT) == false)
			return false;
		Write(offset,size,data);
		return Unmap();
	}
	return m_buffer->get_memory_block(0u)->write(offset,size,data);
}
bool Buffer::Read(Offset offset,Size size,void *data) const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Read(GetStartOffset() +offset,size,data);
	if(m_mappedTmpBuffer != nullptr)
	{
		auto &context = const_cast<Context&>(GetContext());
		auto &buf = m_mappedTmpBuffer->buffer;
		Anvil::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.src_offset = offset;
		copyInfo.dst_offset = 0ull;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = prosper::util::record_copy_buffer(setupCmd->GetAnvilCommandBuffer(),copyInfo,*const_cast<Buffer*>(this),*buf);
		context.FlushSetupCommandBuffer();
		if(r == false)
			return false;
		return buf->Read(0ull,size,data);
	}
	if((m_buffer->get_create_info_ptr()->get_memory_features() &Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
	{
		if(Map(offset,size,Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT,Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT) == false)
			return false;
		Read(offset,size,data);
		return Unmap();
	}
	return m_buffer->get_memory_block(0u)->read(offset,size,data);
}
void Buffer::Initialize() {MemoryTracker::GetInstance().AddResource(*this);}
bool Buffer::Map(Offset offset,Size size,Anvil::BufferUsageFlags deviceUsageFlags,Anvil::BufferUsageFlags hostUsageFlags) const
{
	if((m_buffer->get_memory_block(0u)->get_create_info_ptr()->get_memory_features() &Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT) == 0)
	{
		if((GetUsageFlags() &deviceUsageFlags) == Anvil::BufferUsageFlagBits::NONE)
		{
			if(GetContext().IsValidationEnabled() == true)
				std::cout<<"WARNING: Attempted to map unmappable buffer without usage flags required for copy commands! Skipping..."<<std::endl;
			return false;
		}
		if(GetContext().IsValidationEnabled() == true)
			;//std::cout<<"WARNING: Attempted to map unmappable buffer! While still possible, this is highly discouraged!"<<std::endl;
		auto &context = const_cast<Context&>(GetContext());
		auto &dev = context.GetDevice();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable | prosper::util::MemoryFeatureFlags::HostCached;
		createInfo.size = size;
		createInfo.usageFlags = hostUsageFlags;
		auto buf = prosper::util::create_buffer(dev,createInfo);
		if(buf == nullptr)
			return false;
		m_mappedTmpBuffer = std::unique_ptr<MappedBuffer>(new MappedBuffer());
		m_mappedTmpBuffer->buffer = buf;
		m_mappedTmpBuffer->offset = GetStartOffset() +offset;
		return true;
	}
	return m_buffer->get_memory_block(0u)->map(offset,size);
}
bool Buffer::Map(Offset offset,Size size) const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Map(GetStartOffset() +offset,size);
	const auto usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
	return Map(offset,size,usageFlags,usageFlags);
}
bool Buffer::Unmap() const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Unmap();
	if(m_mappedTmpBuffer != nullptr)
	{
		m_mappedTmpBuffer = nullptr;
		return true;
	}
	return m_buffer->get_memory_block(0u)->unmap();
}
Buffer::SubBufferIndex Buffer::GetBaseIndex() const {return m_baseIndex;}
Anvil::BufferUsageFlags Buffer::GetUsageFlags() const {return m_buffer->get_create_info_ptr()->get_usage_flags();}
#pragma optimize("",on)
