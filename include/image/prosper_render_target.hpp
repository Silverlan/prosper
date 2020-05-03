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
	class IRenderPass;
	class IFramebuffer;
	class IImageView;
	class Texture;
	class DLLPROSPER RenderTarget
		: public ContextObject,
		public std::enable_shared_from_this<RenderTarget>
	{
	public:
		RenderTarget(const RenderTarget&)=delete;
		RenderTarget &operator=(const RenderTarget&)=delete;
		RenderTarget(
			IPrContext &context,const std::vector<std::shared_ptr<Texture>> &textures,
			const std::vector<std::shared_ptr<IFramebuffer>> &framebuffers,
			IRenderPass &renderPass
		);

		uint32_t GetAttachmentCount() const;
		const Texture *GetTexture(uint32_t attachmentId) const;
		Texture *GetTexture(uint32_t attachmentId);

		const Texture &GetTexture() const;
		Texture &GetTexture();

		const IFramebuffer *GetFramebuffer(uint32_t layerId) const;
		IFramebuffer *GetFramebuffer(uint32_t layerId);

		const IFramebuffer &GetFramebuffer() const;
		IFramebuffer &GetFramebuffer();

		const IRenderPass &GetRenderPass() const;
		IRenderPass &GetRenderPass();

		virtual void SetDebugName(const std::string &name) override;
	private:
		std::vector<std::shared_ptr<Texture>> m_textures = {};
		std::vector<std::shared_ptr<IFramebuffer>> m_framebuffers = {};
		std::shared_ptr<IRenderPass> m_renderPass = nullptr;
	};
};
#pragma warning(pop)

#endif
