// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:render_pass;

export import :context_object;
export import :structs;

#undef max

export namespace prosper {
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
