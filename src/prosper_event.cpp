/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_event.hpp"

using namespace prosper;

IEvent::IEvent(IPrContext &context)
	: ContextObject{context}
{}
IEvent::~IEvent() {}
