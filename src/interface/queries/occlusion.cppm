// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"
#include <chrono>

export module pragma.prosper:query.occlusion;

export import :query.query;

export namespace prosper {
	class IQueryPool;
	class OcclusionQuery;
	class ICommandBuffer;
	class DLLPROSPER OcclusionQuery : public Query {
	  public:
		bool RecordBegin(prosper::ICommandBuffer &cmdBuffer) const;
		bool RecordEnd(prosper::ICommandBuffer &cmdBuffer) const;
	  private:
		OcclusionQuery(IQueryPool &queryPool, uint32_t queryId);
		friend IQueryPool;
	};
};
