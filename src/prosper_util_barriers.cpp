/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include "stdafx_prosper.h"
#include "prosper_util.hpp"
#include "vk_command_buffer.hpp"
#include "debug/prosper_debug.hpp"
#include "image/prosper_render_target.hpp"
#include "image/vk_image_view.hpp"
#include "image/vk_image.hpp"
#include "vk_command_buffer.hpp"
#include "vk_context.hpp"
#include "buffers/vk_buffer.hpp"
#include "vk_framebuffer.hpp"
#include "vk_render_pass.hpp"
#include <wrappers/render_pass.h>
#include <wrappers/command_buffer.h>
#include <wrappers/render_pass.h>
#include <wrappers/image.h>
#include <config.h>
#include <misc/render_pass_create_info.h>
#include <misc/image_create_info.h>
#include <sharedutils/util.h>

#ifdef DEBUG_VERBOSE
#include <iostream>
#endif

static void apply_range(const prosper::util::ImageSubresourceRange &srcRange,prosper::util::ImageSubresourceRange &vkRange,prosper::IImage &img)
{
	vkRange.baseMipLevel = srcRange.baseMipLevel;
	vkRange.levelCount = srcRange.levelCount;
	vkRange.baseArrayLayer = srcRange.baseArrayLayer;
	vkRange.layerCount = srcRange.layerCount;

	if(vkRange.levelCount == std::numeric_limits<uint32_t>::max())
		vkRange.levelCount = img.GetMipmapCount() -srcRange.baseMipLevel;
	if(vkRange.layerCount == std::numeric_limits<uint32_t>::max())
		vkRange.layerCount = img.GetLayerCount() -srcRange.baseArrayLayer;
}
prosper::util::BarrierImageLayout::BarrierImageLayout(PipelineStageFlags pipelineStageFlags,ImageLayout imageLayout,AccessFlags accessFlags)
	: stageMask(pipelineStageFlags),layout(imageLayout),accessMask(accessFlags)
{}

prosper::util::BufferBarrier prosper::util::create_buffer_barrier(const prosper::util::BufferBarrierInfo &barrierInfo,IBuffer &buffer)
{
	return util::BufferBarrier(
		barrierInfo.srcAccessMask,barrierInfo.dstAccessMask,
		VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,&buffer,barrierInfo.offset,
		umath::min(barrierInfo.offset +barrierInfo.size,buffer.GetSize()) -barrierInfo.offset
	);
}

prosper::util::ImageBarrier prosper::util::create_image_barrier(IImage &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,const ImageSubresourceRange &subresourceRange)
{
	util::ImageSubresourceRange resourceRange {};
	apply_range(subresourceRange,resourceRange,img);
	return prosper::util::ImageBarrier(
		srcBarrierInfo.accessMask,dstBarrierInfo.accessMask,
		srcBarrierInfo.layout,dstBarrierInfo.layout,
		VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,&img,resourceRange
	);
}

prosper::util::ImageBarrier prosper::util::create_image_barrier(IImage &img,const prosper::util::ImageBarrierInfo &barrierInfo)
{
	util::ImageSubresourceRange resourceRange {};
	apply_range(barrierInfo.subresourceRange,resourceRange,img);
	return ImageBarrier{
		barrierInfo.srcAccessMask,barrierInfo.dstAccessMask,
		barrierInfo.oldLayout,barrierInfo.newLayout,
		barrierInfo.srcQueueFamilyIndex,barrierInfo.dstQueueFamilyIndex,
		&img,resourceRange
	};
}

struct DebugImageLayouts
{
	std::vector<prosper::ImageLayout> mipmapLayouts;
};
struct DebugImageLayoutInfo
{
	std::vector<DebugImageLayouts> layerLayouts;
};
std::unique_ptr<std::unordered_map<prosper::ICommandBuffer*,std::unordered_map<prosper::IImage*,DebugImageLayoutInfo>>> s_lastRecordedImageLayouts = nullptr;
static std::function<void(prosper::DebugReportObjectTypeEXT,const std::string&)> s_debugCallback = nullptr;
void prosper::debug::set_debug_validation_callback(const std::function<void(prosper::DebugReportObjectTypeEXT,const std::string&)> &callback) {s_debugCallback = callback;}
void prosper::debug::exec_debug_validation_callback(prosper::DebugReportObjectTypeEXT objType,const std::string &msg)
{
	if(s_debugCallback == nullptr)
		return;
	auto debugMsg = msg;
	prosper::debug::add_debug_object_information(debugMsg);
	s_debugCallback(objType,debugMsg);
}
bool prosper::debug::is_debug_recorded_image_layout_enabled() {return s_lastRecordedImageLayouts != nullptr;}
void prosper::debug::enable_debug_recorded_image_layout(bool b)
{
	if(b == false)
	{
		s_lastRecordedImageLayouts = nullptr;
		return;
	}
	s_lastRecordedImageLayouts = std::make_unique<std::unordered_map<prosper::ICommandBuffer*,std::unordered_map<prosper::IImage*,DebugImageLayoutInfo>>>();
}
static Anvil::ImageSubresourceRange to_anvil_subresource_range(const prosper::util::ImageSubresourceRange &range,prosper::IImage &img)
{
	Anvil::ImageSubresourceRange anvRange {};
	anvRange.base_array_layer = range.baseArrayLayer;
	anvRange.base_mip_level = range.baseMipLevel;
	anvRange.layer_count = range.layerCount;
	anvRange.level_count = range.levelCount;
	anvRange.aspect_mask = static_cast<Anvil::ImageAspectFlagBits>(prosper::util::get_aspect_mask(img));
	return anvRange;
}
bool prosper::ICommandBuffer::RecordPipelineBarrier(const prosper::util::PipelineBarrierInfo &barrierInfo)
{
	if(s_lastRecordedImageLayouts != nullptr)
	{
		auto it = s_lastRecordedImageLayouts->find(this);
		if(it == s_lastRecordedImageLayouts->end())
			it = s_lastRecordedImageLayouts->insert(std::make_pair(this,std::unordered_map<prosper::IImage*,DebugImageLayoutInfo>{})).first;
		else
		{
			for(auto &imgBarrier : barrierInfo.imageBarriers)
			{
				auto itImg = it->second.find(imgBarrier.image);
				if(itImg == it->second.end())
					continue;
				auto &layoutInfo = itImg->second;
				auto &range = imgBarrier.subresourceRange;
				for(auto i=range.baseArrayLayer;i<(range.baseArrayLayer +range.layerCount);++i)
				{
					if(i >= layoutInfo.layerLayouts.size())
						break;
					auto &layerLayout = layoutInfo.layerLayouts.at(i);
					for(auto j=range.baseMipLevel;j<(range.baseMipLevel +range.levelCount);++j)
					{
						if(j >= layerLayout.mipmapLayouts.size())
							break;
						auto layout = layerLayout.mipmapLayouts.at(j);
						if(layout != imgBarrier.oldLayout && imgBarrier.oldLayout != ImageLayout::Undefined)
						{
							debug::exec_debug_validation_callback(
								prosper::DebugReportObjectTypeEXT::Image,"Record pipeline barrier: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgBarrier.image)) +
								" at array layer " +std::to_string(i) +", mipmap " +std::to_string(j) +" has current layout " +vk::to_string(static_cast<vk::ImageLayout>(layout)) +
								", but should have " +vk::to_string(static_cast<vk::ImageLayout>(imgBarrier.oldLayout)) +" according to barrier info!"
							);
						}
					}
				}
			}
		}
		for(auto &imgBarrier : barrierInfo.imageBarriers)
		{
			debug::set_last_recorded_image_layout(
				*this,*imgBarrier.image,imgBarrier.newLayout,
				imgBarrier.subresourceRange.baseArrayLayer,imgBarrier.subresourceRange.layerCount,
				imgBarrier.subresourceRange.baseMipLevel,imgBarrier.subresourceRange.levelCount
			);
		}
	}
	std::vector<Anvil::BufferBarrier> anvBufBarriers {};
	anvBufBarriers.reserve(barrierInfo.bufferBarriers.size());
	for(auto &barrier : barrierInfo.bufferBarriers)
	{
		anvBufBarriers.push_back(Anvil::BufferBarrier{
			static_cast<Anvil::AccessFlagBits>(barrier.srcAccessMask),static_cast<Anvil::AccessFlagBits>(barrier.dstAccessMask),
			barrier.srcQueueFamilyIndex,barrier.dstQueueFamilyIndex,
			&dynamic_cast<VlkBuffer*>(barrier.buffer)->GetAnvilBuffer(),barrier.offset,barrier.size
			});
	}
	std::vector<Anvil::ImageBarrier> anvImgBarriers {};
	anvImgBarriers.reserve(barrierInfo.imageBarriers.size());
	for(auto &barrier : barrierInfo.imageBarriers)
	{
		anvImgBarriers.push_back(Anvil::ImageBarrier{
			static_cast<Anvil::AccessFlagBits>(barrier.srcAccessMask),static_cast<Anvil::AccessFlagBits>(barrier.dstAccessMask),
			static_cast<Anvil::ImageLayout>(barrier.oldLayout),static_cast<Anvil::ImageLayout>(barrier.newLayout),
			barrier.srcQueueFamilyIndex,barrier.dstQueueFamilyIndex,
			&static_cast<VlkImage*>(barrier.image)->GetAnvilImage(),to_anvil_subresource_range(barrier.subresourceRange,*barrier.image)
			});
	}
	return dynamic_cast<VlkCommandBuffer&>(*this)->record_pipeline_barrier(
		static_cast<Anvil::PipelineStageFlagBits>(barrierInfo.srcStageMask),static_cast<Anvil::PipelineStageFlagBits>(barrierInfo.dstStageMask),
		Anvil::DependencyFlagBits::NONE,0u,nullptr,
		anvBufBarriers.size(),anvBufBarriers.data(),
		anvImgBarriers.size(),anvImgBarriers.data()
	);
}
void prosper::debug::set_last_recorded_image_layout(
	prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout layout,
	uint32_t baseLayer,uint32_t layerCount,uint32_t baseMipmap,uint32_t mipmapLevels
)
{
#ifdef DEBUG_VERBOSE
	std::cout<<"[VK] Setting image layout for image "<<img.get_image()<<" to: "<<vk::to_string(layout)<<std::endl;
#endif
	//if(debug::get_image_from_anvil_image(img)->GetDebugName() == "prepass_depth_normal_rt_tex0_resolved_img")
	//	std::cout<<"prepass_depth_normal_rt_tex0_resolved_img changed to "<<vk::to_string(layout)<<"!"<<std::endl;
	auto it = s_lastRecordedImageLayouts->find(&cmdBuffer);
	if(it == s_lastRecordedImageLayouts->end())
		it = s_lastRecordedImageLayouts->insert(std::make_pair(&cmdBuffer,std::unordered_map<prosper::IImage*,DebugImageLayoutInfo>{})).first;

	auto itLayoutInfo = it->second.find(&img);
	if(itLayoutInfo == it->second.end())
		itLayoutInfo = it->second.insert(std::make_pair(&img,DebugImageLayoutInfo{})).first;
	auto &layoutInfo = itLayoutInfo->second;
	if(layerCount == std::numeric_limits<uint32_t>::max())
		layerCount = img.GetLayerCount() -baseLayer;
	if(mipmapLevels == std::numeric_limits<uint32_t>::max())
		mipmapLevels = img.GetMipmapCount() -baseMipmap;
	auto totalLayerCount = layerCount +baseLayer;
	if(totalLayerCount >= layoutInfo.layerLayouts.size())
		layoutInfo.layerLayouts.resize(totalLayerCount);

	auto totalMipmapLevels = baseMipmap +mipmapLevels;
	for(auto &layouts : layoutInfo.layerLayouts)
	{
		if(layouts.mipmapLayouts.size() < totalMipmapLevels)
			layouts.mipmapLayouts.resize(totalMipmapLevels,ImageLayout::Undefined);
	}

	for(auto i=baseLayer;i<(baseLayer +layerCount);++i)
	{
		for(auto j=baseMipmap;j<(baseMipmap +mipmapLevels);++j)
			itLayoutInfo->second.layerLayouts.at(i).mipmapLayouts.at(j) = layout;
	}
}
bool prosper::debug::get_last_recorded_image_layout(prosper::ICommandBuffer &cmdBuffer,IImage &img,ImageLayout &layout,uint32_t baseLayer,uint32_t baseMipmap)
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

static bool record_image_barrier(
	prosper::ICommandBuffer &cmdBuffer,prosper::IImage &img,
	prosper::ImageLayout originalLayout,prosper::ImageLayout currentLayout,
	prosper::ImageLayout dstLayout,const prosper::util::ImageSubresourceRange &subresourceRange={}
)
{
	prosper::util::BarrierImageLayout srcInfo {};
	switch(originalLayout)
	{
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		srcInfo = {prosper::PipelineStageFlags::FragmentShaderBit,currentLayout,prosper::AccessFlags::ShaderReadBit};
		break;
	case prosper::ImageLayout::ColorAttachmentOptimal:
		srcInfo = {prosper::PipelineStageFlags::ColorAttachmentOutputBit,currentLayout,prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::ColorAttachmentWriteBit};
		break;
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		srcInfo = {prosper::PipelineStageFlags::LateFragmentTestsBit,currentLayout,prosper::AccessFlags::DepthStencilAttachmentReadBit | prosper::AccessFlags::DepthStencilAttachmentWriteBit};
		break;
	case prosper::ImageLayout::TransferSrcOptimal:
		srcInfo = {prosper::PipelineStageFlags::TransferBit,currentLayout,prosper::AccessFlags::TransferReadBit};
		break;
	case prosper::ImageLayout::TransferDstOptimal:
		srcInfo = {prosper::PipelineStageFlags::TransferBit,currentLayout,prosper::AccessFlags::TransferWriteBit};
		break;
	}

	prosper::util::BarrierImageLayout dstInfo {};
	switch(dstLayout)
	{
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		dstInfo = {prosper::PipelineStageFlags::FragmentShaderBit,dstLayout,prosper::AccessFlags::ShaderReadBit};
		break;
	case prosper::ImageLayout::ColorAttachmentOptimal:
		dstInfo = {prosper::PipelineStageFlags::ColorAttachmentOutputBit,dstLayout,prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::ColorAttachmentWriteBit};
		break;
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		dstInfo = {prosper::PipelineStageFlags::EarlyFragmentTestsBit,dstLayout,prosper::AccessFlags::DepthStencilAttachmentReadBit | prosper::AccessFlags::DepthStencilAttachmentWriteBit};
		break;
	case prosper::ImageLayout::TransferSrcOptimal:
		dstInfo = {prosper::PipelineStageFlags::TransferBit,dstLayout,prosper::AccessFlags::TransferReadBit};
		break;
	case prosper::ImageLayout::TransferDstOptimal:
		dstInfo = {prosper::PipelineStageFlags::TransferBit,dstLayout,prosper::AccessFlags::TransferWriteBit};
		break;
	}
	return cmdBuffer.RecordImageBarrier(img,srcInfo,dstInfo,subresourceRange);
}

bool prosper::ICommandBuffer::RecordImageBarrier(
	IImage &img,PipelineStageFlags srcStageMask,PipelineStageFlags dstStageMask,
	ImageLayout oldLayout,ImageLayout newLayout,AccessFlags srcAccessMask,AccessFlags dstAccessMask,
	uint32_t baseLayer
)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcStageMask;
	barrier.dstStageMask = dstStageMask;

	prosper::util::ImageBarrierInfo imgBarrier {};
	imgBarrier.oldLayout = oldLayout;
	imgBarrier.newLayout = newLayout;
	imgBarrier.srcAccessMask = srcAccessMask;
	imgBarrier.dstAccessMask = dstAccessMask;
	if(baseLayer != std::numeric_limits<uint32_t>::max())
	{
		imgBarrier.subresourceRange.baseArrayLayer = baseLayer;
		imgBarrier.subresourceRange.layerCount = 1u;
	}
	barrier.imageBarriers.push_back(prosper::util::create_image_barrier(img,imgBarrier));

	return RecordPipelineBarrier(barrier);
}
bool prosper::ICommandBuffer::RecordImageBarrier(
	IImage &img,const util::BarrierImageLayout &srcBarrierInfo,const util::BarrierImageLayout &dstBarrierInfo,
	const util::ImageSubresourceRange &subresourceRange
)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcBarrierInfo.stageMask;
	barrier.dstStageMask = dstBarrierInfo.stageMask;
	barrier.imageBarriers.push_back(prosper::util::create_image_barrier(img,srcBarrierInfo,dstBarrierInfo,subresourceRange));
	return RecordPipelineBarrier(barrier);
}
bool prosper::ICommandBuffer::RecordImageBarrier(
	IImage &img,ImageLayout srcLayout,ImageLayout dstLayout,const util::ImageSubresourceRange &subresourceRange
)
{
	return ::record_image_barrier(*this,img,srcLayout,srcLayout,dstLayout,subresourceRange);
}
bool prosper::ICommandBuffer::RecordPostRenderPassImageBarrier(
	IImage &img,
	ImageLayout preRenderPassLayout,ImageLayout postRenderPassLayout,
	const util::ImageSubresourceRange &subresourceRange
)
{
	return ::record_image_barrier(*this,img,preRenderPassLayout,postRenderPassLayout,postRenderPassLayout,subresourceRange);
}
bool prosper::ICommandBuffer::RecordBufferBarrier(
	IBuffer &buf,PipelineStageFlags srcStageMask,PipelineStageFlags dstStageMask,
	AccessFlags srcAccessMask,AccessFlags dstAccessMask,DeviceSize offset,DeviceSize size
)
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
	bufBarrier.offset = buf.GetStartOffset() +offset;
	barrier.bufferBarriers.push_back(prosper::util::create_buffer_barrier(bufBarrier,buf));
	return RecordPipelineBarrier(barrier);
}

/////////////////

struct DebugRenderTargetInfo
{
	std::weak_ptr<prosper::IRenderPass> renderPass;
	uint32_t baseLayer;
	std::weak_ptr<prosper::IImage> image;
	std::weak_ptr<prosper::IFramebuffer> framebuffer;
	std::weak_ptr<prosper::RenderTarget> renderTarget;
};
static std::unordered_map<prosper::IPrimaryCommandBuffer*,DebugRenderTargetInfo> s_wpCurrentRenderTargets = {};
bool prosper::util::get_current_render_pass_target(prosper::IPrimaryCommandBuffer &cmdBuffer,prosper::IRenderPass **outRp,prosper::IImage **outImg,prosper::IFramebuffer **outFb,prosper::RenderTarget **outRt)
{
	auto it = s_wpCurrentRenderTargets.find(&cmdBuffer);
	if(it == s_wpCurrentRenderTargets.end())
		return false;
	auto &dbgInfo = it->second;
	if(outRp)
		*outRp = dbgInfo.renderPass.lock().get();
	if(outImg)
		*outImg = dbgInfo.image.lock().get();
	if(outFb)
		*outFb = dbgInfo.framebuffer.lock().get();
	if(outRt)
		*outRt = dbgInfo.renderTarget.lock().get();
	return true;
}

static bool record_begin_render_pass(prosper::IPrimaryCommandBuffer &cmdBuffer,prosper::RenderTarget &rt,uint32_t *layerId,const std::vector<prosper::ClearValue> &clearValues,prosper::IRenderPass *rp)
{
#ifdef DEBUG_VERBOSE
	auto numAttachments = rt.GetAttachmentCount();
	std::cout<<"[VK] Beginning render pass with "<<numAttachments<<" attachments:"<<std::endl;
	for(auto i=decltype(numAttachments){0u};i<numAttachments;++i)
	{
		auto &tex = rt.GetTexture(i);
		auto img = (tex != nullptr) ? tex->GetImage() : nullptr;
		std::cout<<"\t"<<((img != nullptr) ? img->GetAnvilImage().get_image() : nullptr)<<std::endl;
	}
#endif
	auto *fb = (layerId != nullptr) ? rt.GetFramebuffer(*layerId) : &rt.GetFramebuffer();
	auto &context = rt.GetContext();
	if(context.IsValidationEnabled())
	{
		auto &rpCreateInfo = *static_cast<prosper::VlkRenderPass&>(rt.GetRenderPass()).GetAnvilRenderPass().get_render_pass_create_info();
		auto numAttachments = fb->GetAttachmentCount();
		for(auto i=decltype(numAttachments){0};i<numAttachments;++i)
		{
			auto *imgView = fb->GetAttachment(i);
			if(imgView == nullptr)
				continue;
			auto &img = imgView->GetImage();
			Anvil::AttachmentType attType;
			if(rpCreateInfo.get_attachment_type(i,&attType) == false)
				continue;
			Anvil::ImageLayout initialLayout;
			auto bValid = false;
			switch(attType)
			{
			case Anvil::AttachmentType::COLOR:
				bValid = rpCreateInfo.get_color_attachment_properties(i,nullptr,nullptr,nullptr,nullptr,&initialLayout,nullptr);
				break;
			case Anvil::AttachmentType::DEPTH_STENCIL:
				bValid = rpCreateInfo.get_depth_stencil_attachment_properties(i,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&initialLayout,nullptr);
				break;
			}
			if(bValid == false)
				continue;
			prosper::ImageLayout imgLayout;
			if(prosper::debug::get_last_recorded_image_layout(cmdBuffer,img,imgLayout,(layerId != nullptr) ? *layerId : 0u) == false)
				continue;
			if(imgLayout != static_cast<prosper::ImageLayout>(initialLayout))
			{
				prosper::debug::exec_debug_validation_callback(
					prosper::DebugReportObjectTypeEXT::Image,"Begin render pass: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(static_cast<prosper::VlkImage&>(img).GetAnvilImage().get_image())) +
					" has to be in layout " +vk::to_string(static_cast<vk::ImageLayout>(initialLayout)) +", but is in layout " +
					vk::to_string(static_cast<vk::ImageLayout>(imgLayout)) +"!"
				);
			}
		}
	}
	auto it = s_wpCurrentRenderTargets.find(&cmdBuffer);
	if(it != s_wpCurrentRenderTargets.end())
		s_wpCurrentRenderTargets.erase(it);
	if(rp == nullptr)
		rp = &rt.GetRenderPass();
	auto &tex = rt.GetTexture();
	if(fb == nullptr)
		throw std::runtime_error("Attempted to begin render pass with NULL framebuffer object!");
	if(rp == nullptr)
		throw std::runtime_error("Attempted to begin render pass with NULL render pass object!");
	auto &img = tex.GetImage();
	auto extents = img.GetExtents();
	static_assert(sizeof(prosper::Extent2D) == sizeof(vk::Extent2D));
	auto renderArea = vk::Rect2D(vk::Offset2D(),reinterpret_cast<vk::Extent2D&>(extents));
	s_wpCurrentRenderTargets[&cmdBuffer] = {rp->shared_from_this(),(layerId != nullptr) ? *layerId : std::numeric_limits<uint32_t>::max(),img.shared_from_this(),fb ? fb->shared_from_this() : nullptr,rt.shared_from_this()};
	return static_cast<prosper::VlkPrimaryCommandBuffer&>(cmdBuffer)->record_begin_render_pass(
		clearValues.size(),reinterpret_cast<const VkClearValue*>(clearValues.data()),
		&static_cast<prosper::VlkFramebuffer&>(*fb).GetAnvilFramebuffer(),renderArea,&static_cast<prosper::VlkRenderPass&>(*rp).GetAnvilRenderPass(),Anvil::SubpassContents::INLINE
	);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt,uint32_t layerId,const prosper::ClearValue *clearValue,prosper::IRenderPass *rp)
{
	return ::record_begin_render_pass(*this,rt,&layerId,(clearValue != nullptr) ? std::vector<prosper::ClearValue>{*clearValue} : std::vector<prosper::ClearValue>{},rp);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt,uint32_t layerId,const std::vector<prosper::ClearValue> &clearValues,prosper::IRenderPass *rp)
{
	return ::record_begin_render_pass(*this,rt,&layerId,clearValues,rp);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt,const prosper::ClearValue *clearValue,prosper::IRenderPass *rp)
{
	return ::record_begin_render_pass(*this,rt,nullptr,(clearValue != nullptr) ? std::vector<prosper::ClearValue>{*clearValue} : std::vector<prosper::ClearValue>{},rp);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::RenderTarget &rt,const std::vector<prosper::ClearValue> &clearValues,prosper::IRenderPass *rp)
{
	return ::record_begin_render_pass(*this,rt,nullptr,clearValues,rp);
}
bool prosper::IPrimaryCommandBuffer::RecordBeginRenderPass(prosper::IImage &img,prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const std::vector<prosper::ClearValue> &clearValues)
{
	s_wpCurrentRenderTargets[this] = {rp.shared_from_this(),0u,img.shared_from_this(),fb.shared_from_this(),std::weak_ptr<prosper::RenderTarget>{}};

	auto extents = img.GetExtents();
	static_assert(sizeof(prosper::Extent2D) == sizeof(vk::Extent2D));
	auto renderArea = prosper::Rect2D(prosper::Offset2D(),reinterpret_cast<prosper::Extent2D&>(extents));
	static_assert(sizeof(prosper::Rect2D) == sizeof(VkRect2D));
	return static_cast<VlkPrimaryCommandBuffer&>(*this).GetAnvilCommandBuffer().record_begin_render_pass(
		clearValues.size(),reinterpret_cast<const VkClearValue*>(clearValues.data()),
		&static_cast<VlkFramebuffer&>(fb).GetAnvilFramebuffer(),reinterpret_cast<VkRect2D&>(renderArea),&static_cast<VlkRenderPass&>(rp).GetAnvilRenderPass(),Anvil::SubpassContents::INLINE
	);
}

bool prosper::VlkCommandBuffer::RecordClearImage(IImage &img,ImageLayout layout,const std::array<float,4> &clearColor,const util::ClearImageInfo &clearImageInfo)
{
	// TODO
	//if(bufferDst.GetContext().IsValidationEnabled() && cmdBuffer.get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && get_current_render_pass_target(static_cast<Anvil::PrimaryCommandBuffer&>(cmdBuffer)) != nullptr)
	//	throw std::logic_error("Attempted to copy image to buffer while render pass is active!");
	// if(cmdBuffer.GetContext().IsValidationEnabled() && is_compressed_format(img.get_create_info_ptr()->get_format()))
	// 	throw std::logic_error("Attempted to clear image with compressed image format! This is not allowed!");

	prosper::util::ImageSubresourceRange resourceRange {};
	apply_range(clearImageInfo.subresourceRange,resourceRange,img);

	auto anvResourceRange = to_anvil_subresource_range(resourceRange,img);
	prosper::ClearColorValue clearColorVal {clearColor};
	return m_cmdBuffer->record_clear_color_image(
		&*static_cast<VlkImage&>(img),static_cast<Anvil::ImageLayout>(layout),reinterpret_cast<VkClearColorValue*>(&clearColorVal),
		1u,&anvResourceRange
	);
}
bool prosper::VlkCommandBuffer::RecordClearImage(IImage &img,ImageLayout layout,float clearDepth,const util::ClearImageInfo &clearImageInfo)
{
	// TODO
	//if(bufferDst.GetContext().IsValidationEnabled() && cmdBuffer.get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && get_current_render_pass_target(static_cast<Anvil::PrimaryCommandBuffer&>(cmdBuffer)) != nullptr)
	//	throw std::logic_error("Attempted to copy image to buffer while render pass is active!");
	// if(cmdBuffer.GetContext().IsValidationEnabled() && is_compressed_format(img.get_create_info_ptr()->get_format()))
	// 	throw std::logic_error("Attempted to clear image with compressed image format! This is not allowed!");

	prosper::util::ImageSubresourceRange resourceRange {};
	apply_range(clearImageInfo.subresourceRange,resourceRange,img);

	auto anvResourceRange = to_anvil_subresource_range(resourceRange,img);
	prosper::ClearDepthStencilValue clearDepthValue {clearDepth};
	return m_cmdBuffer->record_clear_depth_stencil_image(
		&*static_cast<VlkImage&>(img),static_cast<Anvil::ImageLayout>(layout),reinterpret_cast<VkClearDepthStencilValue*>(&clearDepthValue),
		1u,&anvResourceRange
	);
}

bool prosper::VlkPrimaryCommandBuffer::RecordEndRenderPass()
{
#ifdef DEBUG_VERBOSE
	std::cout<<"[VK] Ending render pass..."<<std::endl;
#endif
	auto it = s_wpCurrentRenderTargets.find(this);
	if(it != s_wpCurrentRenderTargets.end())
	{
		auto rp = it->second.renderPass.lock();
		auto fb = it->second.framebuffer.lock();
		if(rp && fb)
		{
			auto &context = rp->GetContext();
			auto rt = it->second.renderTarget.lock();
			if(context.IsValidationEnabled())
			{
				auto &rpCreateInfo = *static_cast<VlkRenderPass*>(rp.get())->GetAnvilRenderPass().get_render_pass_create_info();
				auto numAttachments = fb->GetAttachmentCount();
				for(auto i=decltype(numAttachments){0};i<numAttachments;++i)
				{
					auto *imgView = fb->GetAttachment(i);
					if(!imgView)
						continue;
					auto &img = imgView->GetImage();
					Anvil::AttachmentType attType;
					if(rpCreateInfo.get_attachment_type(i,&attType) == false)
						continue;
					Anvil::ImageLayout finalLayout;
					auto bValid = false;
					switch(attType)
					{
					case Anvil::AttachmentType::COLOR:
						bValid = rpCreateInfo.get_color_attachment_properties(i,nullptr,nullptr,nullptr,nullptr,nullptr,&finalLayout);
						break;
					case Anvil::AttachmentType::DEPTH_STENCIL:
						bValid = rpCreateInfo.get_depth_stencil_attachment_properties(i,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&finalLayout);
						break;
					}
					if(bValid == false)
						continue;
					auto layerCount = umath::min(imgView->GetLayerCount(),fb->GetLayerCount());
					auto baseLayer = imgView->GetBaseLayer();
					prosper::debug::set_last_recorded_image_layout(*this,img,static_cast<prosper::ImageLayout>(finalLayout),baseLayer,layerCount);
				}
			}
		}
		s_wpCurrentRenderTargets.erase(it);
	}
	return (*this)->record_end_render_pass();
}
