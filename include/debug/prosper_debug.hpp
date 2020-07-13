/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_definitions.hpp"
#include "prosper_enums.hpp"

namespace prosper
{
	class IPrContext;
	class ICommandBuffer;
};

namespace prosper::debug
{
	struct DLLPROSPER ImageLayouts
	{
		std::vector<prosper::ImageLayout> mipmapLayouts;
	};
	struct DLLPROSPER ImageLayoutInfo
	{
		std::vector<ImageLayouts> layerLayouts;
	};
	DLLPROSPER void enable_debug_recorded_image_layout(bool b);
	DLLPROSPER bool is_debug_recorded_image_layout_enabled();
	DLLPROSPER void set_debug_validation_callback(const std::function<void(DebugReportObjectTypeEXT,const std::string&)> &callback);
	DLLPROSPER void exec_debug_validation_callback(IPrContext &context,DebugReportObjectTypeEXT objType,const std::string &msg);
	DLLPROSPER bool get_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout &layout,uint32_t baseLayer=0u,uint32_t baseMipmap=0u);
	DLLPROSPER void set_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout layout,uint32_t baseLayer=0u,uint32_t layerCount=std::numeric_limits<uint32_t>::max(),uint32_t baseMipmap=0u,uint32_t mipmapLevels=std::numeric_limits<uint32_t>::max());
};
