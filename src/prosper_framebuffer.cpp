/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_framebuffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_image_view.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/framebuffer_create_info.h>

using namespace prosper;

IFramebuffer::IFramebuffer(
	Context &context,const std::vector<std::shared_ptr<IImageView>> &attachments,
	uint32_t width,uint32_t height,uint32_t depth,uint32_t layers
)
	: ContextObject(context),std::enable_shared_from_this<IFramebuffer>(),m_attachments{attachments},
	m_width{width},m_height{height},m_depth{depth},m_layers{layers}
{
	//prosper::debug::register_debug_object(m_framebuffer->get_framebuffer(),this,prosper::debug::ObjectType::Framebuffer);
}

IFramebuffer::~IFramebuffer()
{
	//prosper::debug::deregister_debug_object(m_framebuffer->get_framebuffer());
}

prosper::IImageView *IFramebuffer::GetAttachment(uint32_t i) {return (i < m_attachments.size()) ? m_attachments.at(i).get() : nullptr;}
const prosper::IImageView *IFramebuffer::GetAttachment(uint32_t i) const {return const_cast<IFramebuffer*>(this)->GetAttachment(i);}

uint32_t IFramebuffer::GetAttachmentCount() const {return m_attachments.size();}
void IFramebuffer::GetSize(uint32_t &width,uint32_t &height,uint32_t &depth) const
{
	width = m_width;
	height = m_height;
	depth = m_depth;
}
uint32_t IFramebuffer::GetWidth() const {return m_width;}
uint32_t IFramebuffer::GetHeight() const {return m_height;}
uint32_t IFramebuffer::GetLayerCount() const {return m_layers;}

///////////////

std::shared_ptr<Framebuffer> Framebuffer::Create(
	Context &context,const std::vector<IImageView*> &attachments,
	uint32_t width,uint32_t height,uint32_t depth,uint32_t layers,
	Anvil::FramebufferUniquePtr fb,const std::function<void(Framebuffer&)> &onDestroyedCallback
)
{
	if(fb == nullptr)
		return nullptr;
	std::vector<std::shared_ptr<IImageView>> ptrAttachments {};
	ptrAttachments.reserve(attachments.size());
	for(auto *att : attachments)
		ptrAttachments.push_back(att->shared_from_this());
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<Framebuffer>(new Framebuffer{context,ptrAttachments,width,height,depth,layers,std::move(fb)});
	return std::shared_ptr<Framebuffer>(new Framebuffer{context,ptrAttachments,width,height,depth,layers,std::move(fb)},[onDestroyedCallback](Framebuffer *fb) {
		fb->OnRelease();
		onDestroyedCallback(*fb);
		delete fb;
	});
}
Framebuffer::Framebuffer(
	Context &context,const std::vector<std::shared_ptr<IImageView>> &attachments,
	uint32_t width,uint32_t height,uint32_t depth,uint32_t layers,
	std::unique_ptr<Anvil::Framebuffer,std::function<void(Anvil::Framebuffer*)>> fb
)
	: IFramebuffer{context,attachments,width,height,depth,layers},m_framebuffer{std::move(fb)}
{
	//prosper::debug::register_debug_object(m_framebuffer->get_framebuffer(),this,prosper::debug::ObjectType::Framebuffer);
}
Framebuffer::~Framebuffer() {}
Anvil::Framebuffer &Framebuffer::GetAnvilFramebuffer() const {return *m_framebuffer;}
Anvil::Framebuffer &Framebuffer::operator*() {return *m_framebuffer;}
const Anvil::Framebuffer &Framebuffer::operator*() const {return const_cast<Framebuffer*>(this)->operator*();}
Anvil::Framebuffer *Framebuffer::operator->() {return m_framebuffer.get();}
const Anvil::Framebuffer *Framebuffer::operator->() const {return const_cast<Framebuffer*>(this)->operator->();}
