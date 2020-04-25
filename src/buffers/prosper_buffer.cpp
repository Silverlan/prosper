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
#include <iostream>

prosper::IBuffer::IBuffer(Context &context,const prosper::util::BufferCreateInfo &bufCreateInfo,DeviceSize startOffset,DeviceSize size)
	: ContextObject(context),std::enable_shared_from_this<IBuffer>(),m_createInfo{bufCreateInfo},m_startOffset{startOffset},
	m_size{size}
{}

prosper::IBuffer::~IBuffer()
{
	MemoryTracker::GetInstance().RemoveResource(*this);
}

void prosper::IBuffer::OnRelease()
{
	if(m_bPermanentlyMapped == true)
		Unmap();
}

void prosper::IBuffer::SetPermanentlyMapped(bool b)
{
	if(m_bPermanentlyMapped == b)
		return;
	m_bPermanentlyMapped = b;
	if(b == true)
		Map(0ull,GetSize());
	else
		Unmap();
}
void prosper::IBuffer::SetParent(IBuffer &parent,SubBufferIndex baseIndex)
{
	m_parent = parent.shared_from_this();
	m_baseIndex = baseIndex;
}

prosper::IBuffer::Offset prosper::IBuffer::GetStartOffset() const {return m_startOffset;}
prosper::IBuffer::Size prosper::IBuffer::GetSize() const {return m_size;}
std::shared_ptr<prosper::IBuffer> prosper::IBuffer::GetParent() {return m_parent;}
const std::shared_ptr<prosper::IBuffer> prosper::IBuffer::GetParent() const {return const_cast<IBuffer*>(this)->GetParent();}

bool prosper::IBuffer::Map(Offset offset,Size size,BufferUsageFlags deviceUsageFlags,BufferUsageFlags hostUsageFlags) const
{
	if(umath::is_flag_set(m_createInfo.memoryFeatures,MemoryFeatureFlags::HostAccessable) == false)
	{
		if((GetUsageFlags() &deviceUsageFlags) == BufferUsageFlags::None)
		{
			if(GetContext().IsValidationEnabled() == true)
				std::cout<<"WARNING: Attempted to map unmappable buffer without usage flags required for copy commands! Skipping..."<<std::endl;
			return false;
		}
		if(GetContext().IsValidationEnabled() == true)
			;//std::cout<<"WARNING: Attempted to map unmappable buffer! While still possible, this is highly discouraged!"<<std::endl;
		auto &context = const_cast<Context&>(GetContext());
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = MemoryFeatureFlags::HostAccessable | MemoryFeatureFlags::HostCached;
		createInfo.size = size;
		createInfo.usageFlags = hostUsageFlags;
		auto buf = context.CreateBuffer(createInfo);
		if(buf == nullptr)
			return false;
		m_mappedTmpBuffer = std::unique_ptr<MappedBuffer>(new MappedBuffer());
		m_mappedTmpBuffer->buffer = buf;
		m_mappedTmpBuffer->offset = GetStartOffset() +offset;
		return true;
	}
	return DoMap(offset,size);
}

bool prosper::IBuffer::Write(Offset offset,Size size,const void *data) const
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
		util::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.srcOffset = 0ull;
		copyInfo.dstOffset = offset;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = setupCmd->RecordCopyBuffer(copyInfo,*buf,*const_cast<IBuffer*>(this));
		context.FlushSetupCommandBuffer();
		return r;
	}
	if(umath::is_flag_set(m_createInfo.memoryFeatures,MemoryFeatureFlags::HostAccessable) == false)
	{
		if(Map(offset,size,BufferUsageFlags::TransferDstBit,BufferUsageFlags::TransferSrcBit) == false)
			return false;
		Write(offset,size,data);
		return Unmap();
	}
	return DoWrite(offset,size,data);
}
bool prosper::IBuffer::Read(Offset offset,Size size,void *data) const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Read(GetStartOffset() +offset,size,data);
	if(m_mappedTmpBuffer != nullptr)
	{
		auto &context = const_cast<Context&>(GetContext());
		auto &buf = m_mappedTmpBuffer->buffer;
		util::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.srcOffset = offset;
		copyInfo.dstOffset = 0ull;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = setupCmd->RecordCopyBuffer(copyInfo,*const_cast<IBuffer*>(this),*buf);
		context.FlushSetupCommandBuffer();
		if(r == false)
			return false;
		return buf->Read(0ull,size,data);
	}
	if(umath::is_flag_set(m_createInfo.memoryFeatures,MemoryFeatureFlags::HostAccessable) == false)
	{
		if(Map(offset,size,BufferUsageFlags::TransferSrcBit,BufferUsageFlags::TransferDstBit) == false)
			return false;
		Read(offset,size,data);
		return Unmap();
	}
	return DoRead(offset,size,data);
}
void prosper::IBuffer::Initialize() {MemoryTracker::GetInstance().AddResource(*this);}
bool prosper::IBuffer::Map(Offset offset,Size size) const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Map(GetStartOffset() +offset,size);
	const auto usageFlags = BufferUsageFlags::TransferSrcBit | BufferUsageFlags::TransferDstBit;
	return Map(offset,size,usageFlags,usageFlags);
}
bool prosper::IBuffer::Unmap() const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Unmap();
	if(m_mappedTmpBuffer != nullptr)
	{
		m_mappedTmpBuffer = nullptr;
		return true;
	}
	return DoUnmap();
}
const prosper::util::BufferCreateInfo &prosper::IBuffer::GetCreateInfo() const {return m_createInfo;}
prosper::IBuffer::SubBufferIndex prosper::IBuffer::GetBaseIndex() const {return m_baseIndex;}
prosper::BufferUsageFlags prosper::IBuffer::GetUsageFlags() const {return m_createInfo.usageFlags;}
