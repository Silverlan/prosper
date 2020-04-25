/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_query.hpp"
#include "prosper_context.hpp"
#include "prosper_command_buffer.hpp"
#include "queries/prosper_query_pool.hpp"
#include "vk_command_buffer.hpp"

using namespace prosper;

Query::Query(QueryPool &queryPool,uint32_t queryId)
	: ContextObject(queryPool.GetContext()),std::enable_shared_from_this<Query>(),
	m_queryId(queryId),m_pool(queryPool.shared_from_this())
{}

Query::~Query()
{
	if(m_pool.expired() == false)
		m_pool.lock()->FreeQuery(m_queryId);
}

QueryPool *Query::GetPool() const {return (m_pool.expired() == false) ? m_pool.lock().get() : nullptr;}
bool Query::Reset(prosper::ICommandBuffer &cmdBuffer)
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return dynamic_cast<VlkCommandBuffer&>(cmdBuffer)->record_reset_query_pool(anvPool,m_queryId,1u /* queryCount */);
}
bool Query::QueryResult(uint32_t &r) const {return QueryResult<uint32_t>(r,prosper::QueryResultFlags{});}
bool Query::QueryResult(uint64_t &r) const {return QueryResult<uint64_t>(r,prosper::QueryResultFlags::e64Bit);}
bool Query::IsResultAvailable() const
{
	uint32_t r;
	return QueryResult(r);
}
