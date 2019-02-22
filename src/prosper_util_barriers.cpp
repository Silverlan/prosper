/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_util.hpp"
#include "debug/prosper_debug.hpp"
#include <config.h>
#include <misc/image_create_info.h>
#include <sharedutils/util.h>

#ifdef DEBUG_VERBOSE
#include <iostream>
#endif

void prosper::util::ImageSubresourceRange::ApplyRange(Anvil::ImageSubresourceRange &vkRange,Anvil::Image &img) const
{
	vkRange.base_mip_level = baseMipLevel;
	vkRange.level_count = levelCount;
	vkRange.base_array_layer = baseArrayLayer;
	vkRange.layer_count = layerCount;

	if(vkRange.level_count == std::numeric_limits<uint32_t>::max())
		vkRange.level_count = img.get_n_mipmaps() -baseMipLevel;
	if(vkRange.layer_count == std::numeric_limits<uint32_t>::max())
		vkRange.layer_count = img.get_create_info_ptr()->get_n_layers() -baseArrayLayer;
}
prosper::util::BarrierImageLayout::BarrierImageLayout(Anvil::PipelineStageFlags pipelineStageFlags,Anvil::ImageLayout imageLayout,Anvil::AccessFlags accessFlags)
	: stageMask(pipelineStageFlags),layout(imageLayout),accessMask(accessFlags)
{}

Anvil::BufferBarrier prosper::util::create_buffer_barrier(const prosper::util::BufferBarrierInfo &barrierInfo,Buffer &buffer)
{
	return Anvil::BufferBarrier(
		barrierInfo.srcAccessMask,barrierInfo.dstAccessMask,
		VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,&buffer.GetBaseAnvilBuffer(),barrierInfo.offset,
		umath::min(barrierInfo.offset +barrierInfo.size,buffer.GetSize()) -barrierInfo.offset
	);
}

Anvil::ImageBarrier prosper::util::create_image_barrier(Anvil::Image &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,const ImageSubresourceRange &subresourceRange)
{
	Anvil::ImageSubresourceRange resourceRange {};
	resourceRange.aspect_mask = get_aspect_mask(img.get_create_info_ptr()->get_format());
	subresourceRange.ApplyRange(resourceRange,img);
	return Anvil::ImageBarrier(
		srcBarrierInfo.accessMask,dstBarrierInfo.accessMask,
		srcBarrierInfo.layout,dstBarrierInfo.layout,
		VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,&img,resourceRange
	);
}

Anvil::ImageBarrier prosper::util::create_image_barrier(Anvil::Image &img,const prosper::util::ImageBarrierInfo &barrierInfo)
{
	Anvil::ImageSubresourceRange resourceRange {};
	resourceRange.aspect_mask = get_aspect_mask(img);
	barrierInfo.subresourceRange.ApplyRange(resourceRange,img);
	return Anvil::ImageBarrier(
		barrierInfo.srcAccessMask,barrierInfo.dstAccessMask,
		barrierInfo.oldLayout,barrierInfo.newLayout,
		barrierInfo.srcQueueFamilyIndex,barrierInfo.dstQueueFamilyIndex,&img,resourceRange
	);
}

struct DebugImageLayouts
{
	std::vector<Anvil::ImageLayout> mipmapLayouts;
};
struct DebugImageLayoutInfo
{
	std::vector<DebugImageLayouts> layerLayouts;
};
static std::unique_ptr<std::unordered_map<Anvil::CommandBufferBase*,std::unordered_map<Anvil::Image*,DebugImageLayoutInfo>>> s_lastRecordedImageLayouts = nullptr;
static std::function<void(vk::DebugReportObjectTypeEXT,const std::string&)> s_debugCallback = nullptr;
void prosper::debug::set_debug_validation_callback(const std::function<void(vk::DebugReportObjectTypeEXT,const std::string&)> &callback) {s_debugCallback = callback;}
void prosper::debug::exec_debug_validation_callback(vk::DebugReportObjectTypeEXT objType,const std::string &msg)
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
	s_lastRecordedImageLayouts = std::make_unique<std::unordered_map<Anvil::CommandBufferBase*,std::unordered_map<Anvil::Image*,DebugImageLayoutInfo>>>();
}
void prosper::debug::set_last_recorded_image_layout(
	Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout layout,uint32_t baseLayer,uint32_t layerCount,uint32_t baseMipmap,uint32_t mipmapLevels
)
{
#ifdef DEBUG_VERBOSE
	std::cout<<"[VK] Setting image layout for image "<<img.get_image()<<" to: "<<vk::to_string(layout)<<std::endl;
#endif
	//if(debug::get_image_from_anvil_image(img)->GetDebugName() == "prepass_depth_normal_rt_tex0_resolved_img")
	//	std::cout<<"prepass_depth_normal_rt_tex0_resolved_img changed to "<<vk::to_string(layout)<<"!"<<std::endl;
	auto it = s_lastRecordedImageLayouts->find(&cmdBuffer);
	if(it == s_lastRecordedImageLayouts->end())
		it = s_lastRecordedImageLayouts->insert(std::make_pair(&cmdBuffer,std::unordered_map<Anvil::Image*,DebugImageLayoutInfo>{})).first;

	auto itLayoutInfo = it->second.find(&img);
	if(itLayoutInfo == it->second.end())
		itLayoutInfo = it->second.insert(std::make_pair(&img,DebugImageLayoutInfo{})).first;
	auto &layoutInfo = itLayoutInfo->second;
	if(layerCount == std::numeric_limits<uint32_t>::max())
		layerCount = img.get_create_info_ptr()->get_n_layers() -baseLayer;
	if(mipmapLevels == std::numeric_limits<uint32_t>::max())
		mipmapLevels = img.get_n_mipmaps() -baseMipmap;
	auto totalLayerCount = layerCount +baseLayer;
	if(totalLayerCount >= layoutInfo.layerLayouts.size())
	{
		layoutInfo.layerLayouts.resize(totalLayerCount);
		auto totalMipmapLevels = baseMipmap +mipmapLevels;
		for(auto &layouts : layoutInfo.layerLayouts)
			layouts.mipmapLayouts.resize(totalMipmapLevels,Anvil::ImageLayout::UNDEFINED);
	}
	for(auto i=baseLayer;i<(baseLayer +layerCount);++i)
	{
		for(auto j=baseMipmap;j<(baseMipmap +mipmapLevels);++j)
			itLayoutInfo->second.layerLayouts.at(i).mipmapLayouts.at(j) = layout;
	}
}
bool prosper::debug::get_last_recorded_image_layout(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout &layout,uint32_t baseLayer,uint32_t baseMipmap)
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
bool prosper::util::record_pipeline_barrier(Anvil::CommandBufferBase &cmdBuffer,const PipelineBarrierInfo &barrierInfo)
{
	if(s_lastRecordedImageLayouts != nullptr)
	{
		auto it = s_lastRecordedImageLayouts->find(&cmdBuffer);
		if(it == s_lastRecordedImageLayouts->end())
			it = s_lastRecordedImageLayouts->insert(std::make_pair(&cmdBuffer,std::unordered_map<Anvil::Image*,DebugImageLayoutInfo>{})).first;
		else
		{
			for(auto &imgBarrier : barrierInfo.imageBarriers)
			{
				auto itImg = it->second.find(imgBarrier.image_ptr);
				if(itImg == it->second.end())
					continue;
				auto &layoutInfo = itImg->second;
				auto &range = imgBarrier.subresource_range;
				for(auto i=range.base_array_layer;i<(range.base_array_layer +range.layer_count);++i)
				{
					if(i >= layoutInfo.layerLayouts.size())
						break;
					auto &layerLayout = layoutInfo.layerLayouts.at(i);
					for(auto j=range.base_mip_level;j<(range.base_mip_level +range.layer_count);++j)
					{
						if(j >= layerLayout.mipmapLayouts.size())
							break;
						auto layout = layerLayout.mipmapLayouts.at(j);
						if(layout != imgBarrier.old_layout && imgBarrier.old_layout != Anvil::ImageLayout::UNDEFINED)
						{
							debug::exec_debug_validation_callback(
								vk::DebugReportObjectTypeEXT::eImage,"Record pipeline barrier: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgBarrier.image_ptr->get_image())) +
								" at array layer " +std::to_string(i) +", mipmap " +std::to_string(j) +" has current layout " +vk::to_string(static_cast<vk::ImageLayout>(layout)) +
								", but should have " +vk::to_string(static_cast<vk::ImageLayout>(imgBarrier.old_layout)) +" according to barrier info!"
							);
						}
					}
				}
			}
		}
		for(auto &imgBarrier : barrierInfo.imageBarriers)
		{
			debug::set_last_recorded_image_layout(
				cmdBuffer,*imgBarrier.image_ptr,static_cast<Anvil::ImageLayout>(imgBarrier.new_layout),
				imgBarrier.subresource_range.base_array_layer,imgBarrier.subresource_range.layer_count,
				imgBarrier.subresource_range.base_mip_level,imgBarrier.subresource_range.level_count
			);
		}
	}

	return cmdBuffer.record_pipeline_barrier(
		barrierInfo.srcStageMask,barrierInfo.dstStageMask,
		Anvil::DependencyFlagBits::NONE,0u,nullptr,barrierInfo.bufferBarriers.size(),barrierInfo.bufferBarriers.data(),barrierInfo.imageBarriers.size(),barrierInfo.imageBarriers.data()
	);
}

bool prosper::util::record_buffer_barrier(
	Anvil::CommandBufferBase &cmdBuffer,Buffer &buf,Anvil::PipelineStageFlags srcStageMask,Anvil::PipelineStageFlags dstStageMask,
	Anvil::AccessFlags srcAccessMask,Anvil::AccessFlags dstAccessMask,vk::DeviceSize offset,vk::DeviceSize size
)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcStageMask;
	barrier.dstStageMask = dstStageMask;

	if(size == std::numeric_limits<vk::DeviceSize>::max())
		size = buf.GetSize();

	prosper::util::BufferBarrierInfo bufBarrier {};
	bufBarrier.srcAccessMask = srcAccessMask;
	bufBarrier.dstAccessMask = dstAccessMask;
	bufBarrier.size = size;
	bufBarrier.offset = buf.GetStartOffset() +offset;
	barrier.bufferBarriers.push_back(prosper::util::create_buffer_barrier(bufBarrier,buf));
	return prosper::util::record_pipeline_barrier(cmdBuffer,barrier);
}

bool prosper::util::record_image_barrier(
	Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,const BarrierImageLayout &srcBarrierInfo,const BarrierImageLayout &dstBarrierInfo,
	const ImageSubresourceRange &subresourceRange
)
{
	prosper::util::PipelineBarrierInfo barrier {};
	barrier.srcStageMask = srcBarrierInfo.stageMask;
	barrier.dstStageMask = dstBarrierInfo.stageMask;
	barrier.imageBarriers.push_back(prosper::util::create_image_barrier(img,srcBarrierInfo,dstBarrierInfo,subresourceRange));
	return prosper::util::record_pipeline_barrier(cmdBuffer,barrier);
}

bool prosper::util::record_image_barrier(
	Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::ImageLayout srcLayout,Anvil::ImageLayout dstLayout,
	const ImageSubresourceRange &subresourceRange
)
{
	BarrierImageLayout srcInfo {};
	switch(srcLayout)
	{
		case Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL:
			srcInfo = {Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,srcLayout,Anvil::AccessFlagBits::SHADER_READ_BIT};
			break;
		case Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
			srcInfo = {Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,srcLayout,Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT};
			break;
		case Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			srcInfo = {Anvil::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,srcLayout,Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
			break;
		case Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL:
			srcInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,srcLayout,Anvil::AccessFlagBits::TRANSFER_READ_BIT};
			break;
		case Anvil::ImageLayout::TRANSFER_DST_OPTIMAL:
			srcInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,srcLayout,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT};
			break;
	}

	BarrierImageLayout dstInfo {};
	switch(dstLayout)
	{
		case Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL:
			dstInfo = {Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,dstLayout,Anvil::AccessFlagBits::SHADER_READ_BIT};
			break;
		case Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
			dstInfo = {Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,dstLayout,Anvil::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT};
			break;
		case Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dstInfo = {Anvil::PipelineStageFlagBits::EARLY_FRAGMENT_TESTS_BIT,dstLayout,Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT | Anvil::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
			break;
		case Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL:
			dstInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,dstLayout,Anvil::AccessFlagBits::TRANSFER_READ_BIT};
			break;
		case Anvil::ImageLayout::TRANSFER_DST_OPTIMAL:
			dstInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,dstLayout,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT};
			break;
	}
	return record_image_barrier(cmdBuffer,img,srcInfo,dstInfo,subresourceRange);
}

bool prosper::util::record_image_barrier(
	Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img,Anvil::PipelineStageFlags srcStageMask,Anvil::PipelineStageFlags dstStageMask,
	Anvil::ImageLayout oldLayout,Anvil::ImageLayout newLayout,Anvil::AccessFlags srcAccessMask,Anvil::AccessFlags dstAccessMask,
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
	
	return prosper::util::record_pipeline_barrier(cmdBuffer,barrier);
}
