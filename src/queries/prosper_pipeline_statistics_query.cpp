/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

PipelineStatisticsQuery::PipelineStatisticsQuery(IQueryPool &queryPool,uint32_t queryId)
	: Query(queryPool,queryId)
{}

void PipelineStatisticsQuery::OnReset(prosper::ICommandBuffer &cmdBuffer)
{
	Query::OnReset(cmdBuffer);
	m_bReset = true;
}
bool PipelineStatisticsQuery::RecordBegin(prosper::ICommandBuffer &cmdBuffer)
{
	return cmdBuffer.RecordBeginPipelineStatisticsQuery(*this);
}
bool PipelineStatisticsQuery::RecordEnd(prosper::ICommandBuffer &cmdBuffer) const
{
	return cmdBuffer.RecordEndPipelineStatisticsQuery(*this);
}
bool PipelineStatisticsQuery::QueryResult(PipelineStatistics &outStatistics) const {return GetContext().QueryResult(*this,outStatistics);}
bool PipelineStatisticsQuery::IsReset() const {return m_bReset;}
