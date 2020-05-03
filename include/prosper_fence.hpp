/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_FENCE_HPP__
#define __PROSPER_FENCE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

#undef max

namespace prosper
{
	class DLLPROSPER IFence
		: public ContextObject,
		public std::enable_shared_from_this<IFence>
	{
	public:
		IFence(const IFence&)=delete;
		IFence &operator=(const IFence&)=delete;
		virtual ~IFence() override;

		virtual bool IsSet() const=0;
		virtual bool Reset() const=0;
	protected:
		IFence(IPrContext &context);
	};
};

#endif
