/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_RENDER_TARGET_HPP__
#define __PROSPER_RENDER_TARGET_HPP__

#include "image/prosper_texture.hpp"

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class RenderPass;
	class Framebuffer;
	class ImageView;
	class Texture;
	class DLLPROSPER RenderTarget
		: public ContextObject,
		public std::enable_shared_from_this<RenderTarget>
	{
	public:
		RenderTarget(Context &context,const std::vector<std::shared_ptr<Texture>> &textures,const std::vector<std::shared_ptr<Framebuffer>> &framebuffers,const std::shared_ptr<RenderPass> &renderPass);

		uint32_t GetAttachmentCount() const;
		const std::shared_ptr<Texture> &GetTexture(uint32_t attachmentId=0u) const;
		const std::shared_ptr<Framebuffer> &GetFramebuffer(uint32_t layerId) const;
		const std::shared_ptr<Framebuffer> &GetFramebuffer() const;
		const std::shared_ptr<RenderPass> &GetRenderPass() const;
		virtual void SetDebugName(const std::string &name) override;
	private:
		std::vector<std::shared_ptr<Texture>> m_textures = {};
		std::vector<std::shared_ptr<Framebuffer>> m_framebuffers = {};
		std::shared_ptr<RenderPass> m_renderPass = nullptr;
	};

	namespace util
	{
		struct DLLPROSPER RenderTargetCreateInfo
		{
			bool useLayerFramebuffers = false;
		};
		DLLPROSPER std::shared_ptr<RenderTarget> create_render_target(Anvil::BaseDevice &dev,const std::vector<std::shared_ptr<Texture>> &textures,const std::shared_ptr<RenderPass> &rp=nullptr,const RenderTargetCreateInfo &rtCreateInfo={});
		DLLPROSPER std::shared_ptr<RenderTarget> create_render_target(Anvil::BaseDevice &dev,Texture &texture,ImageView &imgView,RenderPass &rp,const RenderTargetCreateInfo &rtCreateInfo={});
	};
};
#pragma warning(pop)

#endif
