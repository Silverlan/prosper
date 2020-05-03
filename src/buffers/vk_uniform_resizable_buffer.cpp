/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_uniform_resizable_buffer.hpp"
#include "prosper_util.hpp"
#include "buffers/prosper_buffer.hpp"
#include "vk_context.hpp"
#include <wrappers/device.h>

using namespace prosper;

prosper::VkUniformResizableBuffer::VkUniformResizableBuffer(
	IPrContext &context,IBuffer &buffer,
	uint64_t bufferInstanceSize,
	uint64_t alignedBufferBaseSize,uint64_t maxTotalSize,uint32_t alignment
)
	: IUniformResizableBuffer{context,buffer,bufferInstanceSize,alignedBufferBaseSize,maxTotalSize,alignment},
	IBuffer{buffer.GetContext(),buffer.GetCreateInfo(),buffer.GetStartOffset(),buffer.GetSize()},
	VlkBuffer{buffer.GetContext(),buffer.GetCreateInfo(),buffer.GetStartOffset(),buffer.GetSize(),nullptr}
{
	VlkBuffer::m_buffer = std::move(dynamic_cast<VlkBuffer&>(buffer).m_buffer);
}

static void calc_aligned_sizes(Anvil::BaseDevice &dev,uint64_t instanceSize,uint64_t &bufferBaseSize,uint64_t &maxTotalSize,uint32_t &alignment,prosper::BufferUsageFlags usageFlags)
{
	alignment = prosper::util::calculate_buffer_alignment(dev,usageFlags);
	auto alignedInstanceSize = prosper::util::get_aligned_size(instanceSize,alignment);
	auto alignedBufferBaseSize = static_cast<uint64_t>(umath::floor(bufferBaseSize /static_cast<long double>(alignedInstanceSize))) *alignedInstanceSize;
	auto alignedMaxTotalSize = static_cast<uint64_t>(umath::floor(maxTotalSize /static_cast<long double>(alignedInstanceSize))) *alignedInstanceSize;

	bufferBaseSize = alignedBufferBaseSize;
	maxTotalSize = alignedMaxTotalSize;
}

std::shared_ptr<VkUniformResizableBuffer> prosper::util::create_uniform_resizable_buffer(
	IPrContext &context,prosper::util::BufferCreateInfo createInfo,uint64_t bufferInstanceSize,
	uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data
)
{
	createInfo.size = prosper::util::clamp_gpu_memory_size(static_cast<VlkContext&>(context).GetDevice(),createInfo.size,clampSizeToAvailableGPUMemoryPercentage,createInfo.memoryFeatures);
	maxTotalSize = prosper::util::clamp_gpu_memory_size(static_cast<VlkContext&>(context).GetDevice(),maxTotalSize,clampSizeToAvailableGPUMemoryPercentage,createInfo.memoryFeatures);

	auto bufferBaseSize = createInfo.size;
	auto alignment = 0u;
	calc_aligned_sizes(static_cast<VlkContext&>(context).GetDevice(),bufferInstanceSize,bufferBaseSize,maxTotalSize,alignment,createInfo.usageFlags);

	auto buf = context.CreateBuffer(createInfo,data);
	if(buf == nullptr)
		return nullptr;
	auto r = std::shared_ptr<VkUniformResizableBuffer>(new VkUniformResizableBuffer{context,*buf,bufferInstanceSize,bufferBaseSize,maxTotalSize,alignment});
	r->Initialize();
	return r;
}
