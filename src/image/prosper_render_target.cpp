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

RenderTarget::RenderTarget(Context &context,const std::vector<std::shared_ptr<Texture>> &textures,const std::vector<std::shared_ptr<Framebuffer>> &framebuffers,const std::shared_ptr<RenderPass> &renderPass)
	: ContextObject(context),std::enable_shared_from_this<RenderTarget>(),m_textures(textures),m_framebuffers(framebuffers),m_renderPass(renderPass)
{}

uint32_t RenderTarget::GetAttachmentCount() const {return m_textures.size();}
const std::shared_ptr<Texture> &RenderTarget::GetTexture(uint32_t attachmentId) const
{
	static std::shared_ptr<Texture> nptr = nullptr;
	return (attachmentId < m_textures.size()) ? m_textures.at(attachmentId) : nptr;
}
const std::shared_ptr<Framebuffer> &RenderTarget::GetFramebuffer(uint32_t layerId) const
{
	static std::shared_ptr<Framebuffer> nptr = nullptr;
	++layerId;
	return (layerId < m_framebuffers.size()) ? m_framebuffers.at(layerId) : nptr;
}
const std::shared_ptr<Framebuffer> &RenderTarget::GetFramebuffer() const {return m_framebuffers.at(0);}
const std::shared_ptr<RenderPass> &RenderTarget::GetRenderPass() const {return m_renderPass;}
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

//////////////////////////

std::shared_ptr<RenderTarget> prosper::util::create_render_target(Anvil::BaseDevice &dev,const std::vector<std::shared_ptr<Texture>> &textures,const std::shared_ptr<RenderPass> &rp,const RenderTargetCreateInfo &rtCreateInfo)
{
	if(textures.empty())
		return nullptr;
	auto &tex = textures.front();
	if(tex == nullptr)
		return nullptr;
	auto &img = tex->GetImage();
	if(img == nullptr)
		return nullptr;
	auto &context = prosper::Context::GetContext(dev);
	auto extents = img->GetExtents();
	auto numLayers = img->GetLayerCount();
	std::vector<std::shared_ptr<Framebuffer>> framebuffers;
	std::vector<Anvil::ImageView*> attachments;
	attachments.reserve(textures.size());
	for(auto &tex : textures)
		attachments.push_back(&tex->GetImageView()->GetAnvilImageView());
	framebuffers.push_back(prosper::util::create_framebuffer(dev,extents.width,extents.height,1u,attachments));
	if(rtCreateInfo.useLayerFramebuffers == true)
	{
		framebuffers.reserve(framebuffers.size() +numLayers);
		for(auto i=decltype(numLayers){0};i<numLayers;++i)
		{
			attachments = {&tex->GetImageView(i)->GetAnvilImageView()};
			framebuffers.push_back(prosper::util::create_framebuffer(dev,extents.width,extents.height,1u,attachments));
		}
	}
	return std::make_shared<RenderTarget>(Context::GetContext(dev),textures,framebuffers,rp);
}
