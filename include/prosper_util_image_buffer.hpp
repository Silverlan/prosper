/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_UTIL_IMAGE_BUFFER_HPP__
#define __PROSPER_UTIL_IMAGE_BUFFER_HPP__

#include <mathutil/umath.h>
#include <functional>
#include <optional>
#include <util_image_buffer.hpp>
#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"

namespace Anvil
{
	class BaseDevice;
	class Image;
};

namespace prosper::util
{
	DLLPROSPER void initialize_image(Anvil::BaseDevice &dev,const uimg::ImageBuffer &imgSrc,Anvil::Image &img);
	DLLPROSPER Anvil::Format get_vk_format(uimg::ImageBuffer::Format format);
};

#endif
