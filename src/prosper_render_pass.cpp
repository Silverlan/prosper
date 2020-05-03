/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_render_pass.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

IRenderPass::IRenderPass(IPrContext &context)
	: ContextObject(context),std::enable_shared_from_this<IRenderPass>()
{}

IRenderPass::~IRenderPass() {}
