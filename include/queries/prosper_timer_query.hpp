/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TIMER_QUERY_HPP__
#define __PROSPER_TIMER_QUERY_HPP__

#include "queries/prosper_query.hpp"

namespace Anvil {class CommandBufferBase;};

namespace prosper
{
	class QueryPool;
	class TimerQuery;
	class TimestampQuery;
	class CommandBuffer;
	namespace util
	{
		DLLPROSPER std::shared_ptr<TimerQuery> create_timer_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage);
	};
	class DLLPROSPER TimerQuery
		: public ContextObject,public std::enable_shared_from_this<TimerQuery>
	{
	public:
		Anvil::PipelineStageFlagBits GetPipelineStage() const;
		bool Reset(Anvil::CommandBufferBase &cmdBuffer) const;
		bool Begin(Anvil::CommandBufferBase &cmdBuffer) const;
		bool End(Anvil::CommandBufferBase &cmdBuffer) const;
		bool QueryResult(std::chrono::nanoseconds &outDuration) const;
		bool IsResultAvailable() const;
	private:
		TimerQuery(const std::shared_ptr<TimestampQuery> &tsQuery0,const std::shared_ptr<TimestampQuery> &tsQuery1);
		std::shared_ptr<TimestampQuery> m_tsQuery0 = nullptr;
		std::shared_ptr<TimestampQuery> m_tsQuery1 = nullptr;
	private:
		friend std::shared_ptr<TimerQuery> util::create_timer_query(QueryPool &queryPool,Anvil::PipelineStageFlagBits pipelineStage);
	};
};

#endif
