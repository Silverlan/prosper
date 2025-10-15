// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>

module pragma.prosper;

import :common_buffer_cache;
import :shader_system.pipeline_create_info;
import :shader_system.shaders.copy_image;

prosper::CommonBufferCache::CommonBufferCache(IPrContext &context) : m_context {context} {}
void prosper::CommonBufferCache::Release()
{
	m_squareVertexUvRenderBuffer = nullptr;
	m_squareVertexUvBuffer = nullptr;
	m_squareVertexBuffer = nullptr;
	m_squareUvBuffer = nullptr;

	m_lineVertexBuffer = nullptr;
}
prosper::Format prosper::CommonBufferCache::GetSquareVertexFormat() { return prosper::Format::R32G32_SFloat; }
prosper::Format prosper::CommonBufferCache::GetSquareUvFormat() { return prosper::Format::R32G32_SFloat; }

std::shared_ptr<prosper::IBuffer> prosper::CommonBufferCache::GetSquareVertexUvBuffer()
{
#pragma pack(push, 1)
	struct VertexData {
		Vector2 position;
		Vector2 uv;
	};
#pragma pack(pop)
	if(m_squareVertexUvBuffer == nullptr) {
		auto &vertices = GetSquareVertices();
		auto &uvs = GetSquareUvCoordinates();
		std::vector<VertexData> vertexData {};
		auto numVerts = vertices.size();
		vertexData.reserve(numVerts);
		for(auto i = decltype(numVerts) {0}; i < numVerts; ++i) {
			auto &v = vertices.at(i);
			auto &uv = uvs.at(i);
			vertexData.push_back({v, uv});
		}
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertexData.size() * sizeof(vertexData.front());
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		auto buf = m_context.CreateBuffer(createInfo, vertexData.data());
		buf->SetDebugName("square_vertex_uv_buf");
		m_squareVertexUvBuffer = buf;
	}
	return m_squareVertexUvBuffer;
}
std::shared_ptr<prosper::IRenderBuffer> prosper::CommonBufferCache::GetSquareVertexUvRenderBuffer()
{
	if(m_squareVertexUvRenderBuffer == nullptr) {
		auto vertexBuf = GetSquareVertexBuffer();
		auto uvBuf = GetSquareUvBuffer();
		// The shader here doesn't matter, but has to be derived from ShaderBaseImageProcessing and have its same vertex buffer structure
		auto *shaderCpy = static_cast<prosper::ShaderCopyImage *>(m_context.GetShader("copy_image").get());
		if(shaderCpy)
			m_squareVertexUvRenderBuffer = m_context.CreateRenderBuffer(*static_cast<prosper::GraphicsPipelineCreateInfo *>(shaderCpy->GetPipelineCreateInfo(0u)), {vertexBuf.get(), uvBuf.get()});
	}
	return m_squareVertexUvRenderBuffer;
}
std::shared_ptr<prosper::IBuffer> prosper::CommonBufferCache::GetSquareVertexBuffer()
{
	if(m_squareVertexBuffer == nullptr) {
		auto &vertices = GetSquareVertices();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertices.size() * sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		auto buf = m_context.CreateBuffer(createInfo, vertices.data());
		buf->SetDebugName("square_vertex_buf");
		m_squareVertexBuffer = buf;
	}
	return m_squareVertexBuffer;
}
std::shared_ptr<prosper::IBuffer> prosper::CommonBufferCache::GetSquareUvBuffer()
{
	if(m_squareUvBuffer == nullptr) {
		auto &uvCoordinates = GetSquareUvCoordinates();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = uvCoordinates.size() * sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		auto buf = m_context.CreateBuffer(createInfo, uvCoordinates.data());
		buf->SetDebugName("square_uv_buf");
		m_squareUvBuffer = buf;
	}
	return m_squareUvBuffer;
}
const std::vector<Vector2> &prosper::CommonBufferCache::GetSquareVertices()
{
	static const std::vector<Vector2> vertices = {Vector2(1.f, 1.f), Vector2(-1.f, -1.f), Vector2(-1.f, 1.f), Vector2(-1.f, -1.f), Vector2(1.f, 1.f), Vector2(1.f, -1.f)

	};
	return vertices;
}
const std::vector<Vector2> &prosper::CommonBufferCache::GetSquareUvCoordinates()
{
	static const std::vector<Vector2> uvCoordinates = {Vector2(1.f, 1.f), Vector2(0.f, 0.f), Vector2(0.f, 1.f), Vector2(0.f, 0.f), Vector2(1.f, 1.f), Vector2(1.f, 0.f)};
	return uvCoordinates;
}
uint32_t prosper::CommonBufferCache::GetSquareVertexCount() { return 6u; }

/////////////

prosper::Format prosper::CommonBufferCache::GetLineVertexFormat() { return prosper::Format::R32G32_SFloat; }

std::shared_ptr<prosper::IBuffer> prosper::CommonBufferCache::GetLineVertexBuffer()
{
	if(m_lineVertexBuffer == nullptr) {
		auto &vertices = GetLineVertices();
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = BufferUsageFlags::VertexBufferBit;
		createInfo.size = vertices.size() * sizeof(Vector2);
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		auto buf = m_context.CreateBuffer(createInfo, vertices.data());
		buf->SetDebugName("line_vertex_buf");
		m_lineVertexBuffer = buf;
	}
	return m_lineVertexBuffer;
}
const std::vector<Vector2> &prosper::CommonBufferCache::GetLineVertices()
{
	static const std::vector<Vector2> vertices = {Vector2(-1.f, -1.f), Vector2(1.f, 1.f)};
	return vertices;
}
uint32_t prosper::CommonBufferCache::GetLineVertexCount() { return 2u; }
