// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_FENCE_HPP__
#define __PROSPER_FENCE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

#undef max

namespace prosper {
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

#endif
