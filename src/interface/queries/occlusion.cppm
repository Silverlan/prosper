// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:query.occlusion;

export import :query.query;

export namespace prosper {
	class IQueryPool;
	class OcclusionQuery;
	class ICommandBuffer;
	class DLLPROSPER OcclusionQuery : public Query {
	  public:
		bool RecordBegin(ICommandBuffer &cmdBuffer) const;
		bool RecordEnd(ICommandBuffer &cmdBuffer) const;
	  private:
		OcclusionQuery(IQueryPool &queryPool, uint32_t queryId);
		friend IQueryPool;
	};
};
