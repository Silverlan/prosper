/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_COMMAND_BUFFER_HPP__
#define __PR_PROSPER_VK_COMMAND_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_command_buffer.hpp"

namespace prosper
{
	class DLLPROSPER VlkCommandBuffer
		: virtual public ICommandBuffer
	{
	public:
		virtual ~VlkCommandBuffer() override;
		Anvil::CommandBufferBase &GetAnvilCommandBuffer() const;
		Anvil::CommandBufferBase &operator*();
		const Anvil::CommandBufferBase &operator*() const;
		Anvil::CommandBufferBase *operator->();
		const Anvil::CommandBufferBase *operator->() const;

		virtual bool Reset(bool shouldReleaseResources) const override;
		virtual bool StopRecording() const override;

		virtual bool RecordSetDepthBias(float depthBiasConstantFactor=0.f,float depthBiasClamp=0.f,float depthBiasSlopeFactor=0.f) override;
		virtual bool RecordClearImage(IImage &img,ImageLayout layout,const std::array<float,4> &clearColor,const util::ClearImageInfo &clearImageInfo={}) override;
		virtual bool RecordClearImage(IImage &img,ImageLayout layout,float clearDepth,const util::ClearImageInfo &clearImageInfo={}) override;
		using ICommandBuffer::RecordClearAttachment;
		virtual bool RecordClearAttachment(IImage &img,const std::array<float,4> &clearColor,uint32_t attId,uint32_t layerId,uint32_t layerCount) override;
		virtual bool RecordClearAttachment(IImage &img,float clearDepth,uint32_t layerId=0u) override;
		virtual bool RecordSetViewport(uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u,float minDepth=0.f,float maxDepth=0.f) override;
		virtual bool RecordSetScissor(uint32_t width,uint32_t height,uint32_t x=0u,uint32_t y=0u) override;
		virtual bool RecordUpdateBuffer(IBuffer &buffer,uint64_t offset,uint64_t size,const void *data) override;
	protected:
		VlkCommandBuffer(Context &context,const std::shared_ptr<Anvil::CommandBufferBase> &cmdBuffer,prosper::QueueFamilyType queueFamilyType);
		virtual bool DoRecordCopyBuffer(const util::BufferCopy &copyInfo,IBuffer &bufferSrc,IBuffer &bufferDst) override;
		virtual bool DoRecordCopyImage(const util::CopyInfo &copyInfo,IImage &imgSrc,IImage &imgDst,uint32_t w,uint32_t h) override;
		virtual bool DoRecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo,IBuffer &bufferSrc,IImage &imgDst,uint32_t w,uint32_t h) override;
		virtual bool DoRecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo,IImage &imgSrc,ImageLayout srcImageLayout,IBuffer &bufferDst,uint32_t w,uint32_t h) override;
		virtual bool DoRecordBlitImage(const util::BlitInfo &blitInfo,IImage &imgSrc,IImage &imgDst,const std::array<Offset3D,2> &srcOffsets,const std::array<Offset3D,2> &dstOffsets) override;
		virtual bool DoRecordResolveImage(IImage &imgSrc,IImage &imgDst,const util::ImageResolve &resolve) override;

		std::shared_ptr<Anvil::CommandBufferBase> m_cmdBuffer = nullptr;
	};

	///////////////////

	class DLLPROSPER VlkPrimaryCommandBuffer
		: public VlkCommandBuffer,
		public IPrimaryCommandBuffer
	{
	public:
		static std::shared_ptr<VlkPrimaryCommandBuffer> Create(Context &context,std::unique_ptr<Anvil::PrimaryCommandBuffer,std::function<void(Anvil::PrimaryCommandBuffer*)>> cmdBuffer,prosper::QueueFamilyType queueFamilyType,const std::function<void(VlkCommandBuffer&)> &onDestroyedCallback=nullptr);
		virtual bool IsPrimary() const override;

		Anvil::PrimaryCommandBuffer &GetAnvilCommandBuffer() const;
		Anvil::PrimaryCommandBuffer &operator*();
		const Anvil::PrimaryCommandBuffer &operator*() const;
		Anvil::PrimaryCommandBuffer *operator->();
		const Anvil::PrimaryCommandBuffer *operator->() const;

		virtual bool StartRecording(bool oneTimeSubmit=true,bool simultaneousUseAllowed=false) const override;
		virtual bool RecordEndRenderPass() override;
		virtual bool RecordNextSubPass() override;
	protected:
		VlkPrimaryCommandBuffer(Context &context,std::unique_ptr<Anvil::PrimaryCommandBuffer,std::function<void(Anvil::PrimaryCommandBuffer*)>> cmdBuffer,prosper::QueueFamilyType queueFamilyType);
	};

	///////////////////

	class DLLPROSPER VlkSecondaryCommandBuffer
		: public VlkCommandBuffer,
		public ISecondaryCommandBuffer
	{
	public:
		static std::shared_ptr<VlkSecondaryCommandBuffer> Create(Context &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,prosper::QueueFamilyType queueFamilyType,const std::function<void(VlkCommandBuffer&)> &onDestroyedCallback=nullptr);
		virtual bool IsSecondary() const override;

		Anvil::SecondaryCommandBuffer &GetAnvilCommandBuffer() const;
		Anvil::SecondaryCommandBuffer &operator*();
		const Anvil::SecondaryCommandBuffer &operator*() const;
		Anvil::SecondaryCommandBuffer *operator->();
		const Anvil::SecondaryCommandBuffer *operator->() const;

		bool StartRecording(
			bool oneTimeSubmit,bool simultaneousUseAllowed,bool renderPassUsageOnly,
			const Framebuffer &framebuffer,const RenderPass &rp,Anvil::SubPassID subPassId,
			Anvil::OcclusionQuerySupportScope occlusionQuerySupportScope,bool occlusionQueryUsedByPrimaryCommandBuffer,
			Anvil::QueryPipelineStatisticFlags statisticsFlags
		) const;
	protected:
		VlkSecondaryCommandBuffer(Context &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,prosper::QueueFamilyType queueFamilyType);
	};
};

#endif
