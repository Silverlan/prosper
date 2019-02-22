/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_util.hpp"
#include <misc/image_create_info.h>
#include <sharedutils/util.h>

bool prosper::util::record_blit_texture(Anvil::CommandBufferBase &cmdBuffer,prosper::Texture &texSrc,Anvil::Image &imgDst)
{
	if(texSrc.IsMSAATexture() == false)
		return record_blit_image(cmdBuffer,{},texSrc.GetImage()->GetAnvilImage(),imgDst);
	return record_resolve_image(cmdBuffer,texSrc.GetImage()->GetAnvilImage(),imgDst);
}

bool prosper::util::record_resolve_image(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &imgSrc,Anvil::Image &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled())
	{
		Anvil::ImageLayout lastImgLayout;
		if(prosper::debug::get_last_recorded_image_layout(cmdBuffer,imgSrc,lastImgLayout) && lastImgLayout != Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL)
		{
			debug::exec_debug_validation_callback(
				vk::DebugReportObjectTypeEXT::eImage,"Resolve: Source image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgSrc.get_image())) +
				" for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " +
				to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
			);
		}
		if(prosper::debug::get_last_recorded_image_layout(cmdBuffer,imgDst,lastImgLayout) && lastImgLayout != Anvil::ImageLayout::TRANSFER_DST_OPTIMAL)
		{
			debug::exec_debug_validation_callback(
				vk::DebugReportObjectTypeEXT::eImage,"Resolve: Destination image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgDst.get_image())) +
				" for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " +
				to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
			);
		}
	}
	auto srcAspectMask = get_aspect_mask(imgSrc);
	auto dstAspectMask = get_aspect_mask(imgDst);

	auto srcExtents = imgSrc.get_image_extent_2D(0u);
	Anvil::ImageSubresourceLayers srcLayer {srcAspectMask,0,0,1};
	Anvil::ImageSubresourceLayers destLayer {dstAspectMask,0,0,1};
	Anvil::ImageResolve resolve {srcLayer,vk::Offset3D{0,0,0},destLayer,vk::Offset3D{0,0,0},vk::Extent3D{srcExtents.width,srcExtents.height,1}};
	return cmdBuffer.record_resolve_image(&imgSrc,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,&imgDst,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,1u,&resolve);
}

bool prosper::util::record_blit_image(Anvil::CommandBufferBase &cmdBuffer,const BlitInfo &blitInfo,Anvil::Image &imgSrc,Anvil::Image &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled())
	{
		Anvil::ImageLayout lastImgLayout;
		for(auto i=blitInfo.srcSubresourceLayer.base_array_layer;i<(blitInfo.srcSubresourceLayer.base_array_layer +blitInfo.srcSubresourceLayer.layer_count);++i)
		{
			if(prosper::debug::get_last_recorded_image_layout(cmdBuffer,imgSrc,lastImgLayout,i,blitInfo.srcSubresourceLayer.mip_level) && lastImgLayout != Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL)
			{
				debug::exec_debug_validation_callback(
					vk::DebugReportObjectTypeEXT::eImage,"Blit: Source image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgSrc.get_image())) +
					" for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " +
					to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
				);
			}
		}
		for(auto i=blitInfo.dstSubresourceLayer.base_array_layer;i<(blitInfo.dstSubresourceLayer.base_array_layer +blitInfo.dstSubresourceLayer.layer_count);++i)
		{
			if(prosper::debug::get_last_recorded_image_layout(cmdBuffer,imgDst,lastImgLayout,i,blitInfo.dstSubresourceLayer.mip_level) && lastImgLayout != Anvil::ImageLayout::TRANSFER_DST_OPTIMAL)
			{
				debug::exec_debug_validation_callback(
					vk::DebugReportObjectTypeEXT::eImage,"Blit: Destination image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(imgDst.get_image())) +
					" for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " +
					to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
				);
			}
		}
	}
	auto srcMipLevel = blitInfo.srcSubresourceLayer.mip_level;
	auto srcExtents = imgSrc.get_image_extent_2D(srcMipLevel);
	std::array<vk::Offset3D,2> srcOffsets = {
		vk::Offset3D{0,0,0},
		vk::Offset3D{
			static_cast<int32_t>(srcExtents.width),
			static_cast<int32_t>(srcExtents.height),
			1
		}
	};
	auto dstMipLevel = blitInfo.dstSubresourceLayer.mip_level;
	auto dstExtents = imgDst.get_image_extent_2D(dstMipLevel);
	std::array<vk::Offset3D,2> dstOffsets = {
		vk::Offset3D{0,0,0},
		vk::Offset3D{
			static_cast<int32_t>(dstExtents.width),
			static_cast<int32_t>(dstExtents.height),
			1
		}
	};
	Anvil::ImageBlit blit {};
	blit.src_subresource = blitInfo.srcSubresourceLayer;
	blit.src_offsets[0] = srcOffsets.at(0);
	blit.src_offsets[1] = srcOffsets.at(1);
	blit.dst_subresource = blitInfo.dstSubresourceLayer;
	blit.dst_offsets[0] = dstOffsets.at(0);
	blit.dst_offsets[1] = dstOffsets.at(1);
	auto bDepth = is_depth_format(imgSrc.get_create_info_ptr()->get_format());
	blit.src_subresource.aspect_mask = blit.dst_subresource.aspect_mask = (bDepth == true) ? Anvil::ImageAspectFlagBits::DEPTH_BIT : Anvil::ImageAspectFlagBits::COLOR_BIT;
	return cmdBuffer.record_blit_image(
		&imgSrc,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
		&imgDst,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
		1u,&blit,
		bDepth ? Anvil::Filter::NEAREST : Anvil::Filter::LINEAR
	);
}
