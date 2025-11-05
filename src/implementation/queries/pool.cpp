// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :query.pool;

using namespace prosper;

IQueryPool::IQueryPool(IPrContext &context, QueryType type, uint32_t queryCount) : ContextObject(context), std::enable_shared_from_this<IQueryPool>(), m_type(type), m_queryCount {queryCount} {}
bool IQueryPool::RequestQuery(uint32_t &queryId, QueryType type)
{
	if(m_freeQueries.empty() == false) {
		queryId = m_freeQueries.front();
		m_freeQueries.pop();
		return true;
	}
	if(m_nextQueryId >= m_queryCount)
		return false;
	queryId = m_nextQueryId++;
	return true;
}
void IQueryPool::FreeQuery(uint32_t queryId) { m_freeQueries.push(queryId); }
std::shared_ptr<OcclusionQuery> IQueryPool::CreateOcclusionQuery()
{
	uint32_t query = 0;
	if(RequestQuery(query, QueryType::Occlusion) == false)
		return nullptr;
	return std::shared_ptr<OcclusionQuery>(new OcclusionQuery(*this, query));
}
std::shared_ptr<PipelineStatisticsQuery> IQueryPool::CreatePipelineStatisticsQuery()
{
	uint32_t query = 0;
	if(RequestQuery(query, QueryType::PipelineStatistics) == false)
		return nullptr;
	return std::shared_ptr<PipelineStatisticsQuery>(new PipelineStatisticsQuery(*this, query));
}
std::shared_ptr<TimestampQuery> IQueryPool::CreateTimestampQuery(PipelineStageFlags pipelineStage)
{
	uint32_t query = 0;
	if(RequestQuery(query, QueryType::Timestamp) == false)
		return nullptr;
	return std::shared_ptr<TimestampQuery>(new TimestampQuery(*this, query, pipelineStage));
}
std::shared_ptr<TimerQuery> IQueryPool::CreateTimerQuery(PipelineStageFlags pipelineStageStart, PipelineStageFlags pipelineStageEnd)
{
	auto tsQuery0 = CreateTimestampQuery(pipelineStageStart);
	if(tsQuery0 == nullptr)
		return nullptr;
	auto tsQuery1 = CreateTimestampQuery(pipelineStageEnd);
	if(tsQuery1 == nullptr)
		return nullptr;
	return std::shared_ptr<TimerQuery>(new TimerQuery(tsQuery0, tsQuery1));
}
