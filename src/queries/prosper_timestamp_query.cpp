/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_timestamp_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_context.hpp"
#include <wrappers/command_buffer.h>

using namespace prosper;

TimestampQuery::TimestampQuery(QueryPool &queryPool,uint32_t queryId,Anvil::PipelineStageFlagBits pipelineStage)
	: Query(queryPool,queryId),m_pipelineStage(pipelineStage),m_wpQueryPool(queryPool.shared_from_this())
{}

bool TimestampQuery::Write(Anvil::CommandBufferBase &cmdBuffer)
{
	if(m_wpQueryPool.expired())
		return false;
	auto queryPool = m_wpQueryPool.lock();
	auto *anvPool = &queryPool->GetAnvilQueryPool();
	return cmdBuffer.record_reset_query_pool(anvPool,m_queryId,1u /* queryCount */) &&
		cmdBuffer.record_write_timestamp(m_pipelineStage,anvPool,m_queryId);
}

template<class T>
	bool TimestampQuery::QueryResult(long double &r) const
{
	T queryResult = 0;
	if(TimestampQuery::QueryResult(queryResult) == false)
		return false;
	r = static_cast<float>(queryResult) *GetContext().GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->limits.timestamp_period;
	return true;
}

template<class T>
	bool TimestampQuery::QueryResult(T &r) const
{
	auto result = 0.L;
	if(QueryResult(result) == false)
		return false;
	r = static_cast<T>(r);
	return true;
}

bool TimestampQuery::QueryResult(uint32_t &r) const {return QueryResult<uint32_t>(r);}
bool TimestampQuery::QueryResult(uint64_t &r) const {return QueryResult<uint64_t>(r);}

bool TimestampQuery::QueryResult(long double &r) const
{
	auto result = 0.0L;
	if(QueryResult<uint64_t>(result) == false)
		return false;
	r = result /1'000'000.0L;
	return true;
}

Anvil::PipelineStageFlagBits TimestampQuery::GetPipelineStage() const {return m_pipelineStage;}

std::shared_ptr<TimestampQuery> prosper::util::create_timestamp_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage)
{
	uint32_t query = 0;
	if(queryPool.RequestQuery(query) == false)
		return nullptr;
	return std::shared_ptr<TimestampQuery>(new TimestampQuery(queryPool,query,pipelineStage));
}
