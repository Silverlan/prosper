/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_context.hpp"
#include <wrappers/command_buffer.h>

using namespace prosper;

PipelineStatisticsQuery::PipelineStatisticsQuery(QueryPool &queryPool,uint32_t queryId)
	: Query(queryPool,queryId)
{}

bool PipelineStatisticsQuery::Reset(Anvil::CommandBufferBase &cmdBuffer)
{
	auto r = Query::Reset(cmdBuffer);
	if(r == true)
		m_bReset = true;
	return r;
}
bool PipelineStatisticsQuery::RecordBegin(Anvil::CommandBufferBase &cmdBuffer)
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	if(m_bReset == false && Reset(cmdBuffer) == false)
		return false;
	return cmdBuffer.record_begin_query(anvPool,m_queryId,{});
}
bool PipelineStatisticsQuery::RecordEnd(Anvil::CommandBufferBase &cmdBuffer) const
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return cmdBuffer.record_end_query(anvPool,m_queryId);
}

bool PipelineStatisticsQuery::QueryResult(Statistics &outStatistics) const
{
	return Query::QueryResult<Statistics,uint64_t>(outStatistics,Anvil::QueryResultFlagBits::_64_BIT);
}

std::shared_ptr<PipelineStatisticsQuery> prosper::util::create_pipeline_statistics_query(QueryPool &queryPool)
{
	if(queryPool.GetContext().GetDevice().get_physical_device_features().core_vk1_0_features_ptr->pipeline_statistics_query == false)
		return nullptr;
	uint32_t query = 0;
	if(queryPool.RequestQuery(query) == false)
		return nullptr;
	return std::shared_ptr<PipelineStatisticsQuery>(new PipelineStatisticsQuery(queryPool,query));
}
