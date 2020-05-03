/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_OCCLUSION_QUERY_HPP__
#define __PROSPER_OCCLUSION_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper
{
	class QueryPool;
	class OcclusionQuery;
	namespace util
	{
		DLLPROSPER std::shared_ptr<OcclusionQuery> create_occlusion_query(QueryPool &queryPool);
	};
	class DLLPROSPER OcclusionQuery
		: public Query
	{
	public:
		bool RecordBegin(Anvil::CommandBufferBase &cmdBuffer) const;
		bool RecordEnd(Anvil::CommandBufferBase &cmdBuffer) const;
	private:
		OcclusionQuery(QueryPool &queryPool,uint32_t queryId);
	private:
		friend std::shared_ptr<OcclusionQuery> util::create_occlusion_query(QueryPool &queryPool);
	};
};

#endif
