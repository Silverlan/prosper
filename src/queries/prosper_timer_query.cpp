/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "queries/prosper_timer_query.hpp"
#include "queries/prosper_timestamp_query.hpp"
#include "prosper_command_buffer.hpp"

using namespace prosper;

TimerQuery::TimerQuery(const std::shared_ptr<prosper::CommandBuffer> &cmdBuffer,const std::shared_ptr<TimestampQuery> &tsQuery0,const std::shared_ptr<TimestampQuery> &tsQuery1)
	: ContextObject(tsQuery0->GetContext()),std::enable_shared_from_this<TimerQuery>(),m_tsQuery0(tsQuery0),m_tsQuery1(tsQuery1),m_cmdBuffer(cmdBuffer)
{}

bool TimerQuery::Begin() const {return m_tsQuery0->Write(**m_cmdBuffer);}
bool TimerQuery::End() const {return m_tsQuery1->Write(**m_cmdBuffer);}

bool TimerQuery::QueryResult(long double &r) const
{
	auto start = 0.L;
	auto end = 0.L;
	if(m_tsQuery0->QueryResult(start) == false)
		return false;
	if(m_tsQuery1->QueryResult(end) == false)
		return false;
	r = end -start;
	return true;
}

bool TimerQuery::IsResultAvailable() const {return (m_tsQuery0->IsResultAvailable() && m_tsQuery1->IsResultAvailable()) ? true : false;}
Anvil::PipelineStageFlagBits TimerQuery::GetPipelineStage() const {return m_tsQuery0->GetPipelineStage();}

std::shared_ptr<TimerQuery> prosper::util::create_timer_query(QueryPool &queryPool,const std::shared_ptr<prosper::CommandBuffer> &cmdBuffer,Anvil::PipelineStageFlagBits pipelineStage)
{
	auto tsQuery0 = prosper::util::create_timestamp_query(queryPool,pipelineStage);
	if(tsQuery0 == nullptr)
		return nullptr;
	auto tsQuery1 = prosper::util::create_timestamp_query(queryPool,pipelineStage);
	if(tsQuery1 == nullptr)
		return nullptr;
	return std::shared_ptr<TimerQuery>(new TimerQuery(cmdBuffer,tsQuery0,tsQuery1));
}
