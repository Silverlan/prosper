// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_TIMESTAMP_QUERY_HPP__
#define __PROSPER_TIMESTAMP_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper {
	class IQueryPool;
	class TimestampQuery;
	class ICommandBuffer;
	class DLLPROSPER TimestampQuery : public Query {
	  public:
		enum class State : uint8_t { Initial = 0, Reset, Set };
		PipelineStageFlags GetPipelineStage() const;
		bool Write(ICommandBuffer &cmdBuffer);
		bool QueryResult(std::chrono::nanoseconds &outTimestampValue) const;
		bool IsReset() const;
		State GetState() const { return m_state; }
	  private:
		TimestampQuery(IQueryPool &queryPool, uint32_t queryId, PipelineStageFlags pipelineStage);
		virtual void OnReset(ICommandBuffer &cmdBuffer) override;
		State m_state = State::Initial;
		PipelineStageFlags m_pipelineStage;
	  private:
		friend IQueryPool;
	};
};

#endif
