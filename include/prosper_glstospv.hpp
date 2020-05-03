/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_GLSTOSPV_HPP__
#define __PROSPER_GLSTOSPV_HPP__

#include "prosper_definitions.hpp"
#include <cinttypes>
#include <vulkan/vulkan.hpp>
#include <string>

namespace prosper
{
	class IPrContext;
	enum class Vendor : uint32_t;
	DLLPROSPER void dump_parsed_shader(IPrContext &context,uint32_t stage,const std::string &shaderFile,const std::string &fileName);
	DLLPROSPER bool glsl_to_spv(IPrContext &context,uint32_t stage,const std::string &fileName,std::vector<unsigned int> &spirv,std::string *infoLog,std::string *debugInfoLog,bool bReload);
};

#endif
