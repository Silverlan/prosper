/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "queries/prosper_query.hpp"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/prosper_timer_query.hpp"
#include "queries/prosper_timestamp_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"

prosper::ICommandBuffer::ICommandBuffer(IPrContext &context,prosper::QueueFamilyType queueFamilyType)
	: ContextObject(context),std::enable_shared_from_this<ICommandBuffer>(),m_queueFamilyType{queueFamilyType}
{}

prosper::ICommandBuffer::~ICommandBuffer() {}

bool prosper::ICommandBuffer::IsPrimary() const {return false;}
bool prosper::ICommandBuffer::IsSecondary() const {return false;}
prosper::QueueFamilyType prosper::ICommandBuffer::GetQueueFamilyType() const {return m_queueFamilyType;}
