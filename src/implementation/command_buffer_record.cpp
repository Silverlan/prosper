// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cinttypes>

#include <sstream>

module pragma.prosper;

import :command_buffer;

bool prosper::ICommandBuffer::RecordCopyBuffer(const util::BufferCopy &copyInfo, IBuffer &bufferSrc, IBuffer &bufferDst)
{
	auto ci = copyInfo;
	ci.srcOffset += bufferSrc.GetStartOffset();
	ci.dstOffset += bufferDst.GetStartOffset();
	return DoRecordCopyBuffer(ci, bufferSrc, bufferDst);
}
bool prosper::ICommandBuffer::RecordClearAttachment(IImage &img, const std::array<float, 4> &clearColor, uint32_t attId) { return RecordClearAttachment(img, clearColor, attId, 0u, img.GetLayerCount()); }
bool prosper::ICommandBuffer::RecordCopyImage(const util::CopyInfo &copyInfo, IImage &imgSrc, IImage &imgDst)
{
	// TODO
	//if(bufferDst.GetContext().IsValidationEnabled() && cmdBuffer.get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && get_current_render_pass_target(static_cast<Anvil::PrimaryCommandBuffer&>(cmdBuffer)) != nullptr)
	//	throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	auto width = copyInfo.width;
	if(width == std::numeric_limits<decltype(width)>::max())
		width = imgSrc.GetWidth();
	auto height = copyInfo.height;
	if(height == std::numeric_limits<decltype(height)>::max())
		height = imgSrc.GetHeight();
	return DoRecordCopyImage(copyInfo, imgSrc, imgDst, width, height);
}
bool prosper::ICommandBuffer::RecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo, IBuffer &bufferSrc, IImage &imgDst) { return DoRecordCopyBufferToImage(copyInfo, bufferSrc, imgDst); }
bool prosper::ICommandBuffer::RecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo, IImage &imgSrc, ImageLayout srcImageLayout, IBuffer &bufferDst) { return DoRecordCopyImageToBuffer(copyInfo, imgSrc, srcImageLayout, bufferDst); }

bool prosper::ICommandBuffer::RecordUpdateGenericShaderReadBuffer(IBuffer &buffer, uint64_t offset, uint64_t size, const void *data)
{
	if(RecordBufferBarrier(buffer, PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::GeometryShaderBit, PipelineStageFlags::TransferBit, AccessFlags::ShaderReadBit, AccessFlags::TransferWriteBit, offset,
	     size)
	  == false)
		return false;
	if(RecordUpdateBuffer(buffer, offset, size, data) == false)
		return false;
	return RecordBufferBarrier(buffer, PipelineStageFlags::TransferBit, PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::GeometryShaderBit, AccessFlags::TransferWriteBit, AccessFlags::ShaderReadBit,
	  offset, size);
}
bool prosper::ICommandBuffer::RecordBlitImage(const util::BlitInfo &blitInfo, IImage &imgSrc, IImage &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled()) {
		ImageLayout lastImgLayout;
		for(auto i = blitInfo.srcSubresourceLayer.baseArrayLayer; i < (blitInfo.srcSubresourceLayer.baseArrayLayer + blitInfo.srcSubresourceLayer.layerCount); ++i) {
			if(prosper::debug::get_last_recorded_image_layout(*this, imgSrc, lastImgLayout, i, blitInfo.srcSubresourceLayer.mipLevel) && lastImgLayout != ImageLayout::TransferSrcOptimal) {
				debug::exec_debug_validation_callback(GetContext(), prosper::DebugReportObjectTypeEXT::Image,
				  "Blit: Source image 0x" + ::umath::to_hex_string(reinterpret_cast<uint64_t>(&imgSrc)) + " for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " + prosper::util::to_string(lastImgLayout) + "!");
			}
		}
		for(auto i = blitInfo.dstSubresourceLayer.baseArrayLayer; i < (blitInfo.dstSubresourceLayer.baseArrayLayer + blitInfo.dstSubresourceLayer.layerCount); ++i) {
			if(prosper::debug::get_last_recorded_image_layout(*this, imgDst, lastImgLayout, i, blitInfo.dstSubresourceLayer.mipLevel) && lastImgLayout != ImageLayout::TransferDstOptimal) {
				debug::exec_debug_validation_callback(GetContext(), prosper::DebugReportObjectTypeEXT::Image,
				  "Blit: Destination image 0x" + ::umath::to_hex_string(reinterpret_cast<uint64_t>(&imgDst)) + " for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " + util::to_string(lastImgLayout) + "!");
			}
		}
	}
	auto srcMipLevel = blitInfo.srcSubresourceLayer.mipLevel;
	auto srcExtents = imgSrc.GetExtents(srcMipLevel);
	std::array<Offset3D, 2> srcOffsets = {Offset3D {blitInfo.offsetSrc.at(0), blitInfo.offsetSrc.at(1), 0}, Offset3D {static_cast<int32_t>(srcExtents.width), static_cast<int32_t>(srcExtents.height), 1}};
	if(blitInfo.extentsSrc.has_value()) {
		srcOffsets.at(1).x = blitInfo.extentsSrc->width;
		srcOffsets.at(1).y = blitInfo.extentsSrc->height;
	}
	auto dstMipLevel = blitInfo.dstSubresourceLayer.mipLevel;
	auto dstExtents = imgDst.GetExtents(dstMipLevel);
	std::array<Offset3D, 2> dstOffsets = {Offset3D {blitInfo.offsetDst.at(0), blitInfo.offsetDst.at(1), 0}, Offset3D {static_cast<int32_t>(dstExtents.width), static_cast<int32_t>(dstExtents.height), 1}};
	if(blitInfo.extentsDst.has_value()) {
		dstOffsets.at(1).x = blitInfo.extentsDst->width;
		dstOffsets.at(1).y = blitInfo.extentsDst->height;
	}
	return DoRecordBlitImage(blitInfo, imgSrc, imgDst, srcOffsets, dstOffsets);
}
bool prosper::ICommandBuffer::RecordResolveImage(IImage &imgSrc, IImage &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled()) {
		ImageLayout lastImgLayout;
		if(prosper::debug::get_last_recorded_image_layout(*this, imgSrc, lastImgLayout) && lastImgLayout != ImageLayout::TransferSrcOptimal) {
			debug::exec_debug_validation_callback(GetContext(), prosper::DebugReportObjectTypeEXT::Image,
			  "Resolve: Source image 0x" + ::umath::to_hex_string(reinterpret_cast<uint64_t>(&imgSrc)) + " for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " + util::to_string(lastImgLayout) + "!");
		}
		if(prosper::debug::get_last_recorded_image_layout(*this, imgDst, lastImgLayout) && lastImgLayout != ImageLayout::TransferDstOptimal) {
			debug::exec_debug_validation_callback(GetContext(), prosper::DebugReportObjectTypeEXT::Image,
			  "Resolve: Destination image 0x" + ::umath::to_hex_string(reinterpret_cast<uint64_t>(&imgDst)) + " for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " + util::to_string(lastImgLayout) + "!");
		}
	}
	auto srcAspectMask = util::get_aspect_mask(imgSrc);
	auto dstAspectMask = util::get_aspect_mask(imgDst);

	auto srcExtents = imgSrc.GetExtents(0u);
	util::ImageSubresourceLayers srcLayer {srcAspectMask, 0, 0, 1};
	util::ImageSubresourceLayers destLayer {dstAspectMask, 0, 0, 1};
	util::ImageResolve resolve {srcLayer, Offset3D {0, 0, 0}, destLayer, Offset3D {0, 0, 0}, Extent3D {srcExtents.width, srcExtents.height, 1}};
	return DoRecordResolveImage(imgSrc, imgDst, resolve);
}
bool prosper::ICommandBuffer::RecordBlitTexture(prosper::Texture &texSrc, IImage &imgDst)
{
	if(texSrc.IsMSAATexture() == false)
		return RecordBlitImage({}, texSrc.GetImage(), imgDst);
	return RecordResolveImage(texSrc.GetImage(), imgDst);
}
bool prosper::ICommandBuffer::RecordGenerateMipmaps(IImage &img, ImageLayout currentLayout, AccessFlags srcAccessMask, PipelineStageFlags srcStage)
{
	auto blitInfo = prosper::util::BlitInfo {};
	auto numMipmaps = img.GetMipmapCount();
	auto numLayers = img.GetLayerCount();
	prosper::util::PipelineBarrierInfo barrierInfo {};
	barrierInfo.srcStageMask = srcStage;
	barrierInfo.dstStageMask = PipelineStageFlags::TransferBit;

	prosper::util::ImageBarrierInfo imgBarrierInfo {};
	imgBarrierInfo.subresourceRange.levelCount = 1u;
	imgBarrierInfo.subresourceRange.baseArrayLayer = 0u;
	imgBarrierInfo.subresourceRange.layerCount = numLayers;
	imgBarrierInfo.oldLayout = currentLayout;
	imgBarrierInfo.newLayout = ImageLayout::TransferSrcOptimal;
	imgBarrierInfo.srcAccessMask = srcAccessMask;
	imgBarrierInfo.dstAccessMask = AccessFlags::TransferReadBit;
	imgBarrierInfo.subresourceRange.baseMipLevel = 0u;

	barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers) {util::create_image_barrier(img, imgBarrierInfo)};
	if(RecordPipelineBarrier(barrierInfo) == false) // Move first mipmap into transfer-src layout
		return false;

	imgBarrierInfo.subresourceRange.baseMipLevel = 1u;
	imgBarrierInfo.subresourceRange.levelCount = img.GetMipmapCount() - 1;
	imgBarrierInfo.newLayout = ImageLayout::TransferDstOptimal;
	imgBarrierInfo.dstAccessMask = AccessFlags::TransferWriteBit;
	barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers) {util::create_image_barrier(img, imgBarrierInfo)};
	if(RecordPipelineBarrier(barrierInfo) == false) // Move other mipmaps into transfer-dst layout
		return false;

	barrierInfo.srcStageMask = PipelineStageFlags::TransferBit;

	imgBarrierInfo.subresourceRange.levelCount = 1u;
	imgBarrierInfo.subresourceRange.layerCount = 1u;
	imgBarrierInfo.oldLayout = ImageLayout::TransferDstOptimal;
	imgBarrierInfo.newLayout = ImageLayout::TransferSrcOptimal;
	imgBarrierInfo.srcAccessMask = AccessFlags::TransferWriteBit;
	imgBarrierInfo.dstAccessMask = AccessFlags::TransferReadBit;

	for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
		imgBarrierInfo.subresourceRange.baseArrayLayer = iLayer;
		for(auto i = decltype(numMipmaps) {1}; i < numMipmaps; ++i) {
			blitInfo.srcSubresourceLayer.mipLevel = i - 1u;
			blitInfo.dstSubresourceLayer.mipLevel = i;

			blitInfo.srcSubresourceLayer.baseArrayLayer = iLayer;
			blitInfo.dstSubresourceLayer.baseArrayLayer = iLayer;

			imgBarrierInfo.subresourceRange.baseMipLevel = i;
			barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers) {util::create_image_barrier(img, imgBarrierInfo)};
			if(RecordBlitImage(blitInfo, img, img) == false || RecordPipelineBarrier(barrierInfo) == false)
				return false;
		}
	}
	return RecordImageBarrier(img, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);
}
