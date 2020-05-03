/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_COMMAND_BUFFER_HPP__
#define __PROSPER_COMMAND_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"
#include <optional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class IBuffer;
	class IDescriptorSet;
	class Texture;
	class RenderTarget;
	class IFramebuffer;
	class IRenderPass;
	class DLLPROSPER ICommandBuffer
		: public ContextObject,
		public std::enable_shared_from_this<ICommandBuffer>
	{
	public:
		ICommandBuffer(const ICommandBuffer&)=delete;
		ICommandBuffer &operator=(const ICommandBuffer&)=delete;
		virtual ~ICommandBuffer() override;

		virtual bool IsPrimary() const;
		virtual bool IsSecondary() const;
		virtual bool Reset(bool shouldReleaseResources) const=0;
		virtual bool StopRecording() const=0;

		bool RecordBindIndexBuffer(IBuffer &buf,IndexType indexType=IndexType::UInt16,DeviceSize offset=0);
		bool RecordBindVertexBuffers(const std::vector<IBuffer*> &buffers,uint32_t startBinding=0u,const std::vector<DeviceSize> &offsets={});
		bool RecordDispatchIndirect(prosper::IBuffer &buffer,DeviceSize size);
		bool RecordDraw(uint32_t vertCount,uint32_t instanceCount=1,uint32_t firstVertex=0,uint32_t firstInstance=0);
		bool RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount=1,uint32_t firstIndex=0,int32_t vertexOffset=0,uint32_t firstInstance=0);
		bool RecordDrawIndexedIndirect(IBuffer &buf,DeviceSize offset,uint32_t drawCount,uint32_t stride);
		bool RecordDrawIndirect(IBuffer &buf,DeviceSize offset,uint32_t count,uint32_t stride);
		bool RecordFillBuffer(IBuffer &buf,DeviceSize offset,DeviceSize size,uint32_t data);
		// bool RecordResetEvent(Event &ev,PipelineStateFlags stageMask);
		bool RecordSetBlendConstants(const std::array<float,4> &blendConstants);
		bool RecordSetDepthBounds(float minDepthBounds,float maxDepthBounds);
		// bool RecordSetEvent(Event &ev,PipelineStageFlags stageMask);
		bool RecordSetStencilCompareMask(StencilFaceFlags faceMask,uint32_t stencilCompareMask);
		bool RecordSetStencilReference(StencilFaceFlags faceMask,uint32_t stencilReference);
		bool RecordSetStencilWriteMask(StencilFaceFlags faceMask,uint32_t stencilWriteMask);

		prosper::QueueFamilyType GetQueueFamilyType() const;
		bool RecordCopyBuffer(const util::BufferCopy &copyInfo,IBuffer &bufferSrc,IBuffer &bufferDst);
		virtual bool RecordSetDepthBias(float depthBiasConstantFactor=0.f,float depthBiasClamp=0.f,float depthBiasSlopeFactor=0.f)=0;
		virtual bool RecordClearImage(IImage &img,ImageLayout layout,const std::array<float,4> &clearColor,const util::ClearImageInfo &clearImageInfo={})=0;
		virtual bool RecordClearImage(IImage &img,ImageLayout layout,float clearDepth,const util::ClearImageInfo &clearImageInfo={})=0;
		virtual bool RecordClearAttachment(IImage &img,const std::array<float,4> &clearColor,uint32_t attId,uint32_t layerId,uint32_t layerCount=1)=0;
		virtual bool RecordClearAttachment(IImage &img,float clearDepth,uint32_t layerId=0u)=0;
		bool RecordClearAttachment(IImage &img,const std::array<float,4> &clearColor,uint32_t attId=0u);
		bool RecordCopyImage(const util::CopyInfo &copyInfo,IImage &imgSrc,IImage &imgDst);
		bool RecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo,IBuffer &bufferSrc,IImage &imgDst);
		bool RecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo,IImage &imgSrc,ImageLayout srcImageLayout,IBuffer &bufferDst);
		virtual bool RecordUpdateBuffer(IBuffer &buffer,uint64_t offset,uint64_t size,const void *data)=0;
		template<typename T>
			bool RecordUpdateBuffer(IBuffer &buffer,uint64_t offset,const T &data);

		// Records the buffer update, as well as a pre- and post-update barrier to ensure shaders cannot read from the buffer while it is being updated.
		bool RecordUpdateGenericShaderReadBuffer(IBuffer &buffer,uint64_t offset,uint64_t size,const void *data);
		bool RecordBlitImage(const util::BlitInfo &blitInfo,IImage &imgSrc,IImage &imgDst);
		bool RecordResolveImage(IImage &imgSrc,IImage &imgDst);
		// The source texture image will be copied to the destination image using a resolve (if it's a MSAA texture) or a blit
		bool RecordBlitTexture(prosper::Texture &texSrc,IImage &imgDst);
		bool RecordGenerateMipmaps(IImage &img,ImageLayout currentLayout,AccessFlags srcAccessMask,PipelineStageFlags srcStage);
		bool RecordPipelineBarrier(const util::PipelineBarrierInfo &barrierInfo);
		// Records an image barrier. If no layer is specified, ALL layers of the image will be included in the barrier.
		bool RecordImageBarrier(
			IImage &img,PipelineStageFlags srcStageMask,PipelineStageFlags dstStageMask,
			ImageLayout oldLayout,ImageLayout newLayout,AccessFlags srcAccessMask,AccessFlags dstAccessMask,
			uint32_t baseLayer=std::numeric_limits<uint32_t>::max()
		);
		bool RecordImageBarrier(
			IImage &img,const util::BarrierImageLayout &srcBarrierInfo,const util::BarrierImageLayout &dstBarrierInfo,
			const util::ImageSubresourceRange &subresourceRange={}
		);
		bool RecordImageBarrier(
			IImage &img,ImageLayout srcLayout,ImageLayout dstLayout,const util::ImageSubresourceRange &subresourceRange={}
		);
		bool RecordPostRenderPassImageBarrier(
			IImage &img,
			ImageLayout preRenderPassLayout,ImageLayout postRenderPassLayout,
			const util::ImageSubresourceRange &subresourceRange={}
		);
		bool RecordBufferBarrier(
			IBuffer &buf,PipelineStageFlags srcStageMask,PipelineStageFlags dstStageMask,
			AccessFlags srcAccessMask,AccessFlags dstAccessMask,vk::DeviceSize offset=0ull,vk::DeviceSize size=std::numeric_limits<vk::DeviceSize>::max()
		);
		bool RecordBindDescriptorSets(PipelineBindPoint bindPoint,Anvil::PipelineLayout &layout,uint32_t firstSet,const std::vector<prosper::IDescriptorSet*> &descSets,const std::vector<uint32_t> dynamicOffsets={});
		bool RecordPushConstants(Anvil::PipelineLayout &layout,ShaderStageFlags stageFlags,uint32_t offset,uint32_t size,const void *data);
		bool RecordBindPipeline(PipelineBindPoint in_pipeline_bind_point,PipelineID in_pipeline_id);

		bool RecordSetLineWidth(float lineWidth);
		virtual bool RecordSetViewport(uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u,float minDepth=0.f,float maxDepth=0.f)=0;
		virtual bool RecordSetScissor(uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u)=0;
	protected:
		ICommandBuffer(IPrContext &context,prosper::QueueFamilyType queueFamilyType);
		virtual bool DoRecordCopyBuffer(const util::BufferCopy &copyInfo,IBuffer &bufferSrc,IBuffer &bufferDst)=0;
		virtual bool DoRecordCopyImage(const util::CopyInfo &copyInfo,IImage &imgSrc,IImage &imgDst,uint32_t w,uint32_t h)=0;
		virtual bool DoRecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo,IBuffer &bufferSrc,IImage &imgDst,uint32_t w,uint32_t h)=0;
		virtual bool DoRecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo,IImage &imgSrc,ImageLayout srcImageLayout,IBuffer &bufferDst,uint32_t w,uint32_t h)=0;
		virtual bool DoRecordBlitImage(const util::BlitInfo &blitInfo,IImage &imgSrc,IImage &imgDst,const std::array<Offset3D,2> &srcOffsets,const std::array<Offset3D,2> &dstOffsets)=0;
		virtual bool DoRecordResolveImage(IImage &imgSrc,IImage &imgDst,const util::ImageResolve &resolve)=0;

		prosper::QueueFamilyType m_queueFamilyType = prosper::QueueFamilyType::Compute;
	};

	///////////////////

	class DLLPROSPER IPrimaryCommandBuffer
		: virtual public ICommandBuffer
	{
	public:
		// If no render pass is specified, the render target's render pass will be used
		bool RecordBeginRenderPass(prosper::RenderTarget &rt,uint32_t layerId,const vk::ClearValue *clearValue=nullptr,prosper::IRenderPass *rp=nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt,uint32_t layerId,const std::vector<vk::ClearValue> &clearValues,prosper::IRenderPass *rp=nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt,const vk::ClearValue *clearValue=nullptr,prosper::IRenderPass *rp=nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt,const std::vector<vk::ClearValue> &clearValues,prosper::IRenderPass *rp=nullptr);
		bool RecordBeginRenderPass(prosper::IImage &img,prosper::IRenderPass &rp,prosper::IFramebuffer &fb,const std::vector<vk::ClearValue> &clearValues={});
		virtual bool StartRecording(bool oneTimeSubmit=true,bool simultaneousUseAllowed=false) const=0;
		virtual bool RecordEndRenderPass()=0;
		virtual bool RecordNextSubPass()=0;
	protected:
	};

	///////////////////

	class DLLPROSPER ISecondaryCommandBuffer
		: virtual public ICommandBuffer
	{
	public:
		using ICommandBuffer::ICommandBuffer;
	protected:
	};
};
template<typename T>
		bool prosper::ICommandBuffer::RecordUpdateBuffer(IBuffer &buffer,uint64_t offset,const T &data)
{
	return RecordUpdateBuffer(buffer,offset,sizeof(data),&data);
}
#pragma warning(pop)

#endif
