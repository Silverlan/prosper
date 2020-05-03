/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_render_target.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "prosper_framebuffer.hpp"
#include "image/prosper_image_view.hpp"
#include "prosper_render_pass.hpp"
#include <misc/image_create_info.h>

using namespace prosper;

RenderTarget::RenderTarget(
	IPrContext &context,const std::vector<std::shared_ptr<Texture>> &textures,
	const std::vector<std::shared_ptr<IFramebuffer>> &framebuffers,
	IRenderPass &renderPass
)
	: ContextObject{context},std::enable_shared_from_this<RenderTarget>(),m_textures{textures},m_framebuffers{framebuffers},m_renderPass{renderPass.shared_from_this()}
{}

uint32_t RenderTarget::GetAttachmentCount() const {return m_textures.size();}
const Texture *RenderTarget::GetTexture(uint32_t attachmentId) const {return const_cast<RenderTarget*>(this)->GetTexture(attachmentId);}
Texture *RenderTarget::GetTexture(uint32_t attachmentId)
{
	return (attachmentId < m_textures.size()) ? m_textures.at(attachmentId).get() : nullptr;
}
const IFramebuffer *RenderTarget::GetFramebuffer(uint32_t layerId) const {return const_cast<RenderTarget*>(this)->GetFramebuffer(layerId);}
IFramebuffer *RenderTarget::GetFramebuffer(uint32_t layerId)
{
	++layerId;
	return (layerId < m_framebuffers.size()) ? m_framebuffers.at(layerId).get() : nullptr;
}
const Texture &RenderTarget::GetTexture() const {return const_cast<RenderTarget*>(this)->GetTexture();}
Texture &RenderTarget::GetTexture() {return *m_textures.front();}
const IFramebuffer &RenderTarget::GetFramebuffer() const {return const_cast<RenderTarget*>(this)->GetFramebuffer();}
IFramebuffer &RenderTarget::GetFramebuffer() {return *m_framebuffers.at(0);}
const IRenderPass &RenderTarget::GetRenderPass() const {return const_cast<RenderTarget*>(this)->GetRenderPass();}
IRenderPass &RenderTarget::GetRenderPass() {return *m_renderPass;}
void RenderTarget::SetDebugName(const std::string &name)
{
	ContextObject::SetDebugName(name);
	auto idx = 0u;
	for(auto &tex : m_textures)
		tex->SetDebugName(name +"_tex" +std::to_string(idx++));
	idx = 0u;
	for(auto &fb : m_framebuffers)
		fb->SetDebugName(name +"_fb" +std::to_string(idx++));
	if(m_renderPass != nullptr)
		m_renderPass->SetDebugName(name +"_rp");
}
