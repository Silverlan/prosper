// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


module pragma.prosper;

import :query.timestamp;

using namespace prosper;

TimestampQuery::TimestampQuery(IQueryPool &queryPool, uint32_t queryId, PipelineStageFlags pipelineStage) : Query(queryPool, queryId), m_pipelineStage(pipelineStage) {}

void TimestampQuery::OnReset(prosper::ICommandBuffer &cmdBuffer) { m_state = State::Reset; }

bool TimestampQuery::Write(prosper::ICommandBuffer &cmdBuffer)
{
	m_state = State::Set;
	return cmdBuffer.WriteTimestampQuery(*this);
}
bool TimestampQuery::QueryResult(std::chrono::nanoseconds &outTimestampValue) const { return GetContext().QueryResult(*this, outTimestampValue); }
bool TimestampQuery::IsReset() const { return m_state != State::Set; }

prosper::PipelineStageFlags TimestampQuery::GetPipelineStage() const { return m_pipelineStage; }
