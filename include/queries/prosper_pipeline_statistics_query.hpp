/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PIPELINE_STATISTICS_QUERY_HPP__
#define __PROSPER_PIPELINE_STATISTICS_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper
{
	class QueryPool;
	class PipelineStatisticsQuery;
	namespace util
	{
		DLLPROSPER std::shared_ptr<PipelineStatisticsQuery> create_pipeline_statistics_query(QueryPool &queryPool);
	};
	class DLLPROSPER PipelineStatisticsQuery
		: public Query
	{
	public:
#pragma pack(push,1)
		struct Statistics
		{
			uint64_t inputAssemblyVertices;
			uint64_t inputAssemblyPrimitives;
			uint64_t vertexShaderInvocations;
			uint64_t geometryShaderInvocations;
			uint64_t geometryShaderPrimitives;
			uint64_t clippingInvocations;
			uint64_t clippingPrimitives;
			uint64_t fragmentShaderInvocations;
			uint64_t tessellationControlShaderPatches;
			uint64_t tessellationEvaluationShaderInvocations;
			uint64_t computeShaderInvocations;
		};
#pragma pack(pop)

		bool Reset(prosper::ICommandBuffer &cmdBuffer);
		bool RecordBegin(prosper::ICommandBuffer &cmdBuffer);
		bool RecordEnd(prosper::ICommandBuffer &cmdBuffer) const;
		bool QueryResult(Statistics &outStatistics) const;
	private:
		PipelineStatisticsQuery(QueryPool &queryPool,uint32_t queryId);
		bool m_bReset = true;
	private:
		friend std::shared_ptr<PipelineStatisticsQuery> util::create_pipeline_statistics_query(QueryPool &queryPool);
	};
};

#endif
