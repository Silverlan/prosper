/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_context_object.hpp"
#include "prosper_context.hpp"

using namespace prosper;

ContextObject::ContextObject(IPrContext &context)
	: m_wpContext(context.shared_from_this())
{}

IPrContext &ContextObject::GetContext() const {return *(m_wpContext.lock());}

void prosper::ContextObject::SetDebugName(const std::string &name)
{
	// if(s_lookupHandler == nullptr)
	// 	return;
	if(m_dbgName.empty() == false)
		m_dbgName += ",";
	m_dbgName += name;
}
const std::string &prosper::ContextObject::GetDebugName() const {return m_dbgName;}
