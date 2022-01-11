/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "queries/prosper_query.hpp"
#include "queries/prosper_occlusion_query.hpp"
#include "queries/prosper_pipeline_statistics_query.hpp"
#include "queries/prosper_timer_query.hpp"
#include "queries/prosper_timestamp_query.hpp"
#include "queries/prosper_query_pool.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_window.hpp"

prosper::ICommandBuffer::ICommandBuffer(IPrContext &context,prosper::QueueFamilyType queueFamilyType)
	: ContextObject(context),std::enable_shared_from_this<ICommandBuffer>(),m_queueFamilyType{queueFamilyType}
{}

prosper::ICommandBuffer::~ICommandBuffer() {}

void prosper::ICommandBuffer::Initialize()
{
	m_cmdBufSpecializationPtr = IsPrimary() ? static_cast<void*>(dynamic_cast<IPrimaryCommandBuffer*>(this)) : static_cast<void*>(dynamic_cast<ISecondaryCommandBuffer*>(this));
}

prosper::IPrimaryCommandBuffer *prosper::ICommandBuffer::GetPrimaryCommandBufferPtr() {return IsPrimary() ? static_cast<IPrimaryCommandBuffer*>(m_cmdBufSpecializationPtr) : nullptr;}
const prosper::IPrimaryCommandBuffer *prosper::ICommandBuffer::GetPrimaryCommandBufferPtr() const {return const_cast<ICommandBuffer*>(this)->GetPrimaryCommandBufferPtr();}
prosper::ISecondaryCommandBuffer *prosper::ICommandBuffer::GetSecondaryCommandBufferPtr() {return !IsPrimary() ? static_cast<ISecondaryCommandBuffer*>(m_cmdBufSpecializationPtr) : nullptr;}
const prosper::ISecondaryCommandBuffer *prosper::ICommandBuffer::GetSecondaryCommandBufferPtr() const {return const_cast<ICommandBuffer*>(this)->GetSecondaryCommandBufferPtr();}

bool prosper::ICommandBuffer::IsPrimary() const {return false;}
bool prosper::ICommandBuffer::IsSecondary() const {return false;}
prosper::QueueFamilyType prosper::ICommandBuffer::GetQueueFamilyType() const {return m_queueFamilyType;}

void prosper::ICommandBuffer::UpdateLastUsageTimes(IDescriptorSet &ds) {GetContext().UpdateLastUsageTimes(ds);}

bool prosper::ICommandBuffer::RecordPresentImage(IImage &img,uint32_t swapchainImgIndex)
{
	return RecordPresentImage(img,*GetContext().GetSwapchainImage(swapchainImgIndex),*GetContext().GetSwapchainFramebuffer(swapchainImgIndex));
}
bool prosper::ICommandBuffer::RecordPresentImage(IImage &img,Window &window,uint32_t swapchainImgIndex)
{
	auto *imgSc = window.GetSwapchainImage(swapchainImgIndex);
	auto *fbSc = window.GetSwapchainFramebuffer(swapchainImgIndex);
	if(!imgSc || !fbSc)
		return false;
	return RecordPresentImage(img,*imgSc,*fbSc);
}
bool prosper::ICommandBuffer::RecordPresentImage(IImage &img,Window &window)
{
	return RecordPresentImage(img,window,window.GetLastAcquiredSwapchainImageIndex());
}

bool prosper::IPrimaryCommandBuffer::StartRecording(bool oneTimeSubmit,bool simultaneousUseAllowed) const
{
	assert(!m_recording);
	SetRecording(true);
	return true;
}
bool prosper::IPrimaryCommandBuffer::StopRecording() const
{
	assert(m_recording);
	SetRecording(false);
	return true;
}

bool prosper::ISecondaryCommandBuffer::StartRecording(bool oneTimeSubmit,bool simultaneousUseAllowed) const
{
	assert(!m_recording);
	SetRecording(true);
	return true;
}
bool prosper::ISecondaryCommandBuffer::StartRecording(prosper::IRenderPass &rp,prosper::IFramebuffer &fb,bool oneTimeSubmit,bool simultaneousUseAllowed) const
{
	assert(!m_recording);
	SetRecording(true);
	m_currentRenderPass = &rp;
	m_currentFramebuffer = &fb;
	return true;
}
bool prosper::ISecondaryCommandBuffer::StopRecording() const
{
	assert(m_recording);
	SetRecording(false);
	m_currentRenderPass = nullptr;
	m_currentFramebuffer = nullptr;
	return true;
}
