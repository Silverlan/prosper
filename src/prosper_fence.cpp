// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "prosper_fence.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

IFence::IFence(IPrContext &context) : ContextObject(context), std::enable_shared_from_this<IFence>() {}

IFence::~IFence() {}
