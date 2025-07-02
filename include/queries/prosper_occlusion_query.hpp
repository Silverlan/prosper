// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_OCCLUSION_QUERY_HPP__
#define __PROSPER_OCCLUSION_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper {
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

#endif
