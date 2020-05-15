/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include <config.h>
#include <wrappers/image.h>
#include "prosper_util_line_shape.hpp"
#include "prosper_util.hpp"

using namespace prosper;

prosper::Format prosper::util::get_line_vertex_format() {return prosper::Format::R32G32_SFloat;}

std::shared_ptr<IBuffer> prosper::util::get_line_vertex_buffer(prosper::IPrContext &context)
{
	static std::weak_ptr<IBuffer> wpBuffer = {};
	std::shared_ptr<IBuffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &vertices = get_line_vertices();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertices.size() *sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		buf = context.CreateBuffer(createInfo,vertices.data());
		buf->SetDebugName("line_vertex_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
const std::vector<Vector2> &prosper::util::get_line_vertices()
{
	static const std::vector<Vector2> vertices = {
		Vector2(-1.f,-1.f),
		Vector2(1.f,1.f)
	};
	return vertices;
}
uint32_t prosper::util::get_line_vertex_count() {return 2u;}
