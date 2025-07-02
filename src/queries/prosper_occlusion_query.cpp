// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "prosper_command_buffer.hpp"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_query_pool.hpp"

using namespace prosper;

OcclusionQuery::OcclusionQuery(IQueryPool &queryPool, uint32_t queryId) : Query(queryPool, queryId) {}

bool OcclusionQuery::RecordBegin(prosper::ICommandBuffer &cmdBuffer) const { return cmdBuffer.RecordBeginOcclusionQuery(*this); }
bool OcclusionQuery::RecordEnd(prosper::ICommandBuffer &cmdBuffer) const { return cmdBuffer.RecordEndOcclusionQuery(*this); }
