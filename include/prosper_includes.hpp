// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_INCLUDES_HPP__
#define __PROSPER_INCLUDES_HPP__

#include "prosper_enums.hpp"

namespace prosper {
	namespace util {
		constexpr auto PIPELINE_STAGE_SHADER_INPUT_FLAGS = PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::GeometryShaderBit;
	};
};

#endif
