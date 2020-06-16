/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_command_buffer.hpp"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "vk_context.hpp"
#include <wrappers/command_buffer.h>
#include <wrappers/device.h>
#include <wrappers/physical_device.h>

using namespace prosper;

OcclusionQuery::OcclusionQuery(IQueryPool &queryPool,uint32_t queryId)
	: Query(queryPool,queryId)
{}

bool OcclusionQuery::RecordBegin(prosper::ICommandBuffer &cmdBuffer) const
{
	return cmdBuffer.RecordBeginOcclusionQuery(*this);
}
bool OcclusionQuery::RecordEnd(prosper::ICommandBuffer &cmdBuffer) const
{
	return cmdBuffer.RecordEndOcclusionQuery(*this);
}
