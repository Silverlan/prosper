/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "buffers/prosper_render_buffer.hpp"

using namespace prosper;

IRenderBuffer::IRenderBuffer(
	prosper::IPrContext &context,const std::vector<prosper::IBuffer*> &buffers,const std::optional<IndexBufferInfo> &indexBufferInfo
)
	: prosper::ContextObject{context},m_indexBufferInfo{indexBufferInfo}
{
	m_buffers.reserve(buffers.size());
	for(auto *buf : buffers)
		m_buffers.push_back(buf->shared_from_this());
	if(m_indexBufferInfo.has_value() && m_indexBufferInfo->buffer == nullptr)
		m_indexBufferInfo = {};
}

const std::vector<std::shared_ptr<prosper::IBuffer>> &IRenderBuffer::GetBuffers() const {return m_buffers;}
const prosper::IndexBufferInfo *IRenderBuffer::GetIndexBufferInfo() const {return const_cast<IRenderBuffer*>(this)->GetIndexBufferInfo();}
prosper::IndexBufferInfo *IRenderBuffer::GetIndexBufferInfo() {return m_indexBufferInfo.has_value() ? &*m_indexBufferInfo : nullptr;}
