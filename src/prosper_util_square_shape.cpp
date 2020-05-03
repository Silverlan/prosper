/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include <config.h>
#include <wrappers/image.h>
#include "prosper_util_square_shape.hpp"
#include "prosper_util.hpp"

using namespace prosper;

prosper::Format prosper::util::get_square_vertex_format() {return prosper::Format::R32G32_SFloat;}
prosper::Format prosper::util::get_square_uv_format() {return prosper::Format::R32G32_SFloat;}

std::shared_ptr<IBuffer> prosper::util::get_square_vertex_uv_buffer(prosper::IPrContext &context)
{
#pragma pack(push,1)
	struct VertexData
	{
		Vector2 position;
		Vector2 uv;
	};
#pragma pack(pop)
	static std::weak_ptr<IBuffer> wpBuffer = {};
	std::shared_ptr<IBuffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &vertices = get_square_vertices();
		auto &uvs = get_square_uv_coordinates();
		std::vector<VertexData> vertexData {};
		auto numVerts = vertices.size();
		vertexData.reserve(numVerts);
		for(auto i=decltype(numVerts){0};i<numVerts;++i)
		{
			auto &v = vertices.at(i);
			auto &uv = uvs.at(i);
			vertexData.push_back({v,uv});
		}
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertexData.size() *sizeof(vertexData.front());
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		buf = context.CreateBuffer(createInfo,vertexData.data());
		buf->SetDebugName("square_vertex_uv_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
std::shared_ptr<IBuffer> prosper::util::get_square_vertex_buffer(prosper::IPrContext &context)
{
	static std::weak_ptr<IBuffer> wpBuffer = {};
	std::shared_ptr<IBuffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &vertices = get_square_vertices();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertices.size() *sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		buf = context.CreateBuffer(createInfo,vertices.data());
		buf->SetDebugName("square_vertex_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
std::shared_ptr<IBuffer> prosper::util::get_square_uv_buffer(prosper::IPrContext &context)
{
	static std::weak_ptr<IBuffer> wpBuffer = {};
	std::shared_ptr<IBuffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &uvCoordinates = get_square_uv_coordinates();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = uvCoordinates.size() *sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		buf = context.CreateBuffer(createInfo,uvCoordinates.data());
		buf->SetDebugName("square_uv_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
const std::vector<Vector2> &prosper::util::get_square_vertices()
{
	static const std::vector<Vector2> vertices = {
		Vector2(1.f,1.f),
		Vector2(-1.f,-1.f),
		Vector2(-1.f,1.f),
		Vector2(-1.f,-1.f),
		Vector2(1.f,1.f),
		Vector2(1.f,-1.f)
		
	};
	return vertices;
}
const std::vector<Vector2> &prosper::util::get_square_uv_coordinates()
{
	static const std::vector<Vector2> uvCoordinates = {
		Vector2(1.f,1.f),
		Vector2(0.f,0.f),
		Vector2(0.f,1.f),
		Vector2(0.f,0.f),
		Vector2(1.f,1.f),
		Vector2(1.f,0.f)
	};
	return uvCoordinates;
}
uint32_t prosper::util::get_square_vertex_count() {return 6u;}
