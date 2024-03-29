/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_render_pass.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

IRenderPass::IRenderPass(IPrContext &context, const prosper::util::RenderPassCreateInfo &createInfo) : ContextObject(context), std::enable_shared_from_this<IRenderPass>(), m_createInfo {createInfo} {}

IRenderPass::~IRenderPass() {}

const prosper::util::RenderPassCreateInfo &IRenderPass::GetCreateInfo() const { return m_createInfo; }
