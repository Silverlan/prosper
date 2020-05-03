/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_command_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "vk_framebuffer.hpp"
#include "vk_render_pass.hpp"
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
