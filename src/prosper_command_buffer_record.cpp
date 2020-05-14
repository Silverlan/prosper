/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sstream>
#include "prosper_context.hpp"
#include "prosper_command_buffer.hpp"
#include "prosper_util.hpp"
#include "image/prosper_image.hpp"
#include "image/prosper_image_view.hpp"
#include "image/prosper_texture.hpp"
#include "image/prosper_render_target.hpp"
#include "image/vk_image.hpp"
#include "vk_command_buffer.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "buffers/prosper_buffer.hpp"
#include "buffers/vk_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "prosper_util.hpp"
#include <sharedutils/util.h>
#include <wrappers/command_buffer.h>
#include <wrappers/image.h>

bool prosper::ICommandBuffer::RecordCopyBuffer(const util::BufferCopy &copyInfo,IBuffer &bufferSrc,IBuffer &bufferDst)
{
	auto ci = copyInfo;
	ci.srcOffset += bufferSrc.GetStartOffset();
	ci.dstOffset += bufferDst.GetStartOffset();
	static_assert(sizeof(util::BufferCopy) == sizeof(Anvil::BufferCopy));
	return DoRecordCopyBuffer(ci,bufferSrc,bufferDst);
}
bool prosper::ICommandBuffer::RecordClearAttachment(IImage &img,const std::array<float,4> &clearColor,uint32_t attId)
{
	return RecordClearAttachment(img,clearColor,attId,0u,img.GetLayerCount());
}
bool prosper::ICommandBuffer::RecordCopyImage(const util::CopyInfo &copyInfo,IImage &imgSrc,IImage &imgDst)
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
	return DoRecordCopyImage(copyInfo,imgSrc,imgDst,width,height);
}
bool prosper::ICommandBuffer::RecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo,IBuffer &bufferSrc,IImage &imgDst)
{
	uint32_t w,h;
	w = imgDst.GetWidth();
	h = imgDst.GetHeight();
	if(copyInfo.width.has_value())
		w = *copyInfo.width;
	if(copyInfo.height.has_value())
		h = *copyInfo.height;
	return DoRecordCopyBufferToImage(copyInfo,bufferSrc,imgDst,w,h);
}
bool prosper::ICommandBuffer::RecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo,IImage &imgSrc,ImageLayout srcImageLayout,IBuffer &bufferDst)
{
	uint32_t w,h;
	w = imgSrc.GetWidth();
	h = imgSrc.GetHeight();
	if(copyInfo.width.has_value())
		w = *copyInfo.width;
	if(copyInfo.height.has_value())
		h = *copyInfo.height;
	return DoRecordCopyImageToBuffer(copyInfo,imgSrc,srcImageLayout,bufferDst,w,h);
}

bool prosper::ICommandBuffer::RecordUpdateGenericShaderReadBuffer(IBuffer &buffer,uint64_t offset,uint64_t size,const void *data)
{
	if(RecordBufferBarrier(
		buffer,
		PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::GeometryShaderBit,
		PipelineStageFlags::TransferBit,
		AccessFlags::ShaderReadBit,AccessFlags::TransferWriteBit,
		offset,size
	) == false)
		return false;
	if(RecordUpdateBuffer(buffer,offset,size,data) == false)
		return false;
	return RecordBufferBarrier(
		buffer,
		PipelineStageFlags::TransferBit,
		PipelineStageFlags::FragmentShaderBit | PipelineStageFlags::VertexShaderBit | PipelineStageFlags::ComputeShaderBit | PipelineStageFlags::GeometryShaderBit,
		AccessFlags::TransferWriteBit,AccessFlags::ShaderReadBit,
		offset,size
	);
}
bool prosper::ICommandBuffer::RecordBlitImage(const util::BlitInfo &blitInfo,IImage &imgSrc,IImage &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled())
	{
		ImageLayout lastImgLayout;
		for(auto i=blitInfo.srcSubresourceLayer.baseArrayLayer;i<(blitInfo.srcSubresourceLayer.baseArrayLayer +blitInfo.srcSubresourceLayer.layerCount);++i)
		{
			if(prosper::debug::get_last_recorded_image_layout(*this,imgSrc,lastImgLayout,i,blitInfo.srcSubresourceLayer.mipLevel) && lastImgLayout != ImageLayout::TransferSrcOptimal)
			{
				debug::exec_debug_validation_callback(
					vk::DebugReportObjectTypeEXT::eImage,"Blit: Source image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(&imgSrc)) +
					" for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " +
					prosper::util::to_string(lastImgLayout) +"!"
				);
			}
		}
		for(auto i=blitInfo.dstSubresourceLayer.baseArrayLayer;i<(blitInfo.dstSubresourceLayer.baseArrayLayer +blitInfo.dstSubresourceLayer.layerCount);++i)
		{
			if(prosper::debug::get_last_recorded_image_layout(*this,imgDst,lastImgLayout,i,blitInfo.dstSubresourceLayer.mipLevel) && lastImgLayout != ImageLayout::TransferDstOptimal)
			{
				debug::exec_debug_validation_callback(
					vk::DebugReportObjectTypeEXT::eImage,"Blit: Destination image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(&imgDst)) +
					" for blit has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " +
					to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
				);
			}
		}
	}
	auto srcMipLevel = blitInfo.srcSubresourceLayer.mipLevel;
	auto srcExtents = imgSrc.GetExtents(srcMipLevel);
	std::array<Offset3D,2> srcOffsets = {
		Offset3D{blitInfo.offsetSrc.at(0),blitInfo.offsetSrc.at(1),0},
		Offset3D{
		static_cast<int32_t>(srcExtents.width),
		static_cast<int32_t>(srcExtents.height),
		1
	}
	};
	if(blitInfo.extentsSrc.has_value())
	{
		srcOffsets.at(1).x = blitInfo.extentsSrc->width;
		srcOffsets.at(1).y = blitInfo.extentsSrc->height;
	}
	auto dstMipLevel = blitInfo.dstSubresourceLayer.mipLevel;
	auto dstExtents = imgDst.GetExtents(dstMipLevel);
	std::array<Offset3D,2> dstOffsets = {
		Offset3D{blitInfo.offsetDst.at(0),blitInfo.offsetDst.at(1),0},
		Offset3D{
		static_cast<int32_t>(dstExtents.width),
		static_cast<int32_t>(dstExtents.height),
		1
	}
	};
	if(blitInfo.extentsDst.has_value())
	{
		dstOffsets.at(1).x = blitInfo.extentsDst->width;
		dstOffsets.at(1).y = blitInfo.extentsDst->height;
	}
	return DoRecordBlitImage(blitInfo,imgSrc,imgDst,srcOffsets,dstOffsets);
}
bool prosper::ICommandBuffer::RecordResolveImage(IImage &imgSrc,IImage &imgDst)
{
	if(prosper::debug::is_debug_recorded_image_layout_enabled())
	{
		ImageLayout lastImgLayout;
		if(prosper::debug::get_last_recorded_image_layout(*this,imgSrc,lastImgLayout) && lastImgLayout != ImageLayout::TransferSrcOptimal)
		{
			debug::exec_debug_validation_callback(
				vk::DebugReportObjectTypeEXT::eImage,"Resolve: Source image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(&imgSrc)) +
				" for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, but is in layout " +
				to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
			);
		}
		if(prosper::debug::get_last_recorded_image_layout(*this,imgDst,lastImgLayout) && lastImgLayout != ImageLayout::TransferDstOptimal)
		{
			debug::exec_debug_validation_callback(
				vk::DebugReportObjectTypeEXT::eImage,"Resolve: Destination image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(&imgDst)) +
				" for resolve has to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, but is in layout " +
				to_string(static_cast<vk::ImageLayout>(lastImgLayout)) +"!"
			);
		}
	}
	auto srcAspectMask = util::get_aspect_mask(imgSrc);
	auto dstAspectMask = util::get_aspect_mask(imgDst);

	auto srcExtents = imgSrc.GetExtents(0u);
	util::ImageSubresourceLayers srcLayer {srcAspectMask,0,0,1};
	util::ImageSubresourceLayers destLayer {dstAspectMask,0,0,1};
	util::ImageResolve resolve {srcLayer,Offset3D{0,0,0},destLayer,Offset3D{0,0,0},Extent3D{srcExtents.width,srcExtents.height,1}};
	return DoRecordResolveImage(imgSrc,imgDst,resolve);
}
bool prosper::ICommandBuffer::RecordBlitTexture(prosper::Texture &texSrc,IImage &imgDst)
{
	if(texSrc.IsMSAATexture() == false)
		return RecordBlitImage({},texSrc.GetImage(),imgDst);
	return RecordResolveImage(texSrc.GetImage(),imgDst);
}
bool prosper::ICommandBuffer::RecordGenerateMipmaps(IImage &img,ImageLayout currentLayout,AccessFlags srcAccessMask,PipelineStageFlags srcStage)
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

	barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers){util::create_image_barrier(img,imgBarrierInfo)};
	if(RecordPipelineBarrier(barrierInfo) == false) // Move first mipmap into transfer-src layout
		return false;

	imgBarrierInfo.subresourceRange.baseMipLevel = 1u;
	imgBarrierInfo.subresourceRange.levelCount = img.GetMipmapCount() -1;
	imgBarrierInfo.newLayout = ImageLayout::TransferDstOptimal;
	imgBarrierInfo.dstAccessMask = AccessFlags::TransferWriteBit;
	barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers){util::create_image_barrier(img,imgBarrierInfo)};
	if(RecordPipelineBarrier(barrierInfo) == false) // Move other mipmaps into transfer-dst layout
		return false;

	barrierInfo.srcStageMask = PipelineStageFlags::TransferBit;

	imgBarrierInfo.subresourceRange.levelCount = 1u;
	imgBarrierInfo.subresourceRange.layerCount = 1u;
	imgBarrierInfo.oldLayout = ImageLayout::TransferDstOptimal;
	imgBarrierInfo.newLayout = ImageLayout::TransferSrcOptimal;
	imgBarrierInfo.srcAccessMask = AccessFlags::TransferWriteBit;
	imgBarrierInfo.dstAccessMask = AccessFlags::TransferReadBit;

	for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
	{
		imgBarrierInfo.subresourceRange.baseArrayLayer = iLayer;
		for(auto i=decltype(numMipmaps){1};i<numMipmaps;++i)
		{
			blitInfo.srcSubresourceLayer.mipLevel = i -1u;
			blitInfo.dstSubresourceLayer.mipLevel = i;

			blitInfo.srcSubresourceLayer.baseArrayLayer = iLayer;
			blitInfo.dstSubresourceLayer.baseArrayLayer = iLayer;

			imgBarrierInfo.subresourceRange.baseMipLevel = i;
			barrierInfo.imageBarriers = decltype(barrierInfo.imageBarriers){util::create_image_barrier(img,imgBarrierInfo)};
			if(RecordBlitImage(blitInfo,img,img) == false || RecordPipelineBarrier(barrierInfo) == false)
				return false;
		}
	}
	return RecordImageBarrier(img,ImageLayout::TransferSrcOptimal,ImageLayout::ShaderReadOnlyOptimal);
}

///////////////

prosper::VlkCommandBuffer::VlkCommandBuffer(IPrContext &context,const std::shared_ptr<Anvil::CommandBufferBase> &cmdBuffer,prosper::QueueFamilyType queueFamilyType)
	: ICommandBuffer{context,queueFamilyType},m_cmdBuffer{cmdBuffer}
{
	prosper::debug::register_debug_object(m_cmdBuffer->get_command_buffer(),this,prosper::debug::ObjectType::CommandBuffer);
}
prosper::VlkCommandBuffer::~VlkCommandBuffer()
{
	prosper::debug::deregister_debug_object(m_cmdBuffer->get_command_buffer());
}
bool prosper::VlkCommandBuffer::Reset(bool shouldReleaseResources) const
{
	return m_cmdBuffer->reset(shouldReleaseResources);
}
bool prosper::VlkCommandBuffer::StopRecording() const
{
	return m_cmdBuffer->stop_recording();
}
bool prosper::VlkCommandBuffer::RecordSetDepthBias(float depthBiasConstantFactor,float depthBiasClamp,float depthBiasSlopeFactor)
{
	return m_cmdBuffer->record_set_depth_bias(depthBiasConstantFactor,depthBiasClamp,depthBiasSlopeFactor);
}
bool prosper::VlkCommandBuffer::RecordClearAttachment(IImage &img,const std::array<float,4> &clearColor,uint32_t attId,uint32_t layerId,uint32_t layerCount)
{
	// TODO
	//if(bufferDst.GetContext().IsValidationEnabled() && cmdBuffer.get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && get_current_render_pass_target(static_cast<Anvil::PrimaryCommandBuffer&>(cmdBuffer)) == nullptr)
	//	throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	vk::ClearValue clearVal {vk::ClearColorValue{clearColor}};
	Anvil::ClearAttachment clearAtt {Anvil::ImageAspectFlagBits::COLOR_BIT,attId,clearVal};
	vk::ClearRect clearRect {
		vk::Rect2D{vk::Offset2D{0,0},static_cast<prosper::VlkImage&>(img)->get_image_extent_2D(0u)},
		layerId,layerCount
	};
	return m_cmdBuffer->record_clear_attachments(1u,&clearAtt,1u,reinterpret_cast<VkClearRect*>(&clearRect));
}
bool prosper::VlkCommandBuffer::RecordClearAttachment(IImage &img,float clearDepth,uint32_t layerId)
{
	// TODO
	//if(bufferDst.GetContext().IsValidationEnabled() && cmdBuffer.get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && get_current_render_pass_target(static_cast<Anvil::PrimaryCommandBuffer&>(cmdBuffer)) == nullptr)
	//	throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	vk::ClearValue clearVal {vk::ClearDepthStencilValue{clearDepth}};
	Anvil::ClearAttachment clearAtt {Anvil::ImageAspectFlagBits::DEPTH_BIT,0u /* color attachment */,clearVal};
	vk::ClearRect clearRect {
		vk::Rect2D{vk::Offset2D{0,0},static_cast<prosper::VlkImage&>(img)->get_image_extent_2D(0u)},
		layerId,1 /* layerCount */
	};
	return m_cmdBuffer->record_clear_attachments(1u,&clearAtt,1u,reinterpret_cast<VkClearRect*>(&clearRect));
}
bool prosper::VlkCommandBuffer::RecordSetViewport(uint32_t width,uint32_t height,uint32_t x,uint32_t y,float minDepth,float maxDepth)
{
	auto vp = vk::Viewport(x,y,width,height,minDepth,maxDepth);
	return m_cmdBuffer->record_set_viewport(0u,1u,reinterpret_cast<VkViewport*>(&vp));
}
bool prosper::VlkCommandBuffer::RecordSetScissor(uint32_t width,uint32_t height,uint32_t x,uint32_t y)
{
	auto scissor = vk::Rect2D(vk::Offset2D(x,y),vk::Extent2D(width,height));
	return m_cmdBuffer->record_set_scissor(0u,1u,reinterpret_cast<VkRect2D*>(&scissor));
}
bool prosper::VlkCommandBuffer::DoRecordCopyBuffer(const util::BufferCopy &copyInfo,IBuffer &bufferSrc,IBuffer &bufferDst)
{
	if(bufferDst.GetContext().IsValidationEnabled() && m_cmdBuffer->get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && util::get_current_render_pass_target(static_cast<VlkPrimaryCommandBuffer&>(*this)))
		throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	static_assert(sizeof(util::BufferCopy) == sizeof(Anvil::BufferCopy));
	return m_cmdBuffer->record_copy_buffer(&dynamic_cast<VlkBuffer&>(bufferSrc).GetBaseAnvilBuffer(),&dynamic_cast<VlkBuffer&>(bufferDst).GetBaseAnvilBuffer(),1u,reinterpret_cast<const Anvil::BufferCopy*>(&copyInfo));
}
bool prosper::VlkCommandBuffer::DoRecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo,IBuffer &bufferSrc,IImage &imgDst,uint32_t w,uint32_t h)
{
	if(bufferSrc.GetContext().IsValidationEnabled() && m_cmdBuffer->get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && util::get_current_render_pass_target(static_cast<VlkPrimaryCommandBuffer&>(*this)))
		throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	Anvil::BufferImageCopy bufferImageCopy {};
	bufferImageCopy.buffer_offset = bufferSrc.GetStartOffset() +copyInfo.bufferOffset;
	bufferImageCopy.image_extent = vk::Extent3D(w,h,1);
	bufferImageCopy.image_subresource = Anvil::ImageSubresourceLayers{
		static_cast<Anvil::ImageAspectFlagBits>(copyInfo.aspectMask),copyInfo.mipLevel,copyInfo.baseArrayLayer,copyInfo.layerCount
	};
	return m_cmdBuffer->record_copy_buffer_to_image(
		&dynamic_cast<VlkBuffer&>(bufferSrc).GetBaseAnvilBuffer(),&*static_cast<VlkImage&>(imgDst),
		static_cast<Anvil::ImageLayout>(copyInfo.dstImageLayout),1u,&bufferImageCopy
	);
}
bool prosper::VlkCommandBuffer::DoRecordCopyImage(const util::CopyInfo &copyInfo,IImage &imgSrc,IImage &imgDst,uint32_t w,uint32_t h)
{
	vk::Extent3D extent{w,h,1};
	static_assert(sizeof(Anvil::ImageSubresourceLayers) == sizeof(util::ImageSubresourceLayers));
	static_assert(sizeof(vk::Offset3D) == sizeof(Offset3D));
	Anvil::ImageCopy copyRegion{
		reinterpret_cast<const Anvil::ImageSubresourceLayers&>(copyInfo.srcSubresource),reinterpret_cast<const vk::Offset3D&>(copyInfo.srcOffset),
		reinterpret_cast<const Anvil::ImageSubresourceLayers&>(copyInfo.dstSubresource),reinterpret_cast<const vk::Offset3D&>(copyInfo.dstOffset),extent
	};
	return m_cmdBuffer->record_copy_image(
		&*static_cast<VlkImage&>(imgSrc),static_cast<Anvil::ImageLayout>(copyInfo.srcImageLayout),
		&*static_cast<VlkImage&>(imgDst),static_cast<Anvil::ImageLayout>(copyInfo.dstImageLayout),
		1,&copyRegion
	);
}
bool prosper::VlkCommandBuffer::DoRecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo,IImage &imgSrc,ImageLayout srcImageLayout,IBuffer &bufferDst,uint32_t w,uint32_t h)
{
	if(bufferDst.GetContext().IsValidationEnabled() && m_cmdBuffer->get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && util::get_current_render_pass_target(static_cast<VlkPrimaryCommandBuffer&>(*this)))
		throw std::logic_error("Attempted to copy image to buffer while render pass is active!");

	Anvil::BufferImageCopy bufferImageCopy {};
	bufferImageCopy.buffer_offset = bufferDst.GetStartOffset() +copyInfo.bufferOffset;
	bufferImageCopy.image_extent = vk::Extent3D(w,h,1);
	bufferImageCopy.image_subresource = Anvil::ImageSubresourceLayers{
		static_cast<Anvil::ImageAspectFlagBits>(copyInfo.aspectMask),copyInfo.mipLevel,copyInfo.baseArrayLayer,copyInfo.layerCount
	};
	return m_cmdBuffer->record_copy_image_to_buffer(
		&*static_cast<VlkImage&>(imgSrc),static_cast<Anvil::ImageLayout>(srcImageLayout),&dynamic_cast<VlkBuffer&>(bufferDst).GetAnvilBuffer(),1u,&bufferImageCopy
	);
}
bool prosper::VlkCommandBuffer::DoRecordBlitImage(const util::BlitInfo &blitInfo,IImage &imgSrc,IImage &imgDst,const std::array<Offset3D,2> &srcOffsets,const std::array<Offset3D,2> &dstOffsets)
{
	static_assert(sizeof(util::ImageSubresourceLayers) == sizeof(Anvil::ImageSubresourceLayers));
	static_assert(sizeof(Offset3D) == sizeof(vk::Offset3D));
	Anvil::ImageBlit blit {};
	blit.src_subresource = reinterpret_cast<const Anvil::ImageSubresourceLayers&>(blitInfo.srcSubresourceLayer);
	blit.src_offsets[0] = reinterpret_cast<const vk::Offset3D&>(srcOffsets.at(0));
	blit.src_offsets[1] = reinterpret_cast<const vk::Offset3D&>(srcOffsets.at(1));
	blit.dst_subresource = reinterpret_cast<const Anvil::ImageSubresourceLayers&>(blitInfo.dstSubresourceLayer);
	blit.dst_offsets[0] = reinterpret_cast<const vk::Offset3D&>(dstOffsets.at(0));
	blit.dst_offsets[1] = reinterpret_cast<const vk::Offset3D&>(dstOffsets.at(1));
	auto bDepth = util::is_depth_format(imgSrc.GetFormat());
	blit.src_subresource.aspect_mask = blit.dst_subresource.aspect_mask = (bDepth == true) ? Anvil::ImageAspectFlagBits::DEPTH_BIT : Anvil::ImageAspectFlagBits::COLOR_BIT;
	return m_cmdBuffer->record_blit_image(
		&*static_cast<VlkImage&>(imgSrc),Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
		&*static_cast<VlkImage&>(imgDst),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
		1u,&blit,
		bDepth ? Anvil::Filter::NEAREST : Anvil::Filter::LINEAR
	);
}
bool prosper::VlkCommandBuffer::DoRecordResolveImage(IImage &imgSrc,IImage &imgDst,const util::ImageResolve &resolve)
{
	static_assert(sizeof(util::ImageResolve) == sizeof(Anvil::ImageResolve));
	return m_cmdBuffer->record_resolve_image(
		&*static_cast<VlkImage&>(imgSrc),Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
		&*static_cast<VlkImage&>(imgDst),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,
		1u,&reinterpret_cast<const Anvil::ImageResolve&>(resolve)
	);
}
bool prosper::VlkCommandBuffer::RecordUpdateBuffer(IBuffer &buffer,uint64_t offset,uint64_t size,const void *data)
{
	if(buffer.GetContext().IsValidationEnabled() && m_cmdBuffer->get_command_buffer_type() == Anvil::CommandBufferType::COMMAND_BUFFER_TYPE_PRIMARY && util::get_current_render_pass_target(static_cast<VlkPrimaryCommandBuffer&>(*this)))
		throw std::logic_error("Attempted to update buffer while render pass is active!");
	return m_cmdBuffer->record_update_buffer(&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +offset,size,reinterpret_cast<const uint32_t*>(data));
}

///////////////

bool prosper::VlkPrimaryCommandBuffer::RecordNextSubPass()
{
	return (*this)->record_next_subpass(Anvil::SubpassContents::INLINE);
}
