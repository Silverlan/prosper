// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_EVENT_HPP__
#define __PROSPER_EVENT_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

#undef max

namespace prosper {
	class DLLPROSPER IEvent : public ContextObject, public std::enable_shared_from_this<IEvent> {
	  public:
		IEvent(const IEvent &) = delete;
		IEvent &operator=(const IEvent &) = delete;
		virtual ~IEvent() override;

		virtual bool IsSet() const = 0;
		virtual const void *GetInternalHandle() const { return nullptr; }
	  protected:
		IEvent(IPrContext &context);
	};
};

#endif
