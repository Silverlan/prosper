/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_query_pool.hpp"
#include "prosper_context.hpp"
#include <wrappers/query_pool.h>

using namespace prosper;

QueryPool::QueryPool(Context &context,Anvil::QueryPoolUniquePtr queryPool,vk::QueryType type)
	: ContextObject(context),std::enable_shared_from_this<QueryPool>(),m_queryPool(std::move(queryPool)),
	m_type(type),m_queryCount(m_queryPool->get_capacity())
{}
Anvil::QueryPool &QueryPool::GetAnvilQueryPool() const {return *m_queryPool;}
bool QueryPool::RequestQuery(uint32_t &queryId)
{
	if(m_freeQueries.empty() == false)
	{
		queryId = m_freeQueries.front();
		m_freeQueries.pop();
		return true;
	}
	if(m_nextQueryId >= m_queryCount)
		return false;
	queryId = m_nextQueryId++;
	return true;
}
void QueryPool::FreeQuery(uint32_t queryId) {m_freeQueries.push(queryId);}
std::shared_ptr<QueryPool> prosper::util::create_query_pool(Context &context,vk::QueryType queryType,uint32_t maxConcurrentQueries)
{
	if(queryType == vk::QueryType::ePipelineStatistics)
		throw std::logic_error("Cannot create pipeline statistics query pool using this overload!");
	auto pool = Anvil::QueryPool::create_non_ps_query_pool(&context.GetDevice(),static_cast<VkQueryType>(queryType),maxConcurrentQueries);
	if(pool == nullptr)
		return nullptr;
	return std::shared_ptr<QueryPool>(new QueryPool(context,std::move(pool),queryType));
}
std::shared_ptr<QueryPool> prosper::util::create_query_pool(Context &context,Anvil::QueryPipelineStatisticFlags statsFlags,uint32_t maxConcurrentQueries)
{
	auto pool = Anvil::QueryPool::create_ps_query_pool(&context.GetDevice(),statsFlags,maxConcurrentQueries);
	if(pool == nullptr)
		return nullptr;
	return std::shared_ptr<QueryPool>(new QueryPool(context,std::move(pool),vk::QueryType::ePipelineStatistics));
}
