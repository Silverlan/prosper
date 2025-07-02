// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_COMMON_BUFFER_CACHE_HPP__
#define __PROSPER_COMMON_BUFFER_CACHE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_types.hpp"
#include <mathutil/uvec.h>
#include <memory>

namespace prosper {
	class IBuffer;
	class IRenderBuffer;
	class IPrContext;
	class DLLPROSPER CommonBufferCache {
	  public:
		CommonBufferCache(IPrContext &context);

		std::shared_ptr<IRenderBuffer> GetSquareVertexUvRenderBuffer();
		std::shared_ptr<IBuffer> GetSquareVertexUvBuffer();
		std::shared_ptr<IBuffer> GetSquareVertexBuffer();
		std::shared_ptr<IBuffer> GetSquareUvBuffer();
		static const std::vector<Vector2> &GetSquareVertices();
		static const std::vector<Vector2> &GetSquareUvCoordinates();
		static uint32_t GetSquareVertexCount();

		static prosper::Format GetSquareVertexFormat();
		static prosper::Format GetSquareUvFormat();

		std::shared_ptr<IBuffer> GetLineVertexBuffer();
		static const std::vector<Vector2> &GetLineVertices();
		static uint32_t GetLineVertexCount();

		static prosper::Format GetLineVertexFormat();

		// For internal use only
		void Release();
	  private:
		IPrContext &m_context;

		std::shared_ptr<IRenderBuffer> m_squareVertexUvRenderBuffer {};

		std::shared_ptr<IBuffer> m_squareVertexUvBuffer {};
		std::shared_ptr<IBuffer> m_squareVertexBuffer {};
		std::shared_ptr<IBuffer> m_squareUvBuffer {};

		std::shared_ptr<IBuffer> m_lineVertexBuffer {};
	};
};

#endif
