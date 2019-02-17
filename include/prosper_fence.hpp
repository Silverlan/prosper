/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_FENCE_HPP__
#define __PROSPER_FENCE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/fence.h>

#undef max

namespace prosper
{
	class DLLPROSPER Fence
		: public ContextObject,
		public std::enable_shared_from_this<Fence>
	{
	public:
		static std::shared_ptr<Fence> Create(Context &context,bool createSignalled=false,const std::function<void(Fence&)> &onDestroyedCallback=nullptr);
		virtual ~Fence() override;
		Anvil::Fence &GetAnvilFence() const;
		Anvil::Fence &operator*();
		const Anvil::Fence &operator*() const;
		Anvil::Fence *operator->();
		const Anvil::Fence *operator->() const;

		bool IsSet() const;
		bool Reset() const;
	protected:
		Fence(Context &context,std::unique_ptr<Anvil::Fence,std::function<void(Anvil::Fence*)>> fence);
		std::unique_ptr<Anvil::Fence,std::function<void(Anvil::Fence*)>> m_fence = nullptr;
	};
};

#endif
