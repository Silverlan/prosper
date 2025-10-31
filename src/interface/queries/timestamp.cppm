// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:query.timestamp;

export import :enums;
export import :query.query;

export namespace prosper {
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
