/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_dynamic_resizable_buffer.hpp"
#include "prosper_util.hpp"
#include "buffers/prosper_buffer.hpp"
#include "vk_context.hpp"
#include <wrappers/device.h>

using namespace prosper;

prosper::VkDynamicResizableBuffer::VkDynamicResizableBuffer(
	IPrContext &context,IBuffer &buffer,const util::BufferCreateInfo &createInfo,uint64_t maxTotalSize
)
	: IDynamicResizableBuffer{context,buffer,createInfo,maxTotalSize},
	IBuffer{buffer.GetContext(),buffer.GetCreateInfo(),buffer.GetStartOffset(),buffer.GetSize()},
	VlkBuffer{buffer.GetContext(),buffer.GetCreateInfo(),buffer.GetStartOffset(),buffer.GetSize(),nullptr}
{
	VlkBuffer::m_buffer = std::move(dynamic_cast<VlkBuffer&>(buffer).m_buffer);
}

std::shared_ptr<VkDynamicResizableBuffer> prosper::util::create_dynamic_resizable_buffer(IPrContext &context,BufferCreateInfo createInfo,uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage,const void *data)
{
	createInfo.size = prosper::util::clamp_gpu_memory_size(static_cast<VlkContext&>(context).GetDevice(),createInfo.size,clampSizeToAvailableGPUMemoryPercentage,createInfo.memoryFeatures);
	maxTotalSize = prosper::util::clamp_gpu_memory_size(static_cast<VlkContext&>(context).GetDevice(),maxTotalSize,clampSizeToAvailableGPUMemoryPercentage,createInfo.memoryFeatures);
	auto buf = context.CreateBuffer(createInfo,data);
	if(buf == nullptr)
		return nullptr;
	auto r = std::shared_ptr<VkDynamicResizableBuffer>(new VkDynamicResizableBuffer{context,*buf,createInfo,maxTotalSize});
	r->Initialize();
	return r;
}
