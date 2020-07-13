/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_COMMON_BUFFER_CACHE_HPP__
#define __PROSPER_COMMON_BUFFER_CACHE_HPP__

#include "prosper_definitions.hpp"
#include <mathutil/uvec.h>

namespace prosper
{
	class IPrContext;
	class DLLPROSPER CommonBufferCache
	{
	public:
		CommonBufferCache(IPrContext &context);

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

		std::shared_ptr<IBuffer> m_squareVertexUvBuffer {};
		std::shared_ptr<IBuffer> m_squareVertexBuffer {};
		std::shared_ptr<IBuffer> m_squareUvBuffer {};

		std::shared_ptr<IBuffer> m_lineVertexBuffer {};
	};
};

#endif
