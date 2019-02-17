/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"

using namespace prosper;

CommandBuffer::CommandBuffer(Context &context,const std::shared_ptr<Anvil::CommandBufferBase> &cmdBuffer,Anvil::QueueFamilyType queueFamilyType)
	: ContextObject(context),std::enable_shared_from_this<CommandBuffer>(),m_cmdBuffer(cmdBuffer),
	m_queueFamilyType(queueFamilyType)
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}

CommandBuffer::~CommandBuffer()
{
	prosper::debug::deregister_debug_object(m_cmdBuffer->get_command_buffer());
}

bool CommandBuffer::IsPrimary() const {return false;}
bool CommandBuffer::IsSecondary() const {return false;}
Anvil::QueueFamilyType CommandBuffer::GetQueueFamilyType() const {return m_queueFamilyType;}
bool CommandBuffer::Reset(bool shouldReleaseResources) const {return m_cmdBuffer->reset(shouldReleaseResources);}

bool CommandBuffer::StopRecording() const {return m_cmdBuffer->stop_recording();}

Anvil::CommandBufferBase &CommandBuffer::GetAnvilCommandBuffer() const {return *m_cmdBuffer;}
Anvil::CommandBufferBase &CommandBuffer::operator*() {return *m_cmdBuffer;}
const Anvil::CommandBufferBase &CommandBuffer::operator*() const {return const_cast<CommandBuffer*>(this)->operator*();}
Anvil::CommandBufferBase *CommandBuffer::operator->() {return m_cmdBuffer.get();}
const Anvil::CommandBufferBase *CommandBuffer::operator->() const {return const_cast<CommandBuffer*>(this)->operator->();}

///////////////////

std::shared_ptr<PrimaryCommandBuffer> PrimaryCommandBuffer::Create(Context &context,Anvil::PrimaryCommandBufferUniquePtr cmdBuffer,Anvil::QueueFamilyType queueFamilyType,const std::function<void(CommandBuffer&)> &onDestroyedCallback)
{
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<PrimaryCommandBuffer>(new PrimaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType));
	return std::shared_ptr<PrimaryCommandBuffer>(new PrimaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType),[onDestroyedCallback](CommandBuffer *buf) {
		onDestroyedCallback(*buf);
		delete buf;
	});
}
PrimaryCommandBuffer::PrimaryCommandBuffer(Context &context,Anvil::PrimaryCommandBufferUniquePtr cmdBuffer,Anvil::QueueFamilyType queueFamilyType)
	: CommandBuffer(context,prosper::util::unique_ptr_to_shared_ptr(std::move(cmdBuffer)),queueFamilyType)
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}
bool PrimaryCommandBuffer::StartRecording(bool oneTimeSubmit,bool simultaneousUseAllowed) const
{
	return static_cast<Anvil::PrimaryCommandBuffer&>(*m_cmdBuffer).start_recording(oneTimeSubmit,simultaneousUseAllowed);
}
bool PrimaryCommandBuffer::IsPrimary() const {return true;}
Anvil::PrimaryCommandBuffer &PrimaryCommandBuffer::GetAnvilCommandBuffer() const {return static_cast<Anvil::PrimaryCommandBuffer&>(CommandBuffer::GetAnvilCommandBuffer());}
Anvil::PrimaryCommandBuffer &PrimaryCommandBuffer::operator*() {return static_cast<Anvil::PrimaryCommandBuffer&>(CommandBuffer::operator*());}
const Anvil::PrimaryCommandBuffer &PrimaryCommandBuffer::operator*() const {return static_cast<const Anvil::PrimaryCommandBuffer&>(CommandBuffer::operator*());}
Anvil::PrimaryCommandBuffer *PrimaryCommandBuffer::operator->() {return static_cast<Anvil::PrimaryCommandBuffer*>(CommandBuffer::operator->());}
const Anvil::PrimaryCommandBuffer *PrimaryCommandBuffer::operator->() const {return static_cast<const Anvil::PrimaryCommandBuffer*>(CommandBuffer::operator->());}

///////////////////

std::shared_ptr<SecondaryCommandBuffer> SecondaryCommandBuffer::Create(Context &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,Anvil::QueueFamilyType queueFamilyType,const std::function<void(CommandBuffer&)> &onDestroyedCallback)
{
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<SecondaryCommandBuffer>(new SecondaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType));
	return std::shared_ptr<SecondaryCommandBuffer>(new SecondaryCommandBuffer(context,std::move(cmdBuffer),queueFamilyType),[onDestroyedCallback](CommandBuffer *buf) {
		onDestroyedCallback(*buf);
		delete buf;
	});
}
SecondaryCommandBuffer::SecondaryCommandBuffer(Context &context,Anvil::SecondaryCommandBufferUniquePtr cmdBuffer,Anvil::QueueFamilyType queueFamilyType)
	: CommandBuffer(context,prosper::util::unique_ptr_to_shared_ptr(std::move(cmdBuffer)),queueFamilyType)
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}
bool SecondaryCommandBuffer::StartRecording(
	bool oneTimeSubmit,bool simultaneousUseAllowed,bool renderPassUsageOnly,
	const Framebuffer &framebuffer,const RenderPass &rp,Anvil::SubPassID subPassId,
	Anvil::OcclusionQuerySupportScope occlusionQuerySupportScope,bool occlusionQueryUsedByPrimaryCommandBuffer,
	Anvil::QueryPipelineStatisticFlags statisticsFlags
) const
{
	return static_cast<Anvil::SecondaryCommandBuffer&>(*m_cmdBuffer).start_recording(
		oneTimeSubmit,simultaneousUseAllowed,
		renderPassUsageOnly,&framebuffer.GetAnvilFramebuffer(),&rp.GetAnvilRenderPass(),subPassId,occlusionQuerySupportScope,
		occlusionQueryUsedByPrimaryCommandBuffer,statisticsFlags
	);
}
bool SecondaryCommandBuffer::IsSecondary() const {return true;}
Anvil::SecondaryCommandBuffer &SecondaryCommandBuffer::GetAnvilCommandBuffer() const {return static_cast<Anvil::SecondaryCommandBuffer&>(CommandBuffer::GetAnvilCommandBuffer());}
Anvil::SecondaryCommandBuffer &SecondaryCommandBuffer::operator*() {return static_cast<Anvil::SecondaryCommandBuffer&>(CommandBuffer::operator*());}
const Anvil::SecondaryCommandBuffer &SecondaryCommandBuffer::operator*() const {return static_cast<const Anvil::SecondaryCommandBuffer&>(CommandBuffer::operator*());}
Anvil::SecondaryCommandBuffer *SecondaryCommandBuffer::operator->() {return static_cast<Anvil::SecondaryCommandBuffer*>(CommandBuffer::operator->());}
const Anvil::SecondaryCommandBuffer *SecondaryCommandBuffer::operator->() const {return static_cast<const Anvil::SecondaryCommandBuffer*>(CommandBuffer::operator->());}
