/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_INCLUDES_HPP__
#define __PROSPER_INCLUDES_HPP__

// TODO: This doesn't belong here, move to prosper_vulkan implementation!
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif
//

#include "prosper_enums.hpp"

namespace prosper
{
	namespace util
	{
		const auto PIPELINE_STAGE_SHADER_INPUT_FLAGS = PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::GeometryShaderBit;
	};
};

#endif
