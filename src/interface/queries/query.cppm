// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"


export module pragma.prosper:query.query;

export import :context_object;
export import std.compat;

#undef max

export namespace prosper {
	class IQueryPool;
	class ICommandBuffer;
	class DLLPROSPER Query : public ContextObject, public std::enable_shared_from_this<Query> {
	  public:
		virtual ~Query() override;

		IQueryPool *GetPool() const;
		// Returns true if the result is available, and false otherwise
		bool QueryResult(uint32_t &r) const;
		// Returns true if the result is available, and false otherwise
		bool QueryResult(uint64_t &r) const;
		bool IsResultAvailable() const;
		uint32_t GetQueryId() const;
		bool Reset(ICommandBuffer &cmdBuffer) const;
		virtual void OnReset(ICommandBuffer &cmdBuffer);
	  protected:
		Query(IQueryPool &queryPool, uint32_t queryId);
		uint32_t m_queryId = std::numeric_limits<uint32_t>::max();
		std::weak_ptr<IQueryPool> m_pool = {};
	};
};
