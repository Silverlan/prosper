/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "buffers/vk_buffer.hpp"
#include "vk_context.hpp"
#include "vk_command_buffer.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "vk_descriptor_set_group.hpp"
#include <wrappers/command_buffer.h>

prosper::ICommandBuffer::ICommandBuffer(IPrContext &context,prosper::QueueFamilyType queueFamilyType)
	: ContextObject(context),std::enable_shared_from_this<ICommandBuffer>(),m_queueFamilyType{queueFamilyType}
{}

prosper::ICommandBuffer::~ICommandBuffer() {}

bool prosper::ICommandBuffer::IsPrimary() const {return false;}
bool prosper::ICommandBuffer::IsSecondary() const {return false;}
bool prosper::ICommandBuffer::RecordBindDescriptorSets(
	PipelineBindPoint bindPoint,prosper::Shader &shader,PipelineID pipelineIdx,uint32_t firstSet,const std::vector<prosper::IDescriptorSet*> &descSets,const std::vector<uint32_t> dynamicOffsets
)
{
	std::vector<Anvil::DescriptorSet*> anvDescSets {};
	anvDescSets.reserve(descSets.size());
	for(auto *ds : descSets)
		anvDescSets.push_back(&static_cast<prosper::VlkDescriptorSet&>(*ds).GetAnvilDescriptorSet());
	prosper::PipelineID pipelineId;
	return shader.GetPipelineId(pipelineId,pipelineIdx) && dynamic_cast<VlkCommandBuffer&>(*this)->record_bind_descriptor_sets(
		static_cast<Anvil::PipelineBindPoint>(bindPoint),static_cast<VlkContext&>(GetContext()).GetPipelineLayout(shader.IsGraphicsShader(),pipelineId),firstSet,anvDescSets.size(),anvDescSets.data(),
		dynamicOffsets.size(),dynamicOffsets.data()
	);
}
bool prosper::ICommandBuffer::RecordPushConstants(prosper::Shader &shader,PipelineID pipelineIdx,ShaderStageFlags stageFlags,uint32_t offset,uint32_t size,const void *data)
{
	prosper::PipelineID pipelineId;
	return shader.GetPipelineId(pipelineId,pipelineIdx) && dynamic_cast<VlkCommandBuffer&>(*this)->record_push_constants(static_cast<VlkContext&>(GetContext()).GetPipelineLayout(shader.IsGraphicsShader(),pipelineId),static_cast<Anvil::ShaderStageFlagBits>(stageFlags),offset,size,data);
}
bool prosper::ICommandBuffer::RecordBindPipeline(PipelineBindPoint in_pipeline_bind_point,PipelineID in_pipeline_id)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_bind_pipeline(static_cast<Anvil::PipelineBindPoint>(in_pipeline_bind_point),static_cast<Anvil::PipelineID>(in_pipeline_id));
}
bool prosper::ICommandBuffer::RecordSetLineWidth(float lineWidth)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_line_width(lineWidth);
}
bool prosper::ICommandBuffer::RecordBindIndexBuffer(IBuffer &buf,IndexType indexType,DeviceSize offset)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_bind_index_buffer(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,static_cast<Anvil::IndexType>(indexType));
}
bool prosper::ICommandBuffer::RecordBindVertexBuffers(const std::vector<IBuffer*> &buffers,uint32_t startBinding,const std::vector<DeviceSize> &offsets)
{
	std::vector<Anvil::Buffer*> anvBuffers {};
	anvBuffers.reserve(buffers.size());
	for(auto *buf : buffers)
		anvBuffers.push_back(&dynamic_cast<VlkBuffer*>(buf)->GetAnvilBuffer());
	std::vector<DeviceSize> anvOffsets;
	if(offsets.empty())
		anvOffsets.resize(buffers.size(),0);
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_bind_vertex_buffers(startBinding,anvBuffers.size(),anvBuffers.data(),anvOffsets.data());
}
bool prosper::ICommandBuffer::RecordDispatchIndirect(prosper::IBuffer &buffer,DeviceSize size)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_dispatch_indirect(&dynamic_cast<VlkBuffer&>(buffer).GetAnvilBuffer(),size);
}
bool prosper::ICommandBuffer::RecordDraw(uint32_t vertCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw(vertCount,instanceCount,firstVertex,firstInstance);
}
bool prosper::ICommandBuffer::RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,uint32_t firstInstance)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indexed(indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
}
bool prosper::ICommandBuffer::RecordDrawIndexedIndirect(IBuffer &buf,DeviceSize offset,uint32_t drawCount,uint32_t stride)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indexed_indirect(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,drawCount,stride);
}
bool prosper::ICommandBuffer::RecordDrawIndirect(IBuffer &buf,DeviceSize offset,uint32_t count,uint32_t stride)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indirect(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,count,stride);
}
bool prosper::ICommandBuffer::RecordFillBuffer(IBuffer &buf,DeviceSize offset,DeviceSize size,uint32_t data)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_fill_buffer(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,size,data);
}
bool prosper::ICommandBuffer::RecordSetBlendConstants(const std::array<float,4> &blendConstants)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_blend_constants(blendConstants.data());
}
bool prosper::ICommandBuffer::RecordSetDepthBounds(float minDepthBounds,float maxDepthBounds)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_depth_bounds(minDepthBounds,maxDepthBounds);
}
bool prosper::ICommandBuffer::RecordSetStencilCompareMask(StencilFaceFlags faceMask,uint32_t stencilCompareMask)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_compare_mask(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilCompareMask);
}
bool prosper::ICommandBuffer::RecordSetStencilReference(StencilFaceFlags faceMask,uint32_t stencilReference)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_reference(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilReference);
}
bool prosper::ICommandBuffer::RecordSetStencilWriteMask(StencilFaceFlags faceMask,uint32_t stencilWriteMask)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_write_mask(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilWriteMask);
}
prosper::QueueFamilyType prosper::ICommandBuffer::GetQueueFamilyType() const {return m_queueFamilyType;}
