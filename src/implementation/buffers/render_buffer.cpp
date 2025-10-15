// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>

module pragma.prosper;

import :buffer.render_buffer;

using namespace prosper;

IRenderBuffer::IRenderBuffer(prosper::IPrContext &context, const prosper::GraphicsPipelineCreateInfo &pipelineCreateInfo, const std::vector<prosper::IBuffer *> &buffers, const std::vector<prosper::DeviceSize> &offsets, const std::optional<IndexBufferInfo> &indexBufferInfo)
    : prosper::ContextObject {context}, m_indexBufferInfo {indexBufferInfo}, m_offsets {offsets}, m_pipelineCreateInfo {pipelineCreateInfo}
{
	m_buffers.reserve(buffers.size());
	m_reallocationCallbacks.reserve(buffers.size());
	for(auto *buf : buffers) {
		m_buffers.push_back(buf->shared_from_this());
		m_reallocationCallbacks.push_back(buf->AddReallocationCallback([this]() { Reload(); }));
	}
	if(m_indexBufferInfo.has_value() && m_indexBufferInfo->buffer == nullptr)
		m_indexBufferInfo = {};
}

IRenderBuffer::~IRenderBuffer()
{
	for(auto &cb : m_reallocationCallbacks) {
		if(cb.IsValid() == false)
			continue;
		cb.Remove();
	}
}

const std::vector<std::shared_ptr<prosper::IBuffer>> &IRenderBuffer::GetBuffers() const { return m_buffers; }
const prosper::IndexBufferInfo *IRenderBuffer::GetIndexBufferInfo() const { return const_cast<IRenderBuffer *>(this)->GetIndexBufferInfo(); }
prosper::IndexBufferInfo *IRenderBuffer::GetIndexBufferInfo() { return m_indexBufferInfo.has_value() ? &*m_indexBufferInfo : nullptr; }
