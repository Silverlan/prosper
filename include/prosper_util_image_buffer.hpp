// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_UTIL_IMAGE_BUFFER_HPP__
#define __PROSPER_UTIL_IMAGE_BUFFER_HPP__

#include <mathutil/umath.h>
#include <functional>
#include <optional>
#include <util_image_buffer.hpp>
#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"

namespace Anvil {
	class BaseDevice;
	class Image;
};

namespace prosper::util {
	DLLPROSPER prosper::Format get_vk_format(uimg::Format format);
};

#endif
