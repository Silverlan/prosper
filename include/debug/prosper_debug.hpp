/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DEBUG_HPP__
#define __PROSPER_DEBUG_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include <sstream>

namespace Anvil {class Image;};
namespace prosper
{
	class Context;
	class Image;
	namespace debug
	{
		DLLPROSPER void dump_layers(Context &context,std::stringstream &out);
		DLLPROSPER void dump_extensions(Context &context,std::stringstream &out);
		DLLPROSPER void dump_limits(Context &context,std::stringstream &out);
		DLLPROSPER void dump_features(Context &context,std::stringstream &out);
		DLLPROSPER void dump_image_format_properties(Context &context,std::stringstream &out,Anvil::ImageCreateFlags createFlags=Anvil::ImageCreateFlags{});
		DLLPROSPER void dump_format_properties(Context &context,std::stringstream &out);
		DLLPROSPER void dump_layers(Context &context,const std::string &fileName);
		DLLPROSPER void dump_extensions(Context &context,const std::string &fileName);
		DLLPROSPER void dump_limits(Context &context,const std::string &fileName);
		DLLPROSPER void dump_features(Context &context,const std::string &fileName);
		DLLPROSPER void dump_image_format_properties(Context &context,const std::string &fileName,Anvil::ImageCreateFlags createFlags=Anvil::ImageCreateFlags{});
		DLLPROSPER void dump_format_properties(Context &context,const std::string &fileName);

		DLLPROSPER void add_debug_object_information(std::string &msgValidation);
		DLLPROSPER Image *get_image_from_anvil_image(Anvil::Image &img);
	};
};

#endif
