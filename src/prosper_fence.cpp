/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_fence.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/fence_create_info.h>

using namespace prosper;

std::shared_ptr<Fence> Fence::Create(Context &context,bool createSignalled,const std::function<void(Fence&)> &onDestroyedCallback)
{
	auto fence = Anvil::Fence::create(Anvil::FenceCreateInfo::create(&context.GetDevice(),createSignalled));
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<Fence>(new Fence(context,std::move(fence)));
	return std::shared_ptr<Fence>(new Fence(context,std::move(fence)),[onDestroyedCallback](Fence *fence) {
		onDestroyedCallback(*fence);
		delete fence;
	});
}

Fence::Fence(Context &context,Anvil::FenceUniquePtr fence)
	: ContextObject(context),std::enable_shared_from_this<Fence>(),m_fence(std::move(fence))
{
	//prosper::debug::register_debug_object(m_framebuffer->get_framebuffer(),this,prosper::debug::ObjectType::Fence);
}

Fence::~Fence()
{
	//prosper::debug::deregister_debug_object(m_framebuffer->get_framebuffer());
}

bool Fence::IsSet() const {return m_fence->is_set();}
bool Fence::Reset() const {return m_fence->reset();}

Anvil::Fence &Fence::GetAnvilFence() const {return *m_fence;}
Anvil::Fence &Fence::operator*() {return *m_fence;}
const Anvil::Fence &Fence::operator*() const {return const_cast<Fence*>(this)->operator*();}
Anvil::Fence *Fence::operator->() {return m_fence.get();}
const Anvil::Fence *Fence::operator->() const {return const_cast<Fence*>(this)->operator->();}
