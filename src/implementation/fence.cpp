// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :fence;

using namespace prosper;

IFence::IFence(IPrContext &context) : ContextObject(context), std::enable_shared_from_this<IFence>() {}

IFence::~IFence() {}
