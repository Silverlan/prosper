/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_context.hpp"
#include <wrappers/command_buffer.h>

using namespace prosper;

OcclusionQuery::OcclusionQuery(QueryPool &queryPool,uint32_t queryId)
	: Query(queryPool,queryId)
{}

bool OcclusionQuery::RecordBegin(Anvil::CommandBufferBase &cmdBuffer) const
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return cmdBuffer.record_reset_query_pool(anvPool,m_queryId,1u /* queryCount */) &&
		cmdBuffer.record_begin_query(anvPool,m_queryId,{});
}
bool OcclusionQuery::RecordEnd(Anvil::CommandBufferBase &cmdBuffer) const
{
	auto *pQueryPool = GetPool();
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return cmdBuffer.record_end_query(anvPool,m_queryId);
}

std::shared_ptr<OcclusionQuery> prosper::util::create_occlusion_query(QueryPool &queryPool)
{
	if(queryPool.GetContext().GetDevice().get_physical_device_features().core_vk1_0_features_ptr->pipeline_statistics_query == false)
		return nullptr;
	uint32_t query = 0;
	if(queryPool.RequestQuery(query) == false)
		return nullptr;
	return std::shared_ptr<OcclusionQuery>(new OcclusionQuery(queryPool,query));
}
