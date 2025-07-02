// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_RENDER_PASS_HPP__
#define __PROSPER_RENDER_PASS_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"

#undef max

namespace prosper {
	class DLLPROSPER IRenderPass : public ContextObject, public std::enable_shared_from_this<IRenderPass> {
	  public:
		IRenderPass(const IRenderPass &) = delete;
		IRenderPass &operator=(const IRenderPass &) = delete;
		virtual ~IRenderPass() override;
		const util::RenderPassCreateInfo &GetCreateInfo() const;
		virtual void Bake() {}
		virtual const void *GetInternalHandle() const { return nullptr; }
	  protected:
		IRenderPass(IPrContext &context, const util::RenderPassCreateInfo &createInfo);
		util::RenderPassCreateInfo m_createInfo {};
	};
};

#endif
