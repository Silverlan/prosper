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
		virtual ~Query()=default;

		QueryPool *GetPool() const;
		// Returns true if the result is available, and false otherwise
		virtual bool QueryResult(uint32_t &r) const;
		// Returns true if the result is available, and false otherwise
		virtual bool QueryResult(uint64_t &r) const;
		bool IsResultAvailable() const;
		template<class T>
			bool QueryResult(T &r,Anvil::QueryResultFlags resultFlags) const;
	protected:
		Query(QueryPool &queryPool,uint32_t queryId);
		uint32_t m_queryId = std::numeric_limits<uint32_t>::max();
		std::weak_ptr<QueryPool> m_pool = {};
	};
};

#endif
