// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_PROFILER_HPP__
#define __PROSPER_PROFILER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <chrono>

#undef max

namespace prosper {
	class DLLPROSPER Profiler : public ContextObject, public std::enable_shared_from_this<Profiler> {
	  public:
		using Stage = uint32_t;
		enum class Device : uint8_t { GPU = 0u, CPU };

		static std::shared_ptr<Profiler> Create(IPrContext &context);
		virtual ~Profiler() override;

		void StartStage(Device device, Stage stage);
		void EndStage(Device device, Stage stage);

		std::chrono::nanoseconds GetResult(Device device, Stage stage) const;
	  private:
	};
};

#endif
