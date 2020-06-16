/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_command_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "shader/prosper_shader.hpp"
#include "buffers/vk_buffer.hpp"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "queries/prosper_timestamp_query.hpp"
#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/vk_query_pool.hpp"
#include "vk_context.hpp"
#include "vk_framebuffer.hpp"
#include "vk_render_pass.hpp"
#include "vk_descriptor_set_group.hpp"
#include <wrappers/buffer.h>
#include <wrappers/memory_block.h>
#include <wrappers/command_buffer.h>
#include <wrappers/framebuffer.h>
#include <misc/buffer_create_info.h>

Anvil::CommandBufferBase &prosper::VlkCommandBuffer::GetAnvilCommandBuffer() const {return *m_cmdBuffer;}
Anvil::CommandBufferBase &prosper::VlkCommandBuffer::operator*() {return *m_cmdBuffer;}
const Anvil::CommandBufferBase &prosper::VlkCommandBuffer::operator*() const {return const_cast<VlkCommandBuffer*>(this)->operator*();}
Anvil::CommandBufferBase *prosper::VlkCommandBuffer::operator->() {return m_cmdBuffer.get();}
const Anvil::CommandBufferBase *prosper::VlkCommandBuffer::operator->() const {return const_cast<VlkCommandBuffer*>(this)->operator->();}

bool prosper::VlkCommandBuffer::RecordBindDescriptorSets(
	PipelineBindPoint bindPoint,prosper::Shader &shader,PipelineID pipelineIdx,uint32_t firstSet,const std::vector<prosper::IDescriptorSet*> &descSets,const std::vector<uint32_t> dynamicOffsets
)
{
	std::vector<Anvil::DescriptorSet*> anvDescSets {};
	anvDescSets.reserve(descSets.size());
	for(auto *ds : descSets)
		anvDescSets.push_back(&static_cast<prosper::VlkDescriptorSet&>(*ds).GetAnvilDescriptorSet());
	prosper::PipelineID pipelineId;
	return shader.GetPipelineId(pipelineId,pipelineIdx) && (*this)->record_bind_descriptor_sets(
		static_cast<Anvil::PipelineBindPoint>(bindPoint),static_cast<VlkContext&>(GetContext()).GetPipelineLayout(shader.IsGraphicsShader(),pipelineId),firstSet,anvDescSets.size(),anvDescSets.data(),
		dynamicOffsets.size(),dynamicOffsets.data()
	);
}
bool prosper::VlkCommandBuffer::RecordPushConstants(prosper::Shader &shader,PipelineID pipelineIdx,ShaderStageFlags stageFlags,uint32_t offset,uint32_t size,const void *data)
{
	prosper::PipelineID pipelineId;
	return shader.GetPipelineId(pipelineId,pipelineIdx) && (*this)->record_push_constants(static_cast<VlkContext&>(GetContext()).GetPipelineLayout(shader.IsGraphicsShader(),pipelineId),static_cast<Anvil::ShaderStageFlagBits>(stageFlags),offset,size,data);
}
bool prosper::VlkCommandBuffer::RecordBindPipeline(PipelineBindPoint in_pipeline_bind_point,PipelineID in_pipeline_id)
{
	return (*this)->record_bind_pipeline(static_cast<Anvil::PipelineBindPoint>(in_pipeline_bind_point),static_cast<Anvil::PipelineID>(in_pipeline_id));
}
bool prosper::VlkCommandBuffer::RecordSetLineWidth(float lineWidth)
{
	return (*this)->record_set_line_width(lineWidth);
}
bool prosper::VlkCommandBuffer::RecordBindIndexBuffer(IBuffer &buf,IndexType indexType,DeviceSize offset)
{
	return (*this)->record_bind_index_buffer(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,static_cast<Anvil::IndexType>(indexType));
}
bool prosper::VlkCommandBuffer::RecordBindVertexBuffers(
	const prosper::ShaderGraphics &shader,const std::vector<IBuffer*> &buffers,uint32_t startBinding,const std::vector<DeviceSize> &offsets
)
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
bool prosper::VlkCommandBuffer::RecordDispatchIndirect(prosper::IBuffer &buffer,DeviceSize size)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_dispatch_indirect(&dynamic_cast<VlkBuffer&>(buffer).GetAnvilBuffer(),size);
}
bool prosper::VlkCommandBuffer::RecordDraw(uint32_t vertCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw(vertCount,instanceCount,firstVertex,firstInstance);
}
bool prosper::VlkCommandBuffer::RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,uint32_t firstInstance)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indexed(indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
}
bool prosper::VlkCommandBuffer::RecordDrawIndexedIndirect(IBuffer &buf,DeviceSize offset,uint32_t drawCount,uint32_t stride)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indexed_indirect(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,drawCount,stride);
}
bool prosper::VlkCommandBuffer::RecordDrawIndirect(IBuffer &buf,DeviceSize offset,uint32_t count,uint32_t stride)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_draw_indirect(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,count,stride);
}
bool prosper::VlkCommandBuffer::RecordFillBuffer(IBuffer &buf,DeviceSize offset,DeviceSize size,uint32_t data)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_fill_buffer(&dynamic_cast<VlkBuffer&>(buf).GetAnvilBuffer(),offset,size,data);
}
bool prosper::VlkCommandBuffer::RecordSetBlendConstants(const std::array<float,4> &blendConstants)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_blend_constants(blendConstants.data());
}
bool prosper::VlkCommandBuffer::RecordSetDepthBounds(float minDepthBounds,float maxDepthBounds)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_depth_bounds(minDepthBounds,maxDepthBounds);
}
bool prosper::VlkCommandBuffer::RecordSetStencilCompareMask(StencilFaceFlags faceMask,uint32_t stencilCompareMask)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_compare_mask(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilCompareMask);
}
bool prosper::VlkCommandBuffer::RecordSetStencilReference(StencilFaceFlags faceMask,uint32_t stencilReference)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_reference(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilReference);
}
bool prosper::VlkCommandBuffer::RecordSetStencilWriteMask(StencilFaceFlags faceMask,uint32_t stencilWriteMask)
{
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_set_stencil_write_mask(static_cast<Anvil::StencilFaceFlagBits>(faceMask),stencilWriteMask);
}
bool prosper::VlkCommandBuffer::RecordBeginOcclusionQuery(const prosper::OcclusionQuery &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return const_cast<VlkCommandBuffer&>(*this)->record_reset_query_pool(anvPool,query.GetQueryId(),1u /* queryCount */) &&
		const_cast<VlkCommandBuffer&>(*this)->record_begin_query(anvPool,query.GetQueryId(),{});
}
bool prosper::VlkCommandBuffer::RecordEndOcclusionQuery(const prosper::OcclusionQuery &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return const_cast<VlkCommandBuffer&>(*this)->record_end_query(anvPool,query.GetQueryId());
}
bool prosper::VlkCommandBuffer::WriteTimestampQuery(const prosper::TimestampQuery &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	if(query.IsReset() == false && query.Reset(const_cast<VlkCommandBuffer&>(*this)) == false)
		return false;
	return const_cast<prosper::VlkCommandBuffer&>(*this)->record_write_timestamp(static_cast<Anvil::PipelineStageFlagBits>(query.GetPipelineStage()),anvPool,query.GetQueryId());
}

bool prosper::VlkCommandBuffer::ResetQuery(const Query &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	auto success = const_cast<VlkCommandBuffer&>(*this)->record_reset_query_pool(anvPool,query.GetQueryId(),1u /* queryCount */);
	if(success)
		const_cast<Query&>(query).OnReset(const_cast<VlkCommandBuffer&>(*this));
	return success;
}

bool prosper::VlkCommandBuffer::RecordBeginPipelineStatisticsQuery(const PipelineStatisticsQuery &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	if(query.IsReset() == false && query.Reset(const_cast<VlkCommandBuffer&>(*this)) == false)
		return false;
	return const_cast<prosper::VlkCommandBuffer&>(*this)->record_begin_query(anvPool,query.GetQueryId(),{});
}
bool prosper::VlkCommandBuffer::RecordEndPipelineStatisticsQuery(const PipelineStatisticsQuery &query) const
{
	auto *pQueryPool = static_cast<VlkQueryPool*>(query.GetPool());
	if(pQueryPool == nullptr)
		return false;
	auto *anvPool = &pQueryPool->GetAnvilQueryPool();
	return const_cast<VlkCommandBuffer&>(*this)->record_end_query(anvPool,query.GetQueryId());
}

///////////////////

std::shared_ptr<prosper::VlkPrimaryCommandBuffer> prosper::VlkPrimaryCommandBuffer::Create(IPrContext &context,Anvil::PrimaryCommandBufferUniquePtr cmdBuffer,prosper::QueueFamilyType queueFamilyType,const std::function<void(VlkCommandBuffer&)> &onDestroyedCallback)
{
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<VlkPrimaryCommandBuffer>(new VlkPrimaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType));
	return std::shared_ptr<VlkPrimaryCommandBuffer>(new VlkPrimaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType),[onDestroyedCallback](VlkCommandBuffer *buf) {
		buf->OnRelease();
		onDestroyedCallback(*buf);
		delete buf;
		});
}
prosper::VlkPrimaryCommandBuffer::VlkPrimaryCommandBuffer(IPrContext &context,Anvil::PrimaryCommandBufferUniquePtr cmdBuffer,prosper::QueueFamilyType queueFamilyType)
	: VlkCommandBuffer(context,prosper::util::unique_ptr_to_shared_ptr(std::move(cmdBuffer)),queueFamilyType),
	ICommandBuffer{context,queueFamilyType}
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}
bool prosper::VlkPrimaryCommandBuffer::StartRecording(bool oneTimeSubmit,bool simultaneousUseAllowed) const
{
	return static_cast<Anvil::PrimaryCommandBuffer&>(*m_cmdBuffer).start_recording(oneTimeSubmit,simultaneousUseAllowed);
}
bool prosper::VlkPrimaryCommandBuffer::IsPrimary() const {return true;}
Anvil::PrimaryCommandBuffer &prosper::VlkPrimaryCommandBuffer::GetAnvilCommandBuffer() const {return static_cast<Anvil::PrimaryCommandBuffer&>(VlkCommandBuffer::GetAnvilCommandBuffer());}
Anvil::PrimaryCommandBuffer &prosper::VlkPrimaryCommandBuffer::operator*() {return static_cast<Anvil::PrimaryCommandBuffer&>(VlkCommandBuffer::operator*());}
const Anvil::PrimaryCommandBuffer &prosper::VlkPrimaryCommandBuffer::operator*() const {return static_cast<const Anvil::PrimaryCommandBuffer&>(VlkCommandBuffer::operator*());}
Anvil::PrimaryCommandBuffer *prosper::VlkPrimaryCommandBuffer::operator->() {return static_cast<Anvil::PrimaryCommandBuffer*>(VlkCommandBuffer::operator->());}
const Anvil::PrimaryCommandBuffer *prosper::VlkPrimaryCommandBuffer::operator->() const {return static_cast<const Anvil::PrimaryCommandBuffer*>(VlkCommandBuffer::operator->());}

///////////////////

std::shared_ptr<prosper::VlkSecondaryCommandBuffer> prosper::VlkSecondaryCommandBuffer::Create(IPrContext &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,prosper::QueueFamilyType queueFamilyType,const std::function<void(VlkCommandBuffer&)> &onDestroyedCallback)
{
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<VlkSecondaryCommandBuffer>(new VlkSecondaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType));
	return std::shared_ptr<VlkSecondaryCommandBuffer>(new VlkSecondaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType),[onDestroyedCallback](VlkCommandBuffer *buf) {
		buf->OnRelease();
		onDestroyedCallback(*buf);
		delete buf;
		});
}
prosper::VlkSecondaryCommandBuffer::VlkSecondaryCommandBuffer(IPrContext &context,Anvil::SecondaryCommandBufferUniquePtr cmdBuffer,prosper::QueueFamilyType queueFamilyType)
	: VlkCommandBuffer(context,prosper::util::unique_ptr_to_shared_ptr(std::move(cmdBuffer)),queueFamilyType),
	ICommandBuffer{context,queueFamilyType},ISecondaryCommandBuffer{context,queueFamilyType}
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}
bool prosper::VlkSecondaryCommandBuffer::StartRecording(
	bool oneTimeSubmit,bool simultaneousUseAllowed,bool renderPassUsageOnly,
	const IFramebuffer &framebuffer,const IRenderPass &rp,Anvil::SubPassID subPassId,
	Anvil::OcclusionQuerySupportScope occlusionQuerySupportScope,bool occlusionQueryUsedByPrimaryCommandBuffer,
	Anvil::QueryPipelineStatisticFlags statisticsFlags
) const
{
	return static_cast<Anvil::SecondaryCommandBuffer&>(*m_cmdBuffer).start_recording(
		oneTimeSubmit,simultaneousUseAllowed,
		renderPassUsageOnly,&static_cast<const VlkFramebuffer&>(framebuffer).GetAnvilFramebuffer(),&static_cast<const VlkRenderPass&>(rp).GetAnvilRenderPass(),subPassId,occlusionQuerySupportScope,
		occlusionQueryUsedByPrimaryCommandBuffer,statisticsFlags
	);
}
bool prosper::VlkSecondaryCommandBuffer::IsSecondary() const {return true;}
Anvil::SecondaryCommandBuffer &prosper::VlkSecondaryCommandBuffer::GetAnvilCommandBuffer() const {return static_cast<Anvil::SecondaryCommandBuffer&>(VlkCommandBuffer::GetAnvilCommandBuffer());}
Anvil::SecondaryCommandBuffer &prosper::VlkSecondaryCommandBuffer::operator*() {return static_cast<Anvil::SecondaryCommandBuffer&>(VlkCommandBuffer::operator*());}
const Anvil::SecondaryCommandBuffer &prosper::VlkSecondaryCommandBuffer::operator*() const {return static_cast<const Anvil::SecondaryCommandBuffer&>(VlkCommandBuffer::operator*());}
Anvil::SecondaryCommandBuffer *prosper::VlkSecondaryCommandBuffer::operator->() {return static_cast<Anvil::SecondaryCommandBuffer*>(VlkCommandBuffer::operator->());}
const Anvil::SecondaryCommandBuffer *prosper::VlkSecondaryCommandBuffer::operator->() const {return static_cast<const Anvil::SecondaryCommandBuffer*>(VlkCommandBuffer::operator->());}
