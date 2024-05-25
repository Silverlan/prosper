/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_util.hpp"
#include "prosper_command_buffer.hpp"
#include "debug/prosper_debug.hpp"
#include "prosper_framebuffer.hpp"
#include "image/prosper_render_target.hpp"
#include "prosper_render_pass.hpp"
#include "buffers/prosper_buffer.hpp"
#include "shader/prosper_shader.hpp"
#include <sharedutils/util.h>

#ifdef DEBUG_VERBOSE
#include <iostream>
#endif

void prosper::util::apply_image_subresource_range(const prosper::util::ImageSubresourceRange &srcRange, prosper::util::ImageSubresourceRange &vkRange, prosper::IImage &img)
{
	vkRange.baseMipLevel = srcRange.baseMipLevel;
	vkRange.levelCount = srcRange.levelCount;
	vkRange.baseArrayLayer = srcRange.baseArrayLayer;
	vkRange.layerCount = srcRange.layerCount;

	if(vkRange.levelCount == std::numeric_limits<uint32_t>::max())
		vkRange.levelCount = img.GetMipmapCount() - srcRange.baseMipLevel;
	if(vkRange.layerCount == std::numeric_limits<uint32_t>::max())
		vkRange.layerCount = img.GetLayerCount() - srcRange.baseArrayLayer;
}
prosper::util::BarrierImageLayout::BarrierImageLayout(PipelineStageFlags pipelineStageFlags, ImageLayout imageLayout, AccessFlags accessFlags) : stageMask(pipelineStageFlags), layout(imageLayout), accessMask(accessFlags) {}

prosper::util::BufferBarrier prosper::util::create_buffer_barrier(const prosper::util::BufferBarrierInfo &barrierInfo, IBuffer &buffer)
{
	return util::BufferBarrier(barrierInfo.srcAccessMask, barrierInfo.dstAccessMask, QUEUE_FAMILY_IGNORED, QUEUE_FAMILY_IGNORED, &buffer, barrierInfo.offset, umath::min(barrierInfo.offset + barrierInfo.size, buffer.GetSize()) - barrierInfo.offset);
}

prosper::util::ImageBarrier prosper::util::create_image_barrier(IImage &img, const BarrierImageLayout &srcBarrierInfo, const BarrierImageLayout &dstBarrierInfo, const ImageSubresourceRange &subresourceRange, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	util::ImageSubresourceRange resourceRange {};
	apply_image_subresource_range(subresourceRange, resourceRange, img);
	return prosper::util::ImageBarrier(srcBarrierInfo.accessMask, dstBarrierInfo.accessMask, srcBarrierInfo.layout, dstBarrierInfo.layout, QUEUE_FAMILY_IGNORED, QUEUE_FAMILY_IGNORED, &img, resourceRange, aspectMask);
}

prosper::util::ImageBarrier prosper::util::create_image_barrier(IImage &img, const prosper::util::ImageBarrierInfo &barrierInfo, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	util::ImageSubresourceRange resourceRange {};
	apply_image_subresource_range(barrierInfo.subresourceRange, resourceRange, img);
	return ImageBarrier {barrierInfo.srcAccessMask, barrierInfo.dstAccessMask, barrierInfo.oldLayout, barrierInfo.newLayout, barrierInfo.srcQueueFamilyIndex, barrierInfo.dstQueueFamilyIndex, &img, resourceRange, aspectMask};
}

std::unique_ptr<std::unordered_map<prosper::ICommandBuffer *, std::unordered_map<prosper::IImage *, prosper::debug::ImageLayoutInfo>>> s_lastRecordedImageLayouts = nullptr;
static std::function<void(prosper::DebugReportObjectTypeEXT, const std::string &)> s_debugCallback = nullptr;
void prosper::debug::set_debug_validation_callback(const std::function<void(prosper::DebugReportObjectTypeEXT, const std::string &)> &callback) { s_debugCallback = callback; }
void prosper::debug::exec_debug_validation_callback(IPrContext &context, prosper::DebugReportObjectTypeEXT objType, const std::string &msg)
{
	if(s_debugCallback == nullptr)
		return;
	auto debugMsg = msg;
	context.AddDebugObjectInformation(debugMsg);
	s_debugCallback(objType, debugMsg);
}
bool prosper::debug::is_debug_recorded_image_layout_enabled() { return s_lastRecordedImageLayouts != nullptr; }
void prosper::debug::enable_debug_recorded_image_layout(bool b)
{
	if(b == false) {
		s_lastRecordedImageLayouts = nullptr;
		return;
	}
	s_lastRecordedImageLayouts = std::make_unique<std::unordered_map<prosper::ICommandBuffer *, std::unordered_map<prosper::IImage *, prosper::debug::ImageLayoutInfo>>>();
}
void prosper::debug::set_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer, IImage &img, ImageLayout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t baseMipmap, uint32_t mipmapLevels)
{
#ifdef DEBUG_VERBOSE
	std::cout << "[PR] Setting image layout for image " << img.get_image() << " to: " << vk::to_string(layout) << std::endl;
#endif
	//if(debug::get_image_from_anvil_image(img)->GetDebugName() == "prepass_depth_normal_rt_tex0_resolved_img")
	//	std::cout<<"prepass_depth_normal_rt_tex0_resolved_img changed to "<<vk::to_string(layout)<<"!"<<std::endl;
	auto it = s_lastRecordedImageLayouts->find(&cmdBuffer);
	if(it == s_lastRecordedImageLayouts->end())
		it = s_lastRecordedImageLayouts->insert(std::make_pair(&cmdBuffer, std::unordered_map<prosper::IImage *, prosper::debug::ImageLayoutInfo> {})).first;

	auto itLayoutInfo = it->second.find(&img);
	if(itLayoutInfo == it->second.end())
		itLayoutInfo = it->second.insert(std::make_pair(&img, prosper::debug::ImageLayoutInfo {})).first;
	auto &layoutInfo = itLayoutInfo->second;
	if(layerCount == std::numeric_limits<uint32_t>::max())
		layerCount = img.GetLayerCount() - baseLayer;
	if(mipmapLevels == std::numeric_limits<uint32_t>::max())
		mipmapLevels = img.GetMipmapCount() - baseMipmap;
	auto totalLayerCount = layerCount + baseLayer;
	if(totalLayerCount >= layoutInfo.layerLayouts.size())
		layoutInfo.layerLayouts.resize(totalLayerCount);

	auto totalMipmapLevels = baseMipmap + mipmapLevels;
	for(auto &layouts : layoutInfo.layerLayouts) {
		if(layouts.mipmapLayouts.size() < totalMipmapLevels)
			layouts.mipmapLayouts.resize(totalMipmapLevels, ImageLayout::Undefined);
	}

	for(auto i = baseLayer; i < (baseLayer + layerCount); ++i) {
		for(auto j = baseMipmap; j < (baseMipmap + mipmapLevels); ++j)
			itLayoutInfo->second.layerLayouts.at(i).mipmapLayouts.at(j) = layout;
	}
}
bool prosper::debug::get_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer, IImage &img, ImageLayout &layout, uint32_t baseLayer, uint32_t baseMipmap)
{
	if(s_lastRecordedImageLayouts == nullptr)
		return false;
	// TODO: Clear elements of s_lastRecordedImageLayouts if command buffer or image were released
	auto it = s_lastRecordedImageLayouts->find(&cmdBuffer);
	if(it == s_lastRecordedImageLayouts->end())
		return false;
	auto itLayout = it->second.find(&img);
	if(itLayout == it->second.end() || baseLayer >= itLayout->second.layerLayouts.size())
		return false;
	auto &mipmapLayouts = itLayout->second.layerLayouts.at(baseLayer).mipmapLayouts;
	if(baseMipmap >= mipmapLayouts.size())
		return false;
	layout = mipmapLayouts.at(baseMipmap);
	return true;
}

static bool record_image_barrier(prosper::ICommandBuffer &cmdBuffer, prosper::IImage &img, prosper::ImageLayout originalLayout, prosper::ImageLayout currentLayout, prosper::ImageLayout dstLayout, const prosper::util::ImageSubresourceRange &subresourceRange = {},
  std::optional<prosper::ImageAspectFlags> aspectMask = {})
{
	prosper::util::BarrierImageLayout srcInfo {};
	switch(originalLayout) {
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		srcInfo = {prosper::PipelineStageFlags::FragmentShaderBit, currentLayout, prosper::AccessFlags::ShaderReadBit};
		break;
	case prosper::ImageLayout::ColorAttachmentOptimal:
		srcInfo = {prosper::PipelineStageFlags::ColorAttachmentOutputBit, currentLayout, prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::ColorAttachmentWriteBit};
		break;
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		srcInfo = {prosper::PipelineStageFlags::LateFragmentTestsBit, currentLayout, prosper::AccessFlags::DepthStencilAttachmentReadBit | prosper::AccessFlags::DepthStencilAttachmentWriteBit};
		break;
	case prosper::ImageLayout::TransferSrcOptimal:
		srcInfo = {prosper::PipelineStageFlags::TransferBit, currentLayout, prosper::AccessFlags::TransferReadBit};
		break;
	case prosper::ImageLayout::TransferDstOptimal:
		srcInfo = {prosper::PipelineStageFlags::TransferBit, currentLayout, prosper::AccessFlags::TransferWriteBit};
		break;
	}

	prosper::util::BarrierImageLayout dstInfo {};
	switch(dstLayout) {
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		dstInfo = {prosper::PipelineStageFlags::FragmentShaderBit, dstLayout, prosper::AccessFlags::ShaderReadBit};
		break;
	case prosper::ImageLayout::ColorAttachmentOptimal:
		dstInfo = {prosper::PipelineStageFlags::ColorAttachmentOutputBit, dstLayout, prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::ColorAttachmentWriteBit};
		break;
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		dstInfo = {prosper::PipelineStageFlags::EarlyFragmentTestsBit, dstLayout, prosper::AccessFlags::DepthStencilAttachmentReadBit | prosper::AccessFlags::DepthStencilAttachmentWriteBit};
		break;
	case prosper::ImageLayout::TransferSrcOptimal:
		dstInfo = {prosper::PipelineStageFlags::TransferBit, dstLayout, prosper::AccessFlags::TransferReadBit};
		break;
	case prosper::ImageLayout::TransferDstOptimal:
		dstInfo = {prosper::PipelineStageFlags::TransferBit, dstLayout, prosper::AccessFlags::TransferWriteBit};
		break;
	}
	return cmdBuffer.RecordImageBarrier(img, srcInfo, dstInfo, subresourceRange, aspectMask);
}

bool prosper::ICommandBuffer::RecordImageBarrier(prosper::IImage &img, prosper::PipelineStageFlags srcStageMask, prosper::PipelineStageFlags dstStageMask, prosper::ImageLayout oldLayout, prosper::ImageLayout newLayout, prosper::AccessFlags srcAccessMask, prosper::AccessFlags dstAccessMask,
  uint32_t baseLayer, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcStageMask;
	barrier.dstStageMask = dstStageMask;

	prosper::util::ImageBarrierInfo imgBarrier {};
	imgBarrier.oldLayout = oldLayout;
	imgBarrier.newLayout = newLayout;
	imgBarrier.srcAccessMask = srcAccessMask;
	imgBarrier.dstAccessMask = dstAccessMask;
	if(baseLayer != std::numeric_limits<uint32_t>::max()) {
		imgBarrier.subresourceRange.baseArrayLayer = baseLayer;
		imgBarrier.subresourceRange.layerCount = 1u;
	}
	barrier.imageBarriers.push_back(prosper::util::create_image_barrier(img, imgBarrier, aspectMask));

	return RecordPipelineBarrier(barrier);
}
bool prosper::ICommandBuffer::RecordImageBarrier(IImage &img, const util::BarrierImageLayout &srcBarrierInfo, const util::BarrierImageLayout &dstBarrierInfo, const util::ImageSubresourceRange &subresourceRange, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcBarrierInfo.stageMask;
	barrier.dstStageMask = dstBarrierInfo.stageMask;
	barrier.imageBarriers.push_back(prosper::util::create_image_barrier(img, srcBarrierInfo, dstBarrierInfo, subresourceRange, aspectMask));
	return RecordPipelineBarrier(barrier);
}
bool prosper::ICommandBuffer::RecordImageBarrier(IImage &img, ImageLayout srcLayout, ImageLayout dstLayout, const util::ImageSubresourceRange &subresourceRange, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	return ::record_image_barrier(*this, img, srcLayout, srcLayout, dstLayout, subresourceRange, aspectMask);
}
bool prosper::ICommandBuffer::RecordPostRenderPassImageBarrier(IImage &img, ImageLayout preRenderPassLayout, ImageLayout postRenderPassLayout, const util::ImageSubresourceRange &subresourceRange, std::optional<prosper::ImageAspectFlags> aspectMask)
{
	return ::record_image_barrier(*this, img, preRenderPassLayout, postRenderPassLayout, postRenderPassLayout, subresourceRange, aspectMask);
}
void prosper::ICommandBuffer::ClearBoundPipeline() {}
bool prosper::ICommandBuffer::RecordUnbindShaderPipeline()
{
	ClearBoundPipeline();
	return true;
}
bool prosper::ICommandBuffer::RecordBindShaderPipeline(prosper::Shader &shader, PipelineID shaderPipelineId)
{
	auto *pipelineInfo = shader.GetPipelineInfo(shaderPipelineId);
	if(pipelineInfo == nullptr)
		return false;
	auto pipelineId = pipelineInfo->id;
	return pipelineId != std::numeric_limits<prosper::PipelineID>::max() && DoRecordBindShaderPipeline(shader, shaderPipelineId, pipelineId);
}
bool prosper::ICommandBuffer::RecordBufferBarrier(IBuffer &buf, PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, AccessFlags srcAccessMask, AccessFlags dstAccessMask, DeviceSize offset, DeviceSize size)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcStageMask;
	barrier.dstStageMask = dstStageMask;

	if(size == std::numeric_limits<DeviceSize>::max())
		size = buf.GetSize();

	prosper::util::BufferBarrierInfo bufBarrier {};
	bufBarrier.srcAccessMask = srcAccessMask;
	bufBarrier.dstAccessMask = dstAccessMask;
	bufBarrier.size = size;
	bufBarrier.offset = buf.GetStartOffset() + offset;
	barrier.bufferBarriers.push_back(prosper::util::create_buffer_barrier(bufBarrier, buf));
	return RecordPipelineBarrier(barrier);
}

/////////////////

bool prosper::IPrimaryCommandBuffer::RecordEndRenderPass()
{
	m_renderTargetInfo = {};
	return DoRecordEndRenderPass();
}
prosper::IPrimaryCommandBuffer::RenderTargetInfo *prosper::IPrimaryCommandBuffer::GetActiveRenderPassTargetInfo() const { return m_renderTargetInfo.has_value() ? &*m_renderTargetInfo : nullptr; }
bool prosper::IPrimaryCommandBuffer::GetActiveRenderPassTarget(prosper::IRenderPass **outRp, prosper::IImage **outImg, prosper::IFramebuffer **outFb, prosper::RenderTarget **outRt) const
{
	auto &rtInfo = m_renderTargetInfo;
	if(rtInfo.has_value() == false)
		return false;
	if(outRp)
		*outRp = rtInfo->renderPass.lock().get();
	if(outImg)
		*outImg = rtInfo->image.lock().get();
	if(outFb)
		*outFb = rtInfo->framebuffer.lock().get();
	if(outRt)
		*outRt = rtInfo->renderTarget.lock().get();
	return true;
}

void prosper::IPrimaryCommandBuffer::SetActiveRenderPassTarget(prosper::IRenderPass *outRp, uint32_t layerId, prosper::IImage *outImg, prosper::IFramebuffer *outFb, prosper::RenderTarget *outRt) const
{
	auto &rtInfo = m_renderTargetInfo;
	rtInfo = RenderTargetInfo {};
	rtInfo->renderPass = outRp ? outRp->shared_from_this() : std::weak_ptr<prosper::IRenderPass> {};
	rtInfo->baseLayer = layerId;
	rtInfo->image = outImg ? outImg->shared_from_this() : std::weak_ptr<prosper::IImage> {};
	rtInfo->framebuffer = outFb ? outFb->shared_from_this() : std::weak_ptr<prosper::IFramebuffer> {};
	rtInfo->renderTarget = outRt ? outRt->shared_from_this() : std::weak_ptr<prosper::RenderTarget> {};
}

bool prosper::IPrimaryCommandBuffer::DoRecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t *layerId, const std::vector<prosper::ClearValue> &clearValues, prosper::IRenderPass *rp, RenderPassFlags renderPassFlags)
{
	auto *fb = (layerId != nullptr) ? rt.GetFramebuffer(*layerId) : &rt.GetFramebuffer();
	if(rp == nullptr)
		rp = &rt.GetRenderPass();
	if(fb == nullptr)
		throw std::runtime_error("Attempted to begin render pass with NULL framebuffer object!");
	if(rp == nullptr)
		throw std::runtime_error("Attempted to begin render pass with NULL render pass object!");
	auto &tex = rt.GetTexture();
	auto &img = tex.GetImage();

	if(GetContext().IsValidationEnabled()) {
		auto n = rt.GetAttachmentCount();
		for(auto i = decltype(n) {0u}; i < n; ++i) {
			auto *tex = rt.GetTexture(i);
			if(tex == nullptr)
				continue;
			GetContext().UpdateLastUsageTime(tex->GetImage());
		}
	}

	SetActiveRenderPassTarget(rp, (layerId != nullptr) ? *layerId : std::numeric_limits<uint32_t>::max(), &img, fb, nullptr);
	return DoRecordBeginRenderPass(img, *rp, *fb, layerId, clearValues, renderPassFlags);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t layerId, RenderPassFlags renderPassFlags, const prosper::ClearValue *clearValue, prosper::IRenderPass *rp)
{
	return DoRecordBeginRenderPass(rt, &layerId, (clearValue != nullptr) ? std::vector<prosper::ClearValue> {*clearValue} : std::vector<prosper::ClearValue> {}, rp, renderPassFlags);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t layerId, const std::vector<prosper::ClearValue> &clearValues, RenderPassFlags renderPassFlags, prosper::IRenderPass *rp)
{
	return DoRecordBeginRenderPass(rt, &layerId, clearValues, rp, renderPassFlags);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt, RenderPassFlags renderPassFlags, const prosper::ClearValue *clearValue, prosper::IRenderPass *rp)
{
	return DoRecordBeginRenderPass(rt, nullptr, (clearValue != nullptr) ? std::vector<prosper::ClearValue> {*clearValue} : std::vector<prosper::ClearValue> {}, rp, renderPassFlags);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt, const std::vector<prosper::ClearValue> &clearValues, RenderPassFlags renderPassFlags, prosper::IRenderPass *rp) { return DoRecordBeginRenderPass(rt, nullptr, clearValues, rp, renderPassFlags); }
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::IImage &img, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, RenderPassFlags renderPassFlags, const std::vector<prosper::ClearValue> &clearValues)
{
	return DoRecordBeginRenderPass(img, rp, fb, 0u, clearValues, renderPassFlags);
}
