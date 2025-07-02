// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_context.hpp"

using namespace prosper;

PipelineStatisticsQuery::PipelineStatisticsQuery(IQueryPool &queryPool, uint32_t queryId) : Query(queryPool, queryId) {}

void PipelineStatisticsQuery::OnReset(prosper::ICommandBuffer &cmdBuffer)
{
	Query::OnReset(cmdBuffer);
	m_bReset = true;
}
bool PipelineStatisticsQuery::RecordBegin(prosper::ICommandBuffer &cmdBuffer) { return cmdBuffer.RecordBeginPipelineStatisticsQuery(*this); }
bool PipelineStatisticsQuery::RecordEnd(prosper::ICommandBuffer &cmdBuffer) const { return cmdBuffer.RecordEndPipelineStatisticsQuery(*this); }
bool PipelineStatisticsQuery::QueryResult(PipelineStatistics &outStatistics) const { return GetContext().QueryResult(*this, outStatistics); }
bool PipelineStatisticsQuery::IsReset() const { return m_bReset; }
