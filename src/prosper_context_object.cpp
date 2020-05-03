/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_context_object.hpp"
#include "vk_context.hpp"
#include <wrappers/device.h>

using namespace prosper;

ContextObject::ContextObject(IPrContext &context)
	: m_wpContext(context.shared_from_this())
{}

IPrContext &ContextObject::GetContext() const {return *(m_wpContext.lock());}
Anvil::BaseDevice &ContextObject::GetDevice() const {return const_cast<VlkContext&>(static_cast<const VlkContext&>(GetContext())).GetDevice();}
