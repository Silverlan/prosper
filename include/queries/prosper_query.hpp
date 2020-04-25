/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_QUERY_HPP__
#define __PROSPER_QUERY_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"

#undef max

namespace prosper
{
	class QueryPool;
	class DLLPROSPER Query
		: public ContextObject,
		public std::enable_shared_from_this<Query>
	{
	public:
		virtual ~Query() override;

		QueryPool *GetPool() const;
		// Returns true if the result is available, and false otherwise
		virtual bool QueryResult(uint32_t &r) const;
		// Returns true if the result is available, and false otherwise
		virtual bool QueryResult(uint64_t &r) const;
		virtual bool Reset(ICommandBuffer &cmdBuffer);
		bool IsResultAvailable() const;
		template<class T,typename TBaseType=T>
			bool QueryResult(T &r,QueryResultFlags resultFlags) const;
	protected:
		Query(QueryPool &queryPool,uint32_t queryId);
		uint32_t m_queryId = std::numeric_limits<uint32_t>::max();
		std::weak_ptr<QueryPool> m_pool = {};
	};
};

template<class T,typename TBaseType>
	bool prosper::Query::QueryResult(T &outResult,QueryResultFlags resultFlags) const
{
	if(m_pool.expired())
		return false;
#pragma pack(push,1)
	struct ResultData
	{
		T data;
		TBaseType availability;
	};
#pragma pack(pop)
	auto bAllQueryResultsRetrieved = false;
	ResultData resultData;
	auto bSuccess = m_pool.lock()->GetAnvilQueryPool().get_query_pool_results(m_queryId,1u,static_cast<Anvil::QueryResultFlagBits>(resultFlags | prosper::QueryResultFlags::WithAvailabilityBit),reinterpret_cast<TBaseType*>(&resultData),&bAllQueryResultsRetrieved);
	if(bSuccess == false || bAllQueryResultsRetrieved == false || resultData.availability == 0)
		return false;
	outResult = resultData.data;
	return true;
}

#endif
