// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_PIPELINE_STATISTICS_QUERY_HPP__
#define __PROSPER_PIPELINE_STATISTICS_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper {
	class IQueryPool;
	class PipelineStatisticsQuery;
#pragma pack(push, 1)
	struct DLLPROSPER PipelineStatistics {
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
	class DLLPROSPER PipelineStatisticsQuery : public Query {
	  public:
		bool RecordBegin(prosper::ICommandBuffer &cmdBuffer);
		bool RecordEnd(prosper::ICommandBuffer &cmdBuffer) const;
		bool QueryResult(PipelineStatistics &outStatistics) const;
		bool IsReset() const;
	  private:
		PipelineStatisticsQuery(IQueryPool &queryPool, uint32_t queryId);
		virtual void OnReset(prosper::ICommandBuffer &cmdBuffer) override;
		bool m_bReset = true;
	  private:
		friend IQueryPool;
	};
};

#endif
