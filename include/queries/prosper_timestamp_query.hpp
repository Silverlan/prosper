/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TIMESTAMP_QUERY_HPP__
#define __PROSPER_TIMESTAMP_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace Anvil {class CommandBufferBase;};

namespace prosper
{
	class QueryPool;
	class TimestampQuery;
	namespace util
	{
		DLLPROSPER std::shared_ptr<TimestampQuery> create_timestamp_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage);
	};
	class DLLPROSPER TimestampQuery
		: public Query
	{
	public:
		Anvil::PipelineStageFlagBits GetPipelineStage() const;
		virtual bool Reset(Anvil::CommandBufferBase &cmdBuffer) override;
		bool Write(Anvil::CommandBufferBase &cmdBuffer);
		bool QueryResult(std::chrono::nanoseconds &outTimestampValue) const;
	private:
		TimestampQuery(QueryPool &queryPool,uint32_t queryId,Anvil::PipelineStageFlagBits pipelineStage);
		bool m_bReset = true;
		Anvil::PipelineStageFlagBits m_pipelineStage;
	private:
		friend std::shared_ptr<TimestampQuery> util::create_timestamp_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage);
	};
};

#endif
