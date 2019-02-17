/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_framebuffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/framebuffer_create_info.h>

using namespace prosper;

std::shared_ptr<Framebuffer> Framebuffer::Create(Context &context,Anvil::FramebufferUniquePtr fb,const std::function<void(Framebuffer&)> &onDestroyedCallback)
{
	if(fb == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<Framebuffer>(new Framebuffer(context,std::move(fb)));
	return std::shared_ptr<Framebuffer>(new Framebuffer(context,std::move(fb)),[onDestroyedCallback](Framebuffer *fb) {
		onDestroyedCallback(*fb);
		delete fb;
	});
}

Framebuffer::Framebuffer(Context &context,Anvil::FramebufferUniquePtr fb)
	: ContextObject(context),std::enable_shared_from_this<Framebuffer>(),m_framebuffer(std::move(fb))
{
	//prosper::debug::register_debug_object(m_framebuffer->get_framebuffer(),this,prosper::debug::ObjectType::Framebuffer);
}

Framebuffer::~Framebuffer()
{
	//prosper::debug::deregister_debug_object(m_framebuffer->get_framebuffer());
}

Anvil::Framebuffer &Framebuffer::GetAnvilFramebuffer() const {return *m_framebuffer;}
Anvil::Framebuffer &Framebuffer::operator*() {return *m_framebuffer;}
const Anvil::Framebuffer &Framebuffer::operator*() const {return const_cast<Framebuffer*>(this)->operator*();}
Anvil::Framebuffer *Framebuffer::operator->() {return m_framebuffer.get();}
const Anvil::Framebuffer *Framebuffer::operator->() const {return const_cast<Framebuffer*>(this)->operator->();}

uint32_t Framebuffer::GetAttachmentCount() const {return m_framebuffer->get_create_info_ptr()->get_n_attachments();}
void Framebuffer::GetSize(uint32_t &width,uint32_t &height,uint32_t &depth) const {return m_framebuffer->get_create_info_ptr()->get_size(&width,&height,&depth);}
uint32_t Framebuffer::GetWidth() const {return m_framebuffer->get_create_info_ptr()->get_width();}
uint32_t Framebuffer::GetHeight() const {return m_framebuffer->get_create_info_ptr()->get_height();}
uint32_t Framebuffer::GetLayerCount() const {return m_framebuffer->get_create_info_ptr()->get_n_layers();}
