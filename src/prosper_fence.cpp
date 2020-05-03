/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_fence.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/fence_create_info.h>

using namespace prosper;

IFence::IFence(IPrContext &context)
	: ContextObject(context),std::enable_shared_from_this<IFence>()
{}

IFence::~IFence() {}
