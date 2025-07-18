// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

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
