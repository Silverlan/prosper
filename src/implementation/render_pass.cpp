// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>

module pragma.prosper;

import :render_pass;

using namespace prosper;

IRenderPass::IRenderPass(IPrContext &context, const prosper::util::RenderPassCreateInfo &createInfo) : ContextObject(context), std::enable_shared_from_this<IRenderPass>(), m_createInfo {createInfo} {}

IRenderPass::~IRenderPass() {}

const prosper::util::RenderPassCreateInfo &IRenderPass::GetCreateInfo() const { return m_createInfo; }
