// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "prosper_event.hpp"

using namespace prosper;

IEvent::IEvent(IPrContext &context) : ContextObject {context} {}
IEvent::~IEvent() {}
