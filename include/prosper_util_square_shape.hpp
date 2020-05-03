/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_UTIL_SQUARE_SHAPE_HPP__
#define __PROSPER_UTIL_SQUARE_SHAPE_HPP__

#include "prosper_definitions.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <mathutil/uvec.h>

namespace prosper
{
	class IBuffer;
	class IPrContext;
	namespace util
	{
		DLLPROSPER std::shared_ptr<IBuffer> get_square_vertex_uv_buffer(prosper::IPrContext &context);
		DLLPROSPER std::shared_ptr<IBuffer> get_square_vertex_buffer(prosper::IPrContext &context);
		DLLPROSPER std::shared_ptr<IBuffer> get_square_uv_buffer(prosper::IPrContext &context);
		DLLPROSPER const std::vector<Vector2> &get_square_vertices();
		DLLPROSPER const std::vector<Vector2> &get_square_uv_coordinates();
		DLLPROSPER uint32_t get_square_vertex_count();

		DLLPROSPER prosper::Format get_square_vertex_format();
		DLLPROSPER prosper::Format get_square_uv_format();
	};
};

#endif
