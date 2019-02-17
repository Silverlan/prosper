/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "buffers/prosper_resizable_buffer.hpp"
#include "buffers/prosper_buffer.hpp"
#include <wrappers/image.h>

using namespace prosper;

ResizableBuffer::ResizableBuffer(Context &context,std::unique_ptr<Anvil::Buffer,std::function<void(Anvil::Buffer*)>> buf,const prosper::util::BufferCreateInfo &createInfo,uint64_t maxTotalSize)
	: Buffer(context,std::move(buf)),m_createInfo(createInfo),m_maxTotalSize(maxTotalSize),m_baseSize(createInfo.size)
{}

void ResizableBuffer::AddReallocationCallback(const std::function<void()> &fCallback) {m_reallocationCallbacks.push_back(fCallback);}
