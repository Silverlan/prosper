/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_OCCLUSION_QUERY_HPP__
#define __PROSPER_OCCLUSION_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace prosper
{
	class IQueryPool;
	class OcclusionQuery;
	class ICommandBuffer;
	class DLLPROSPER OcclusionQuery
		: public Query
	{
	public:
		bool RecordBegin(prosper::ICommandBuffer &cmdBuffer) const;
		bool RecordEnd(prosper::ICommandBuffer &cmdBuffer) const;
	private:
		OcclusionQuery(IQueryPool &queryPool,uint32_t queryId);
		friend IQueryPool;
	};
};

#endif
