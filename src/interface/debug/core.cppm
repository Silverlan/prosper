// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:debug.core;

export import :structs;

export {
	namespace prosper {
		class IPrContext;
		class ICommandBuffer;
	};

	namespace prosper::debug {
		struct DLLPROSPER ImageLayouts {
			std::vector<ImageLayout> mipmapLayouts;
		};
		struct DLLPROSPER ImageLayoutInfo {
			std::vector<ImageLayouts> layerLayouts;
		};
		DLLPROSPER void enable_debug_recorded_image_layout(bool b);
		DLLPROSPER bool is_debug_recorded_image_layout_enabled();
		DLLPROSPER void set_debug_validation_callback(const std::function<void(DebugReportObjectTypeEXT, const std::string &)> &callback);
		DLLPROSPER void exec_debug_validation_callback(IPrContext &context, DebugReportObjectTypeEXT objType, const std::string &msg);
		DLLPROSPER bool get_last_recorded_image_layout(ICommandBuffer &cmdBuffer, IImage &img, ImageLayout &layout, uint32_t baseLayer = 0u, uint32_t baseMipmap = 0u);
		DLLPROSPER void set_last_recorded_image_layout(ICommandBuffer &cmdBuffer, IImage &img, ImageLayout layout, uint32_t baseLayer = 0u, uint32_t layerCount = std::numeric_limits<uint32_t>::max(), uint32_t baseMipmap = 0u,
		  uint32_t mipmapLevels = std::numeric_limits<uint32_t>::max());
	};
}
