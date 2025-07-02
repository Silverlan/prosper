// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_RENDER_BUFFER_HPP__
#define __PROSPER_RENDER_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"
#include <sharedutils/functioncallback.h>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	class ShaderGraphics;
	class IBuffer;
	class DLLPROSPER IRenderBuffer : public prosper::ContextObject {
	  public:
		virtual ~IRenderBuffer() override;
		const std::vector<std::shared_ptr<prosper::IBuffer>> &GetBuffers() const;
		const std::vector<prosper::DeviceSize> &GetOffsets() const { return m_offsets; }
		const prosper::GraphicsPipelineCreateInfo &GetPipelineCreateInfo() const { return m_pipelineCreateInfo; }
		const IndexBufferInfo *GetIndexBufferInfo() const;
		IndexBufferInfo *GetIndexBufferInfo();
	  protected:
		IRenderBuffer(prosper::IPrContext &context, const prosper::GraphicsPipelineCreateInfo &pipelineCreateInfo, const std::vector<prosper::IBuffer *> &buffers, const std::vector<prosper::DeviceSize> &offsets, const std::optional<IndexBufferInfo> &indexBufferInfo = {});
		virtual void Reload() = 0;

		std::vector<std::shared_ptr<prosper::IBuffer>> m_buffers;
		std::vector<prosper::DeviceSize> m_offsets;
		std::optional<IndexBufferInfo> m_indexBufferInfo {};
		std::vector<CallbackHandle> m_reallocationCallbacks {};
		const prosper::GraphicsPipelineCreateInfo &m_pipelineCreateInfo;
	};
};
#pragma warning(pop)

#endif
