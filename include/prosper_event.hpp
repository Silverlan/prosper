/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_EVENT_HPP__
#define __PROSPER_EVENT_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

#undef max

namespace prosper
{
	class DLLPROSPER IEvent
		: public ContextObject,
		public std::enable_shared_from_this<IEvent>
	{
	public:
		IEvent(const IEvent&)=delete;
		IEvent &operator=(const IEvent&)=delete;
		virtual ~IEvent() override;

		virtual bool IsSet() const=0;
	protected:
		IEvent(IPrContext &context);
	};
};

#endif
