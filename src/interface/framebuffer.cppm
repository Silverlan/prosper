// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"


export module pragma.prosper:framebuffer;

export import :render_pass;

#undef max

export namespace prosper {
	class IImageView;
	class DLLPROSPER IFramebuffer : public ContextObject, public std::enable_shared_from_this<IFramebuffer> {
	  public:
		IFramebuffer(const IFramebuffer &) = delete;
		IFramebuffer &operator=(const IFramebuffer &) = delete;
		virtual ~IFramebuffer() override;

		virtual void Bake(IRenderPass &rp) {}
		uint32_t GetAttachmentCount() const;
		prosper::IImageView *GetAttachment(uint32_t i);
		const prosper::IImageView *GetAttachment(uint32_t i) const;
		void GetSize(uint32_t &width, uint32_t &height, uint32_t &depth) const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		uint32_t GetLayerCount() const;
		virtual const void *GetInternalHandle() const = 0;
	  protected:
		IFramebuffer(IPrContext &context, const std::vector<std::shared_ptr<IImageView>> &attachments, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers);
		std::vector<std::shared_ptr<IImageView>> m_attachments = {};
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_depth = 0;
		uint32_t m_layers = 1;
	};
};
