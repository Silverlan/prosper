// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"
#include <string>

export module pragma.prosper:image.render_target;

export import :image.texture;

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)

export namespace prosper {
	class IFramebuffer;
	class IImageView;
	class Texture;
	class DLLPROSPER RenderTarget : public ContextObject, public std::enable_shared_from_this<RenderTarget> {
      public:

		RenderTarget(const RenderTarget &) = delete;
		RenderTarget &operator=(const RenderTarget &) = delete;
		RenderTarget(IPrContext &context, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<IFramebuffer>> &framebuffers, IRenderPass &renderPass);

		void Bake();

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
