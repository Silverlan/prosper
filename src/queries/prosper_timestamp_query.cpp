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
	: Query(queryPool,queryId),m_pipelineStage(pipelineStage)
{}

bool TimestampQuery::Reset(Anvil::CommandBufferBase &cmdBuffer)
{
	auto r = Query::Reset(cmdBuffer);
	if(r == true)
		m_bReset = true;
	return r;
}

bool TimestampQuery::Write(Anvil::CommandBufferBase &cmdBuffer)
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	if(m_bReset == false && Reset(cmdBuffer) == false)
		return false;
	return cmdBuffer.record_write_timestamp(m_pipelineStage,anvPool,m_queryId);
}

bool TimestampQuery::QueryResult(std::chrono::nanoseconds &outTimestampValue) const
{
	uint64_t result;
	if(Query::QueryResult(result) == false)
		return false;
	auto ns = result *static_cast<double>(GetContext().GetDevice().get_physical_device_properties().core_vk1_0_properties_ptr->limits.timestamp_period);
	outTimestampValue = static_cast<std::chrono::nanoseconds>(static_cast<uint64_t>(ns));
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
