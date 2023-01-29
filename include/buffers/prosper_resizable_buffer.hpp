/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_RESIZABLE_BUFFER_HPP__
#define __PROSPER_RESIZABLE_BUFFER_HPP__

#include "buffers/prosper_buffer.hpp"
#include "prosper_util.hpp"

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	class DLLPROSPER IResizableBuffer : virtual public IBuffer {
	  public:
		IResizableBuffer(IBuffer &parent, uint64_t maxTotalSize);

		void AddReallocationCallback(const std::function<void()> &fCallback);
	  protected:
		virtual void MoveInternalBuffer(IBuffer &other) = 0;

		uint64_t m_baseSize = 0ull; // Un-aligned size of m_buffer
		uint64_t m_maxTotalSize = 0ull;

		std::vector<std::function<void()>> m_onReallocCallbacks;
	};
};
#pragma warning(pop)

#endif
