/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_QUERY_POOL_HPP__
#define __PROSPER_QUERY_POOL_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <functional>
#include <queue>

namespace prosper {
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

#endif
