// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "buffers/prosper_resizable_buffer.hpp"
#include "buffers/prosper_buffer.hpp"

using namespace prosper;

IResizableBuffer::IResizableBuffer(IBuffer &parent, uint64_t maxTotalSize) : IBuffer {parent.GetContext(), parent.GetCreateInfo(), parent.GetStartOffset(), parent.GetSize()}, m_maxTotalSize {maxTotalSize}, m_baseSize {parent.GetCreateInfo().size} {}

void IResizableBuffer::AddReallocationCallback(const std::function<void()> &fCallback) { m_onReallocCallbacks.push_back(fCallback); }
