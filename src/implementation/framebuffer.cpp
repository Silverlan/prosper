// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cinttypes>
#include <vector>
#include <memory>

module pragma.prosper;

import :framebuffer;

using namespace prosper;

IFramebuffer::IFramebuffer(IPrContext &context, const std::vector<std::shared_ptr<IImageView>> &attachments, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers)
    : ContextObject(context), std::enable_shared_from_this<IFramebuffer>(), m_attachments {attachments}, m_width {width}, m_height {height}, m_depth {depth}, m_layers {layers}
{
	//prosper::debug::register_debug_object(m_framebuffer->get_framebuffer(),this,prosper::debug::ObjectType::Framebuffer);
}

IFramebuffer::~IFramebuffer()
{
	//prosper::debug::deregister_debug_object(m_framebuffer->get_framebuffer());
}

prosper::IImageView *IFramebuffer::GetAttachment(uint32_t i) { return (i < m_attachments.size()) ? m_attachments.at(i).get() : nullptr; }
const prosper::IImageView *IFramebuffer::GetAttachment(uint32_t i) const { return const_cast<IFramebuffer *>(this)->GetAttachment(i); }

uint32_t IFramebuffer::GetAttachmentCount() const { return m_attachments.size(); }
void IFramebuffer::GetSize(uint32_t &width, uint32_t &height, uint32_t &depth) const
{
	width = m_width;
	height = m_height;
	depth = m_depth;
}
uint32_t IFramebuffer::GetWidth() const { return m_width; }
uint32_t IFramebuffer::GetHeight() const { return m_height; }
uint32_t IFramebuffer::GetLayerCount() const { return m_layers; }
