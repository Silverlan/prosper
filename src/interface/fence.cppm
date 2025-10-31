// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:fence;

export import :context_object;

#undef max

export namespace prosper {
	class DLLPROSPER IFence : public ContextObject, public std::enable_shared_from_this<IFence> {
	  public:
		IFence(const IFence &) = delete;
		IFence &operator=(const IFence &) = delete;
		virtual ~IFence() override;
		virtual const void *GetInternalHandle() const { return nullptr; }

		virtual bool IsSet() const = 0;
		virtual bool Reset() const = 0;
	  protected:
		IFence(IPrContext &context);
	};
};
