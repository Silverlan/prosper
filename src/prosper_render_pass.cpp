/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_render_pass.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

IRenderPass::IRenderPass(Context &context)
	: ContextObject(context),std::enable_shared_from_this<IRenderPass>()
{}

IRenderPass::~IRenderPass() {}

///////////////

std::shared_ptr<RenderPass> RenderPass::Create(Context &context,Anvil::RenderPassUniquePtr imgView,const std::function<void(RenderPass&)> &onDestroyedCallback)
{
	if(imgView == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<RenderPass>(new RenderPass(context,std::move(imgView)));
	return std::shared_ptr<RenderPass>(new RenderPass(context,std::move(imgView)),[onDestroyedCallback](RenderPass *buf) {
		buf->OnRelease();
		onDestroyedCallback(*buf);
		delete buf;
	});
}

RenderPass::RenderPass(Context &context,Anvil::RenderPassUniquePtr imgView)
	: IRenderPass{context},m_renderPass{std::move(imgView)}
{
	prosper::debug::register_debug_object(m_renderPass->get_render_pass(),this,prosper::debug::ObjectType::RenderPass);
}
RenderPass::~RenderPass()
{
	prosper::debug::deregister_debug_object(m_renderPass->get_render_pass());
}

Anvil::RenderPass &RenderPass::GetAnvilRenderPass() const {return *m_renderPass;}
Anvil::RenderPass &RenderPass::operator*() {return *m_renderPass;}
const Anvil::RenderPass &RenderPass::operator*() const {return const_cast<RenderPass*>(this)->operator*();}
Anvil::RenderPass *RenderPass::operator->() {return m_renderPass.get();}
const Anvil::RenderPass *RenderPass::operator->() const {return const_cast<RenderPass*>(this)->operator->();}
