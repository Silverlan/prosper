/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_timestamp_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

TimestampQuery::TimestampQuery(IQueryPool &queryPool,uint32_t queryId,PipelineStageFlags pipelineStage)
	: Query(queryPool,queryId),m_pipelineStage(pipelineStage)
{}

void TimestampQuery::OnReset(prosper::ICommandBuffer &cmdBuffer)
{
	m_state = State::Reset;
}

bool TimestampQuery::Write(prosper::ICommandBuffer &cmdBuffer)
{
	m_state = State::Set;
	return cmdBuffer.WriteTimestampQuery(*this);
}
bool TimestampQuery::QueryResult(std::chrono::nanoseconds &outTimestampValue) const {return GetContext().QueryResult(*this,outTimestampValue);}
bool TimestampQuery::IsReset() const {return m_state != State::Set;}

prosper::PipelineStageFlags TimestampQuery::GetPipelineStage() const {return m_pipelineStage;}
