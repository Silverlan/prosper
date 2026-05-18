// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :context_object;
import :debug.object_register;

using namespace prosper;

ContextObject::ContextObject(IPrContext &context) : m_wpContext(context.shared_from_this()) { debug::ObjectRegister::register_context_object(*this); }

ContextObject::~ContextObject() { debug::ObjectRegister::unregister_context_object(*this); }

IPrContext &ContextObject::GetContext() const { return *(m_wpContext.lock()); }

void ContextObject::SetDebugName(const std::string &name)
{
	// if(s_lookupHandler == nullptr)
	// 	return;
	if(m_dbgName.empty() == false)
		m_dbgName += ",";
	m_dbgName += name;
}
const std::string &ContextObject::GetDebugName() const { return m_dbgName; }
