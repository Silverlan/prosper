/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_CONTEXT_OBJECT_HPP__
#define __PROSPER_CONTEXT_OBJECT_HPP__

#include "prosper_definitions.hpp"
#include <memory>

namespace Anvil
{
	class BaseDevice;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Context;
	class DLLPROSPER ContextObject
	{
	public:
		ContextObject(Context &context);
		virtual ~ContextObject()=default;

		Context &GetContext() const;
		Anvil::BaseDevice &GetDevice() const;

		virtual void SetDebugName(const std::string &name);
		const std::string &GetDebugName() const;
	private:
		mutable std::weak_ptr<Context> m_wpContext = {};
		std::string m_dbgName;
	};
};
#pragma warning(pop)

#endif
