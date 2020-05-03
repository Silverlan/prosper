/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_CONTEXT_OBJECT_HPP__
#define __PROSPER_CONTEXT_OBJECT_HPP__

#include "prosper_definitions.hpp"
#include <memory>
#include <string>

namespace Anvil
{
	class BaseDevice;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class IPrContext;
	class DLLPROSPER ContextObject
	{
	public:
		ContextObject(IPrContext &context);
		virtual ~ContextObject()=default;

		IPrContext &GetContext() const;
		Anvil::BaseDevice &GetDevice() const;

		virtual void SetDebugName(const std::string &name);
		const std::string &GetDebugName() const;

		// For internal use only
		virtual void OnRelease() {}
	private:
		mutable std::weak_ptr<IPrContext> m_wpContext = {};
		std::string m_dbgName;
	};
};
#pragma warning(pop)

#endif
