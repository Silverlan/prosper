/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include <config.h>
#include <wrappers/image.h>
#include "prosper_util_square_shape.hpp"
#include "prosper_util.hpp"

using namespace prosper;

Anvil::Format prosper::util::get_square_vertex_format() {return Anvil::Format::R32G32_SFLOAT;}
Anvil::Format prosper::util::get_square_uv_format() {return Anvil::Format::R32G32_SFLOAT;}

std::shared_ptr<Buffer> prosper::util::get_square_vertex_uv_buffer(Anvil::BaseDevice &dev)
{
#pragma pack(push,1)
	struct VertexData
	{
		Vector2 position;
		Vector2 uv;
	};
#pragma pack(pop)
	static std::weak_ptr<Buffer> wpBuffer = {};
	std::shared_ptr<Buffer> buf = nullptr;
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
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
		createInfo.size = vertexData.size() *sizeof(vertexData.front());
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		buf = prosper::util::create_buffer(dev,createInfo,vertexData.data());
		buf->SetDebugName("square_vertex_uv_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
std::shared_ptr<Buffer> prosper::util::get_square_vertex_buffer(Anvil::BaseDevice &dev)
{
	static std::weak_ptr<Buffer> wpBuffer = {};
	std::shared_ptr<Buffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &vertices = get_square_vertices();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
		createInfo.size = vertices.size() *sizeof(Vector2);
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		buf = prosper::util::create_buffer(dev,createInfo,vertices.data());
		buf->SetDebugName("square_vertex_buf");
		wpBuffer = buf;
	}
	else
		buf = wpBuffer.lock();
	return buf;
}
std::shared_ptr<Buffer> prosper::util::get_square_uv_buffer(Anvil::BaseDevice &dev)
{
	static std::weak_ptr<Buffer> wpBuffer = {};
	std::shared_ptr<Buffer> buf = nullptr;
	if(wpBuffer.expired())
	{
		auto &uvCoordinates = get_square_uv_coordinates();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
		createInfo.size = uvCoordinates.size() *sizeof(Vector2);
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
		buf = prosper::util::create_buffer(dev,createInfo,uvCoordinates.data());
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
