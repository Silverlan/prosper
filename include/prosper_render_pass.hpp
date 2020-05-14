/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_RENDER_PASS_HPP__
#define __PROSPER_RENDER_PASS_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

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
		const util::RenderPassCreateInfo &GetCreateInfo() const;
	protected:
		IRenderPass(IPrContext &context,const util::RenderPassCreateInfo &createInfo);
		util::RenderPassCreateInfo m_createInfo {};
	};
};

#endif
