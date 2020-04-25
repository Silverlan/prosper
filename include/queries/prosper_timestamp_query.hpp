/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TIMESTAMP_QUERY_HPP__
#define __PROSPER_TIMESTAMP_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper
{
	class QueryPool;
	class TimestampQuery;
	class ICommandBuffer;
	namespace util
	{
		DLLPROSPER std::shared_ptr<TimestampQuery> create_timestamp_query(QueryPool &queryPool,PipelineStageFlags pipelineStage);
	};
	class DLLPROSPER TimestampQuery
		: public Query
	{
	public:
		PipelineStageFlags GetPipelineStage() const;
		virtual bool Reset(ICommandBuffer &cmdBuffer) override;
		bool Write(ICommandBuffer &cmdBuffer);
		bool QueryResult(std::chrono::nanoseconds &outTimestampValue) const;
	private:
		TimestampQuery(QueryPool &queryPool,uint32_t queryId,PipelineStageFlags pipelineStage);
		bool m_bReset = true;
		PipelineStageFlags m_pipelineStage;
	private:
		friend std::shared_ptr<TimestampQuery> util::create_timestamp_query(QueryPool &queryPool,PipelineStageFlags pipelineStage);
	};
};

#endif
