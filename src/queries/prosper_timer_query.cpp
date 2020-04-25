/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_timer_query.hpp"
#include "queries/prosper_timestamp_query.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

TimerQuery::TimerQuery(const std::shared_ptr<TimestampQuery> &tsQuery0,const std::shared_ptr<TimestampQuery> &tsQuery1)
	: ContextObject(tsQuery0->GetContext()),std::enable_shared_from_this<TimerQuery>(),m_tsQuery0(tsQuery0),m_tsQuery1(tsQuery1)
{}

bool TimerQuery::Reset(prosper::ICommandBuffer &cmdBuffer) const
{
	return m_tsQuery0->Reset(cmdBuffer) && m_tsQuery1->Reset(cmdBuffer);
}
bool TimerQuery::Begin(prosper::ICommandBuffer &cmdBuffer) const {return m_tsQuery0->Write(cmdBuffer);}
bool TimerQuery::End(prosper::ICommandBuffer &cmdBuffer) const {return m_tsQuery1->Write(cmdBuffer);}

bool TimerQuery::QueryResult(std::chrono::nanoseconds &outDuration) const
{
	std::chrono::nanoseconds nsStart,nsEnd;
	if(m_tsQuery0->QueryResult(nsStart) == false || m_tsQuery1->QueryResult(nsEnd) == false)
		return false;
	outDuration = nsEnd -nsStart;
	return true;
}

bool TimerQuery::IsResultAvailable() const {return (m_tsQuery0->IsResultAvailable() && m_tsQuery1->IsResultAvailable()) ? true : false;}
prosper::PipelineStageFlags TimerQuery::GetPipelineStage() const {return m_tsQuery0->GetPipelineStage();}

std::shared_ptr<TimerQuery> prosper::util::create_timer_query(QueryPool &queryPool,prosper::PipelineStageFlags pipelineStage)
{
	auto tsQuery0 = prosper::util::create_timestamp_query(queryPool,pipelineStage);
	if(tsQuery0 == nullptr)
		return nullptr;
	auto tsQuery1 = prosper::util::create_timestamp_query(queryPool,pipelineStage);
	if(tsQuery1 == nullptr)
		return nullptr;
	return std::shared_ptr<TimerQuery>(new TimerQuery(tsQuery0,tsQuery1));
}
