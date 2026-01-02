// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:query.pool;

export import :enums;
export import :context_object;

export namespace prosper {
	class IQueryPool;
	class OcclusionQuery;
	class PipelineStatisticsQuery;
	class TimestampQuery;
	class TimerQuery;
	class DLLPROSPER IQueryPool : public ContextObject, public std::enable_shared_from_this<IQueryPool> {
	  public:
		virtual bool RequestQuery(uint32_t &queryId, QueryType type);
		void FreeQuery(uint32_t queryId);

		std::shared_ptr<OcclusionQuery> CreateOcclusionQuery();
		std::shared_ptr<PipelineStatisticsQuery> CreatePipelineStatisticsQuery();
		std::shared_ptr<TimestampQuery> CreateTimestampQuery(PipelineStageFlags pipelineStage);
		std::shared_ptr<TimerQuery> CreateTimerQuery(PipelineStageFlags pipelineStageStart, PipelineStageFlags pipelineStageEnd);
	  protected:
		IQueryPool(IPrContext &context, QueryType type, uint32_t queryCount);

		QueryType m_type = {};
		uint32_t m_queryCount = 0u;
		uint32_t m_nextQueryId = 0u;
		std::queue<uint32_t> m_freeQueries;
	};
};
