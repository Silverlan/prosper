// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:event;

export import :context_object;

#undef max

export namespace prosper {
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
