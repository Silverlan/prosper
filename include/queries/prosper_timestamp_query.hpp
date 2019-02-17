/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TIMESTAMP_QUERY_HPP__
#define __PROSPER_TIMESTAMP_QUERY_HPP__

#include "queries/prosper_query.hpp"

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
		bool Write(Anvil::CommandBufferBase &cmdBuffer);
		// Returns the written timestamp in milliseconds
		bool QueryResult(long double &r) const;
	private:
		TimestampQuery(QueryPool &queryPool,uint32_t queryId,Anvil::PipelineStageFlagBits pipelineStage);
		std::weak_ptr<QueryPool> m_wpQueryPool = {};
		template<class T>
			bool QueryResult(long double &r) const;
		template<class T>
			bool QueryResult(T &r) const;
		virtual bool QueryResult(uint32_t &r) const override;
		virtual bool QueryResult(uint64_t &r) const override;

		Anvil::PipelineStageFlagBits m_pipelineStage;
	private:
		friend std::shared_ptr<TimestampQuery> util::create_timestamp_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage);
	};
};

#endif
