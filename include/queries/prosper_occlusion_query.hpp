/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_OCCLUSION_QUERY_HPP__
#define __PROSPER_OCCLUSION_QUERY_HPP__

#include "queries/prosper_query.hpp"
#include <chrono>

namespace Anvil {class CommandBufferBase;};

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
#pragma pack(push,1)
		struct Statistics
		{
			uint64_t inputAssemblyVertices;
			uint64_t inputAssemblyPrimitives;
			uint64_t vertexShaderInvocations;
			uint64_t geometryShaderInvocations;
			uint64_t geometryShaderPrimitives;
			uint64_t clippingInvocations;
			uint64_t clippingPrimitives;
			uint64_t fragmentShaderInvocations;
			uint64_t tessellationControlShaderPatches;
			uint64_t tessellationEvaluationShaderInvocations;
			uint64_t computeShaderInvocations;
		};
#pragma pack(pop)

		bool RecordBegin(Anvil::CommandBufferBase &cmdBuffer) const;
		bool RecordEnd(Anvil::CommandBufferBase &cmdBuffer) const;
	private:
		OcclusionQuery(QueryPool &queryPool,uint32_t queryId);
	private:
		friend std::shared_ptr<OcclusionQuery> util::create_occlusion_query(QueryPool &queryPool);
	};
};

#endif
