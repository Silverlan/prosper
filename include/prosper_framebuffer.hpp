/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_FRAMEBUFFER_HPP__
#define __PROSPER_FRAMEBUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/render_pass.h>

#undef max

namespace prosper
{
	class DLLPROSPER IFramebuffer
		: public ContextObject,
		public std::enable_shared_from_this<IFramebuffer>
	{
	public:
		IFramebuffer(const IFramebuffer&)=delete;
		IFramebuffer &operator=(const IFramebuffer&)=delete;
		virtual ~IFramebuffer() override;

		uint32_t GetAttachmentCount() const;
		prosper::IImageView *GetAttachment(uint32_t i);
		const prosper::IImageView *GetAttachment(uint32_t i) const;
		void GetSize(uint32_t &width,uint32_t &height,uint32_t &depth) const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		uint32_t GetLayerCount() const;
	protected:
		IFramebuffer(
			Context &context,const std::vector<std::shared_ptr<IImageView>> &attachments,
			uint32_t width,uint32_t height,uint32_t depth,uint32_t layers
		);
		std::vector<std::shared_ptr<IImageView>> m_attachments = {};
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_depth = 0;
		uint32_t m_layers = 1;
	};

	class DLLPROSPER Framebuffer
		: public IFramebuffer
	{
	public:
		static std::shared_ptr<Framebuffer> Create(
			Context &context,const std::vector<IImageView*> &attachments,
			uint32_t width,uint32_t height,uint32_t depth,uint32_t layers,
			std::unique_ptr<Anvil::Framebuffer,std::function<void(Anvil::Framebuffer*)>> fb,const std::function<void(Framebuffer&)> &onDestroyedCallback=nullptr
		);
		virtual ~Framebuffer() override;
		Anvil::Framebuffer &GetAnvilFramebuffer() const;
		Anvil::Framebuffer &operator*();
		const Anvil::Framebuffer &operator*() const;
		Anvil::Framebuffer *operator->();
		const Anvil::Framebuffer *operator->() const;
	protected:
		Framebuffer(
			Context &context,const std::vector<std::shared_ptr<IImageView>> &attachments,
			uint32_t width,uint32_t height,uint32_t depth,uint32_t layers,
			std::unique_ptr<Anvil::Framebuffer,std::function<void(Anvil::Framebuffer*)>> fb
		);
		std::unique_ptr<Anvil::Framebuffer,std::function<void(Anvil::Framebuffer*)>> m_framebuffer = nullptr;
	};
};

#endif
