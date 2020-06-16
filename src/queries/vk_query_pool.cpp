/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/vk_query_pool.hpp"
#include "vk_context.hpp"

using namespace prosper;

VlkQueryPool::VlkQueryPool(IPrContext &context,Anvil::QueryPoolUniquePtr queryPool,QueryType type)
	: IQueryPool{context,type,queryPool->get_capacity()},m_queryPool{std::move(queryPool)}
{}
Anvil::QueryPool &VlkQueryPool::GetAnvilQueryPool() const {return *m_queryPool;}
