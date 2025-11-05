// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.prosper;

import :command_buffer;

prosper::ICommandBuffer::ICommandBuffer(IPrContext &context, prosper::QueueFamilyType queueFamilyType)
    : ContextObject(context), std::enable_shared_from_this<ICommandBuffer>(), m_queueFamilyType {queueFamilyType}
#ifdef PR_DEBUG_API_DUMP
      ,
      m_apiDumpRecorder {std::make_unique<debug::ApiDumpRecorder>(this, &context.GetApiDumpRecorder())}
#endif
{
}

prosper::ICommandBuffer::~ICommandBuffer() {}

void prosper::ICommandBuffer::Initialize() { m_cmdBufSpecializationPtr = IsPrimary() ? static_cast<void *>(dynamic_cast<IPrimaryCommandBuffer *>(this)) : static_cast<void *>(dynamic_cast<ISecondaryCommandBuffer *>(this)); }

prosper::IPrimaryCommandBuffer *prosper::ICommandBuffer::GetPrimaryCommandBufferPtr() { return IsPrimary() ? static_cast<IPrimaryCommandBuffer *>(m_cmdBufSpecializationPtr) : nullptr; }
const prosper::IPrimaryCommandBuffer *prosper::ICommandBuffer::GetPrimaryCommandBufferPtr() const { return const_cast<ICommandBuffer *>(this)->GetPrimaryCommandBufferPtr(); }
prosper::ISecondaryCommandBuffer *prosper::ICommandBuffer::GetSecondaryCommandBufferPtr() { return !IsPrimary() ? static_cast<ISecondaryCommandBuffer *>(m_cmdBufSpecializationPtr) : nullptr; }
const prosper::ISecondaryCommandBuffer *prosper::ICommandBuffer::GetSecondaryCommandBufferPtr() const { return const_cast<ICommandBuffer *>(this)->GetSecondaryCommandBufferPtr(); }

bool prosper::ICommandBuffer::IsPrimary() const { return false; }
bool prosper::ICommandBuffer::IsSecondary() const { return false; }
prosper::QueueFamilyType prosper::ICommandBuffer::GetQueueFamilyType() const { return m_queueFamilyType; }

void prosper::ICommandBuffer::UpdateLastUsageTimes(IDescriptorSet &ds) { GetContext().UpdateLastUsageTimes(ds); }

bool prosper::ICommandBuffer::RecordPresentImage(IImage &img, uint32_t swapchainImgIndex) { return RecordPresentImage(img, *GetContext().GetSwapchainImage(swapchainImgIndex), *GetContext().GetSwapchainFramebuffer(swapchainImgIndex)); }
bool prosper::ICommandBuffer::RecordPresentImage(IImage &img, Window &window, uint32_t swapchainImgIndex)
{
	auto *imgSc = window.GetSwapchainImage(swapchainImgIndex);
	auto *fbSc = window.GetSwapchainFramebuffer(swapchainImgIndex);
	if(!imgSc || !fbSc)
		return false;
	return RecordPresentImage(img, *imgSc, *fbSc);
}
bool prosper::ICommandBuffer::RecordPresentImage(IImage &img, Window &window) { return RecordPresentImage(img, window, window.GetLastAcquiredSwapchainImageIndex()); }

bool prosper::IPrimaryCommandBuffer::StartRecording(bool oneTimeSubmit, bool simultaneousUseAllowed) const
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

bool prosper::ISecondaryCommandBuffer::StartRecording(bool oneTimeSubmit, bool simultaneousUseAllowed) const
{
	assert(!m_recording);
	SetRecording(true);
	return true;
}
bool prosper::ISecondaryCommandBuffer::StartRecording(prosper::IRenderPass &rp, prosper::IFramebuffer &fb, bool oneTimeSubmit, bool simultaneousUseAllowed) const
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
