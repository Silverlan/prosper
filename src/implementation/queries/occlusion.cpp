// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cinttypes>

module pragma.prosper;

import :query.occlusion;

using namespace prosper;

OcclusionQuery::OcclusionQuery(IQueryPool &queryPool, uint32_t queryId) : Query(queryPool, queryId) {}

bool OcclusionQuery::RecordBegin(prosper::ICommandBuffer &cmdBuffer) const { return cmdBuffer.RecordBeginOcclusionQuery(*this); }
bool OcclusionQuery::RecordEnd(prosper::ICommandBuffer &cmdBuffer) const { return cmdBuffer.RecordEndOcclusionQuery(*this); }
