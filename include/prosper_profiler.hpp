/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PROFILER_HPP__
#define __PROSPER_PROFILER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <chrono>

#undef max

namespace prosper
{
	class DLLPROSPER Profiler
		: public ContextObject,
		public std::enable_shared_from_this<Profiler>
	{
	public:
		using Stage = uint32_t;
		enum class Device : uint8_t
		{
			GPU = 0u,
			CPU
		};

		static std::shared_ptr<Profiler> Create(IPrContext &context);
		virtual ~Profiler() override;

		void StartStage(Device device,Stage stage);
		void EndStage(Device device,Stage stage);

		std::chrono::nanoseconds GetResult(Device device,Stage stage) const;
	private:

	};
};

#endif
