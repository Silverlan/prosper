/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_QUERY_POOL_HPP__
#define __PROSPER_QUERY_POOL_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <functional>
#include <queue>

namespace Anvil
{
	class QueryPool;
};

namespace prosper
{
	class QueryPool;
	namespace util
	{
		DLLPROSPER std::shared_ptr<QueryPool> create_query_pool(Context &context,vk::QueryType queryType,uint32_t maxConcurrentQueries);
	};
	class DLLPROSPER QueryPool
		: public ContextObject,
		public std::enable_shared_from_this<QueryPool>
	{
	public:
		Anvil::QueryPool &GetAnvilQueryPool() const;
		bool RequestQuery(uint32_t &queryId);
		void FreeQuery(uint32_t queryId);
	protected:
		QueryPool(Context &context,std::unique_ptr<Anvil::QueryPool,std::function<void(Anvil::QueryPool*)>> queryPool,vk::QueryType type);
		std::unique_ptr<Anvil::QueryPool,std::function<void(Anvil::QueryPool*)>> m_queryPool = nullptr;

		vk::QueryType m_type = {};
		uint32_t m_queryCount = 0u;
		uint32_t m_nextQueryId = 0u;
		std::queue<uint32_t> m_freeQueries;
	private:
		friend std::shared_ptr<QueryPool> util::create_query_pool(Context &context,vk::QueryType queryType,uint32_t maxConcurrentQueries);
	};
};

#endif
