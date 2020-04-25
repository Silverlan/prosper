/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_RENDER_PASS_HPP__
#define __PROSPER_RENDER_PASS_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/render_pass.h>

#undef max

namespace prosper
{
	class DLLPROSPER IRenderPass
		: public ContextObject,
		public std::enable_shared_from_this<IRenderPass>
	{
	public:
		IRenderPass(const IRenderPass&)=delete;
		IRenderPass &operator=(const IRenderPass&)=delete;
		virtual ~IRenderPass() override;
	protected:
		IRenderPass(Context &context);
	};

	class DLLPROSPER RenderPass
		: public IRenderPass
	{
	public:
		static std::shared_ptr<RenderPass> Create(Context &context,std::unique_ptr<Anvil::RenderPass,std::function<void(Anvil::RenderPass*)>> rp,const std::function<void(RenderPass&)> &onDestroyedCallback=nullptr);
		virtual ~RenderPass() override;
		Anvil::RenderPass &GetAnvilRenderPass() const;
		Anvil::RenderPass &operator*();
		const Anvil::RenderPass &operator*() const;
		Anvil::RenderPass *operator->();
		const Anvil::RenderPass *operator->() const;
	protected:
		RenderPass(Context &context,std::unique_ptr<Anvil::RenderPass,std::function<void(Anvil::RenderPass*)>> rp);
		std::unique_ptr<Anvil::RenderPass,std::function<void(Anvil::RenderPass*)>> m_renderPass = nullptr;
	};
};

#endif
