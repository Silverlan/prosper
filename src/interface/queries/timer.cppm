// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:query.timer;

export import :context_object;
export import :types;

export namespace prosper {
	class IQueryPool;
	class TimerQuery;
	class TimestampQuery;
	class CommandBuffer;
	class DLLPROSPER TimerQuery : public ContextObject, public std::enable_shared_from_this<TimerQuery> {
	  public:
		PipelineStageFlags GetPipelineStage() const;
		bool Reset(ICommandBuffer &cmdBuffer) const;
		bool Begin(ICommandBuffer &cmdBuffer) const;
		bool End(ICommandBuffer &cmdBuffer) const;
		bool QueryResult(std::chrono::nanoseconds &outDuration) const;
		bool IsResultAvailable() const;
	  private:
		TimerQuery(const std::shared_ptr<TimestampQuery> &tsQuery0, const std::shared_ptr<TimestampQuery> &tsQuery1);
		std::shared_ptr<TimestampQuery> m_tsQuery0 = nullptr;
		std::shared_ptr<TimestampQuery> m_tsQuery1 = nullptr;
		friend IQueryPool;
	};
};
