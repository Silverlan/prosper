// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "queries/prosper_query.hpp"
#include "prosper_context.hpp"
#include "prosper_command_buffer.hpp"
#include "queries/prosper_query_pool.hpp"

using namespace prosper;

Query::Query(IQueryPool &queryPool, uint32_t queryId) : ContextObject(queryPool.GetContext()), std::enable_shared_from_this<Query>(), m_queryId(queryId), m_pool(queryPool.shared_from_this()) {}

Query::~Query()
{
	if(m_pool.expired() == false)
		m_pool.lock()->FreeQuery(m_queryId);
}

IQueryPool *Query::GetPool() const { return (m_pool.expired() == false) ? m_pool.lock().get() : nullptr; }
void Query::OnReset(ICommandBuffer &cmdBuffer) {}
uint32_t Query::GetQueryId() const { return m_queryId; }
bool Query::IsResultAvailable() const
{
	uint32_t r;
	return QueryResult(r);
}
bool Query::Reset(ICommandBuffer &cmdBuffer) const { return cmdBuffer.ResetQuery(*this); }
bool Query::QueryResult(uint32_t &r) const { return GetContext().QueryResult(*this, r); }
bool Query::QueryResult(uint64_t &r) const { return GetContext().QueryResult(*this, r); }
