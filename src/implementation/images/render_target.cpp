// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;



module pragma.prosper;

import :image.render_target;

using namespace prosper;

RenderTarget::RenderTarget(IPrContext &context, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<IFramebuffer>> &framebuffers, IRenderPass &renderPass)
    : ContextObject {context}, std::enable_shared_from_this<RenderTarget>(), m_textures {textures}, m_framebuffers {framebuffers}, m_renderPass {renderPass.shared_from_this()}
{
}

void RenderTarget::Bake()
{
	for(auto &tex : m_textures)
		tex->Bake();
	if(m_renderPass) {
		m_renderPass->Bake();
		for(auto &fb : m_framebuffers)
			fb->Bake(*m_renderPass);
	}
}

uint32_t RenderTarget::GetAttachmentCount() const { return m_textures.size(); }
const Texture *RenderTarget::GetTexture(uint32_t attachmentId) const { return const_cast<RenderTarget *>(this)->GetTexture(attachmentId); }
Texture *RenderTarget::GetTexture(uint32_t attachmentId) { return (attachmentId < m_textures.size()) ? m_textures.at(attachmentId).get() : nullptr; }
const IFramebuffer *RenderTarget::GetFramebuffer(uint32_t layerId) const { return const_cast<RenderTarget *>(this)->GetFramebuffer(layerId); }
IFramebuffer *RenderTarget::GetFramebuffer(uint32_t layerId)
{
	++layerId;
	return (layerId < m_framebuffers.size()) ? m_framebuffers.at(layerId).get() : nullptr;
}
const Texture &RenderTarget::GetTexture() const { return const_cast<RenderTarget *>(this)->GetTexture(); }
Texture &RenderTarget::GetTexture() { return *m_textures.front(); }
const IFramebuffer &RenderTarget::GetFramebuffer() const { return const_cast<RenderTarget *>(this)->GetFramebuffer(); }
IFramebuffer &RenderTarget::GetFramebuffer() { return *m_framebuffers.at(0); }
const IRenderPass &RenderTarget::GetRenderPass() const { return const_cast<RenderTarget *>(this)->GetRenderPass(); }
IRenderPass &RenderTarget::GetRenderPass() { return *m_renderPass; }
void RenderTarget::SetDebugName(const std::string &name)
{
	ContextObject::SetDebugName(name);
	auto idx = 0u;
	for(auto &tex : m_textures)
		tex->SetDebugName(name + "_tex" + std::to_string(idx++));
	idx = 0u;
	for(auto &fb : m_framebuffers)
		fb->SetDebugName(name + "_fb" + std::to_string(idx++));
	if(m_renderPass != nullptr)
		m_renderPass->SetDebugName(name + "_rp");
}
