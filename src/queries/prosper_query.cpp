/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_query.hpp"
#include "prosper_context.hpp"
#include "queries/prosper_query_pool.hpp"

using namespace prosper;

template<class T>
	bool Query::QueryResult(T &r,Anvil::QueryResultFlags resultFlags) const
{
	if(m_pool.expired())
		return false;
	resultFlags = resultFlags | Anvil::QueryResultFlagBits::WITH_AVAILABILITY_BIT;
	auto &context = GetContext();
	auto &dev = context.GetDevice();
	std::array<T,2> result = {0,0};
	auto err = static_cast<vk::Result>(vkGetQueryPoolResults(
		reinterpret_cast<VkDevice&>(dev),m_pool.lock()->GetAnvilQueryPool().get_query_pool(),m_queryId,1u,result.size() *sizeof(T),&result,sizeof(T),
		resultFlags.get_vk()
	));
	if(err != vk::Result::eSuccess)
		return false;
	r = result.front();
	return (result.back() > 0) ? true : false;
}

Query::Query(QueryPool &queryPool,uint32_t queryId)
	: ContextObject(queryPool.GetContext()),std::enable_shared_from_this<Query>(),
	m_queryId(queryId),m_pool(queryPool.shared_from_this())
{}

QueryPool *Query::GetPool() const {return (m_pool.expired() == false) ? m_pool.lock().get() : nullptr;}
bool Query::QueryResult(uint32_t &r) const {return QueryResult<uint32_t>(r,Anvil::QueryResultFlags{});}
bool Query::QueryResult(uint64_t &r) const {return QueryResult<uint64_t>(r,Anvil::QueryResultFlagBits::_64_BIT);}
bool Query::IsResultAvailable() const
{
	uint32_t r;
	return QueryResult(r);
}
