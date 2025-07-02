// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "prosper_render_pass.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

IRenderPass::IRenderPass(IPrContext &context, const prosper::util::RenderPassCreateInfo &createInfo) : ContextObject(context), std::enable_shared_from_this<IRenderPass>(), m_createInfo {createInfo} {}

IRenderPass::~IRenderPass() {}

const prosper::util::RenderPassCreateInfo &IRenderPass::GetCreateInfo() const { return m_createInfo; }
