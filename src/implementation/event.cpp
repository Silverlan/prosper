// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :event;

using namespace prosper;

IEvent::IEvent(IPrContext &context) : ContextObject {context} {}
IEvent::~IEvent() {}
