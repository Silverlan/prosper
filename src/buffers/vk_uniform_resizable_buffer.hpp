/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_UNIFORM_RESIZABLE_BUFFER_HPP__
#define __PR_PROSPER_VK_UNIFORM_RESIZABLE_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "vk_buffer.hpp"
#include "buffers/prosper_uniform_resizable_buffer.hpp"

namespace prosper
{
	class DLLPROSPER VkUniformResizableBuffer
		: public IUniformResizableBuffer,
		virtual public VlkBuffer
	{
	public:
		VkUniformResizableBuffer(
			Context &context,IBuffer &buffer,
			uint64_t bufferInstanceSize,
			uint64_t alignedBufferBaseSize,uint64_t maxTotalSize,uint32_t alignment
		);
	};
};

#endif
