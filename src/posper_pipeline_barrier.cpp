/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <wrappers/command_buffer.h>
#include <misc/types.h>
#include "stdafx_prosper.h"
#include "vk_command_buffer.hpp"
#include "buffers/vk_buffer.hpp"
#include "image/vk_image.hpp"
#include "prosper_util.hpp"

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
#if 0
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
								vk::DebugReportObjectTypeEXT::eImage,"Record pipeline barrier: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgBarrier.image)) +
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
#endif
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
