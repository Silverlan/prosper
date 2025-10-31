// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;



module pragma.prosper;

import :buffer.buffer;

prosper::IBuffer::IBuffer(IPrContext &context, const prosper::util::BufferCreateInfo &bufCreateInfo, DeviceSize startOffset, DeviceSize size) : ContextObject(context), std::enable_shared_from_this<IBuffer>(), m_createInfo {bufCreateInfo}, m_startOffset {startOffset}, m_size {size} {}

prosper::IBuffer::~IBuffer() {}

void prosper::IBuffer::OnRelease()
{
	if(m_permanentlyMapped.has_value())
		Unmap();
}

void prosper::IBuffer::SetPermanentlyMapped(bool b, MapFlags mapFlags)
{
	if(m_permanentlyMapped.has_value() == b) {
		if(b == false || mapFlags == *m_permanentlyMapped)
			return;
		Unmap();
	}
	if(b == true) {
		mapFlags |= prosper::IBuffer::MapFlags::PersistentBit;
		Map(0ull, GetSize(), mapFlags);
		m_permanentlyMapped = mapFlags;
		return;
	}
	Unmap();
	m_permanentlyMapped = {};
}
void prosper::IBuffer::SetParent(IBuffer &parent, SubBufferIndex baseIndex)
{
	m_parent = parent.shared_from_this();
	m_baseIndex = baseIndex;
}

prosper::IBuffer::Offset prosper::IBuffer::GetStartOffset() const { return m_startOffset; }
prosper::IBuffer::Size prosper::IBuffer::GetSize() const { return m_size; }
std::shared_ptr<prosper::IBuffer> prosper::IBuffer::GetParent() { return m_parent; }
const std::shared_ptr<prosper::IBuffer> prosper::IBuffer::GetParent() const { return const_cast<IBuffer *>(this)->GetParent(); }

bool prosper::IBuffer::Map(Offset offset, Size size, BufferUsageFlags deviceUsageFlags, BufferUsageFlags hostUsageFlags, MapFlags mapFlags, void **optOutMappedPtr) const
{
	if(umath::is_flag_set(m_createInfo.memoryFeatures, MemoryFeatureFlags::HostAccessable) == false) {
		if((GetUsageFlags() & deviceUsageFlags) == BufferUsageFlags::None) {
			if(GetContext().IsValidationEnabled() == true)
				std::cout << "WARNING: Attempted to map unmappable buffer without usage flags required for copy commands! Skipping..." << std::endl;
			return false;
		}
		if(GetContext().IsValidationEnabled() == true)
			; //std::cout<<"WARNING: Attempted to map unmappable buffer! While still possible, this is highly discouraged!"<<std::endl;
		auto &context = const_cast<IPrContext &>(GetContext());
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.memoryFeatures = MemoryFeatureFlags::HostAccessable | MemoryFeatureFlags::HostCached;
		createInfo.size = size;
		createInfo.usageFlags = hostUsageFlags;
		auto buf = context.CreateBuffer(createInfo);
		if(buf == nullptr)
			return false;
		m_mappedTmpBuffer = std::unique_ptr<MappedBuffer>(new MappedBuffer());
		m_mappedTmpBuffer->buffer = buf;
		m_mappedTmpBuffer->offset = GetStartOffset() + offset;
		// TODO: Assign optOutMappedPtr?
		return true;
	}
	return DoMap(offset, size, mapFlags, optOutMappedPtr);
}

bool prosper::IBuffer::Write(Offset offset, Size size, const void *data) const
{
	if(offset + size > GetSize())
		throw std::out_of_range {"Memory write range (offset: " + std::to_string(offset) + ", size: " + std::to_string(size) + ") out of bounds of buffer of size " + std::to_string(GetSize()) + "!"};
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Write(GetStartOffset() + offset, size, data);
	if(m_mappedTmpBuffer != nullptr) {
		auto &context = const_cast<IPrContext &>(GetContext());
		auto &buf = m_mappedTmpBuffer->buffer;
		if(buf->Write(0ull, size, data) == false)
			return false;
		util::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.srcOffset = 0ull;
		copyInfo.dstOffset = offset;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = setupCmd->RecordCopyBuffer(copyInfo, *buf, *const_cast<IBuffer *>(this));
		context.FlushSetupCommandBuffer();
		return r;
	}
	if(umath::is_flag_set(m_createInfo.memoryFeatures, MemoryFeatureFlags::HostAccessable) == false) {
		if(Map(offset, size, BufferUsageFlags::TransferDstBit, BufferUsageFlags::TransferSrcBit, prosper::IBuffer::MapFlags::WriteBit, nullptr) == false)
			return false;
		Write(offset, size, data);
		return Unmap();
	}
	return DoWrite(offset, size, data);
}
bool prosper::IBuffer::Read(Offset offset, Size size, void *data) const
{
	if(offset + size > GetSize())
		throw std::out_of_range {"Memory read range (offset: " + std::to_string(offset) + ", size: " + std::to_string(size) + ") out of bounds of buffer of size " + std::to_string(GetSize()) + "!"};
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Read(GetStartOffset() + offset, size, data);
	if(m_mappedTmpBuffer != nullptr) {
		auto &context = const_cast<IPrContext &>(GetContext());
		auto &buf = m_mappedTmpBuffer->buffer;
		util::BufferCopy copyInfo {};
		copyInfo.size = size;
		copyInfo.srcOffset = offset;
		copyInfo.dstOffset = 0ull;
		auto &setupCmd = context.GetSetupCommandBuffer();
		auto r = setupCmd->RecordCopyBuffer(copyInfo, *const_cast<IBuffer *>(this), *buf);
		context.FlushSetupCommandBuffer();
		if(r == false)
			return false;
		return buf->Read(0ull, size, data);
	}
	if(umath::is_flag_set(m_createInfo.memoryFeatures, MemoryFeatureFlags::HostAccessable) == false) {
		if(Map(offset, size, BufferUsageFlags::TransferSrcBit, BufferUsageFlags::TransferDstBit, prosper::IBuffer::MapFlags::ReadBit, nullptr) == false)
			return false;
		Read(offset, size, data);
		return Unmap();
	}
	return DoRead(offset, size, data);
}
void prosper::IBuffer::Initialize() {}
bool prosper::IBuffer::Map(Offset offset, Size size, MapFlags mapFlags, void **optOutMappedPtr) const
{
	if(offset + size > GetSize())
		throw std::out_of_range {"Memory map range (offset: " + std::to_string(offset) + ", size: " + std::to_string(size) + ") out of bounds of buffer of size " + std::to_string(GetSize()) + "!"};
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Map(GetStartOffset() + offset, size, mapFlags, optOutMappedPtr);
	const auto usageFlags = BufferUsageFlags::TransferSrcBit | BufferUsageFlags::TransferDstBit;
	return Map(offset, size, usageFlags, usageFlags, mapFlags, optOutMappedPtr);
}
bool prosper::IBuffer::Unmap() const
{
	auto parent = GetParent();
	if(parent != nullptr)
		return parent->Unmap();
	if(m_mappedTmpBuffer != nullptr) {
		m_mappedTmpBuffer = nullptr;
		return true;
	}
	return DoUnmap();
}
const prosper::util::BufferCreateInfo &prosper::IBuffer::GetCreateInfo() const { return m_createInfo; }
prosper::IBuffer::SubBufferIndex prosper::IBuffer::GetBaseIndex() const { return m_baseIndex; }
prosper::BufferUsageFlags prosper::IBuffer::GetUsageFlags() const { return m_createInfo.usageFlags; }

prosper::IBuffer *prosper::IBuffer::GetMappedBuffer(Offset &outOffset)
{
	if(m_mappedTmpBuffer == nullptr)
		return nullptr;
	outOffset = m_mappedTmpBuffer->offset;
	return m_mappedTmpBuffer->buffer.get();
}

CallbackHandle prosper::IBuffer::AddReallocationCallback(const std::function<void()> &fCallback)
{
	auto cb = FunctionCallback<void>::Create(fCallback);
	m_reallocationCallbacks.push_back(cb);
	return cb;
}
