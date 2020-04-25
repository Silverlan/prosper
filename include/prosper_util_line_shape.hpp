/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_UTIL_LINE_SHAPE_HPP__
#define __PROSPER_UTIL_LINE_SHAPE_HPP__

#include "prosper_definitions.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <mathutil/uvec.h>

namespace prosper
{
	class Context;
	namespace util
	{
		DLLPROSPER std::shared_ptr<IBuffer> get_line_vertex_buffer(prosper::Context &context);
		DLLPROSPER const std::vector<Vector2> &get_line_vertices();
		DLLPROSPER uint32_t get_line_vertex_count();

		DLLPROSPER Anvil::Format get_line_vertex_format();
	};
};

#endif
