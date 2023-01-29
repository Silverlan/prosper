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
namespace prosper {
	class IBuffer;
	class IDescriptorSet;
	class Texture;
	class RenderTarget;
	class IFramebuffer;
	class IRenderPass;
	class ShaderGraphics;
	class OcclusionQuery;
	class TimestampQuery;
	class PipelineStatisticsQuery;
	class IPrimaryCommandBuffer;
	class ISecondaryCommandBuffer;
	class IShaderPipelineLayout;
	class Query;
	class Shader;
	class IRenderBuffer;
	class Window;
	class DLLPROSPER ICommandBuffer : public ContextObject, public std::enable_shared_from_this<ICommandBuffer> {
	  public:
		ICommandBuffer(const ICommandBuffer &) = delete;
		ICommandBuffer &operator=(const ICommandBuffer &) = delete;
		virtual ~ICommandBuffer() override;

		virtual bool IsPrimary() const;
		virtual bool IsSecondary() const;
		virtual bool Reset(bool shouldReleaseResources) const = 0;
		virtual bool StopRecording() const = 0;
		bool IsRecording() const { return m_recording; }

		virtual bool RecordBindIndexBuffer(IBuffer &buf, IndexType indexType = IndexType::UInt16, DeviceSize offset = 0) = 0;
		virtual bool RecordBindVertexBuffers(const prosper::ShaderGraphics &shader, const std::vector<IBuffer *> &buffers, uint32_t startBinding = 0u, const std::vector<DeviceSize> &offsets = {}) = 0;
		virtual bool RecordBindVertexBuffer(const prosper::ShaderGraphics &shader, const IBuffer &buf, uint32_t startBinding = 0u, DeviceSize offset = 0u) = 0;
		virtual bool RecordBindRenderBuffer(const IRenderBuffer &renderBuffer) = 0;
		virtual bool RecordDispatchIndirect(prosper::IBuffer &buffer, DeviceSize size) = 0;
		virtual bool RecordDispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
		virtual bool RecordDraw(uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
		virtual bool RecordDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, uint32_t firstInstance = 0) = 0;
		virtual bool RecordDrawIndexedIndirect(IBuffer &buf, DeviceSize offset, uint32_t drawCount, uint32_t stride) = 0;
		virtual bool RecordDrawIndirect(IBuffer &buf, DeviceSize offset, uint32_t count, uint32_t stride) = 0;
		virtual bool RecordFillBuffer(IBuffer &buf, DeviceSize offset, DeviceSize size, uint32_t data) = 0;
		// bool RecordResetEvent(Event &ev,PipelineStateFlags stageMask);
		virtual bool RecordSetBlendConstants(const std::array<float, 4> &blendConstants) = 0;
		virtual bool RecordSetDepthBounds(float minDepthBounds, float maxDepthBounds) = 0;
		// bool RecordSetEvent(Event &ev,PipelineStageFlags stageMask);
		virtual bool RecordSetStencilCompareMask(StencilFaceFlags faceMask, uint32_t stencilCompareMask) = 0;
		virtual bool RecordSetStencilReference(StencilFaceFlags faceMask, uint32_t stencilReference) = 0;
		virtual bool RecordSetStencilWriteMask(StencilFaceFlags faceMask, uint32_t stencilWriteMask) = 0;

		prosper::QueueFamilyType GetQueueFamilyType() const;
		bool RecordCopyBuffer(const util::BufferCopy &copyInfo, IBuffer &bufferSrc, IBuffer &bufferDst);
		virtual bool RecordSetDepthBias(float depthBiasConstantFactor = 0.f, float depthBiasClamp = 0.f, float depthBiasSlopeFactor = 0.f) = 0;
		virtual bool RecordClearImage(IImage &img, ImageLayout layout, const std::array<float, 4> &clearColor, const util::ClearImageInfo &clearImageInfo = {}) = 0;
		virtual bool RecordClearImage(IImage &img, ImageLayout layout, std::optional<float> clearDepth, std::optional<uint32_t> clearStencil, const util::ClearImageInfo &clearImageInfo = {}) = 0;
		virtual bool RecordClearAttachment(IImage &img, const std::array<float, 4> &clearColor, uint32_t attId, uint32_t layerId, uint32_t layerCount = 1) = 0;
		virtual bool RecordClearAttachment(IImage &img, std::optional<float> clearDepth, std::optional<uint32_t> clearStencil, uint32_t layerId = 0u) = 0;
		bool RecordClearAttachment(IImage &img, const std::array<float, 4> &clearColor, uint32_t attId = 0u);
		bool RecordCopyImage(const util::CopyInfo &copyInfo, IImage &imgSrc, IImage &imgDst);
		bool RecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo, IBuffer &bufferSrc, IImage &imgDst);
		bool RecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo, IImage &imgSrc, ImageLayout srcImageLayout, IBuffer &bufferDst);
		virtual bool RecordUpdateBuffer(IBuffer &buffer, uint64_t offset, uint64_t size, const void *data) = 0;
		template<typename T>
		bool RecordUpdateBuffer(IBuffer &buffer, uint64_t offset, const T &data);

		// Records the buffer update, as well as a pre- and post-update barrier to ensure shaders cannot read from the buffer while it is being updated.
		bool RecordUpdateGenericShaderReadBuffer(IBuffer &buffer, uint64_t offset, uint64_t size, const void *data);
		bool RecordBlitImage(const util::BlitInfo &blitInfo, IImage &imgSrc, IImage &imgDst);
		bool RecordResolveImage(IImage &imgSrc, IImage &imgDst);
		// The source texture image will be copied to the destination image using a resolve (if it's a MSAA texture) or a blit
		bool RecordBlitTexture(prosper::Texture &texSrc, IImage &imgDst);
		bool RecordGenerateMipmaps(IImage &img, ImageLayout currentLayout, AccessFlags srcAccessMask, PipelineStageFlags srcStage);
		virtual bool RecordPipelineBarrier(const util::PipelineBarrierInfo &barrierInfo) = 0;
		// Records an image barrier. If no layer is specified, ALL layers of the image will be included in the barrier.
		bool RecordImageBarrier(IImage &img, PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, ImageLayout oldLayout, ImageLayout newLayout, AccessFlags srcAccessMask, AccessFlags dstAccessMask, uint32_t baseLayer = std::numeric_limits<uint32_t>::max(),
		  std::optional<prosper::ImageAspectFlags> aspectMask = {});
		bool RecordImageBarrier(IImage &img, const util::BarrierImageLayout &srcBarrierInfo, const util::BarrierImageLayout &dstBarrierInfo, const util::ImageSubresourceRange &subresourceRange = {}, std::optional<prosper::ImageAspectFlags> aspectMask = {});
		bool RecordImageBarrier(IImage &img, ImageLayout srcLayout, ImageLayout dstLayout, const util::ImageSubresourceRange &subresourceRange = {}, std::optional<prosper::ImageAspectFlags> aspectMask = {});
		bool RecordPostRenderPassImageBarrier(IImage &img, ImageLayout preRenderPassLayout, ImageLayout postRenderPassLayout, const util::ImageSubresourceRange &subresourceRange = {}, std::optional<prosper::ImageAspectFlags> aspectMask = {});
		bool RecordBufferBarrier(IBuffer &buf, PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, AccessFlags srcAccessMask, AccessFlags dstAccessMask, DeviceSize offset = 0ull, DeviceSize size = std::numeric_limits<DeviceSize>::max());
		virtual bool RecordBindDescriptorSets(PipelineBindPoint bindPoint, prosper::Shader &shader, PipelineID pipelineId, uint32_t firstSet, const std::vector<prosper::IDescriptorSet *> &descSets, const std::vector<uint32_t> dynamicOffsets = {}) = 0;
		virtual bool RecordBindDescriptorSets(PipelineBindPoint bindPoint, const IShaderPipelineLayout &pipelineLayout, uint32_t firstSet, const prosper::IDescriptorSet &descSet, uint32_t *optDynamicOffset = nullptr) = 0;
		virtual bool RecordBindDescriptorSets(PipelineBindPoint bindPoint, const IShaderPipelineLayout &pipelineLayout, uint32_t firstSet, uint32_t numDescSets, const prosper::IDescriptorSet *const *descSets, uint32_t numDynamicOffsets = 0, const uint32_t *dynamicOffsets = nullptr) = 0;
		template<class TDs, class TDo>
		bool RecordBindDescriptorSets(PipelineBindPoint bindPoint, const IShaderPipelineLayout &pipelineLayout, uint32_t firstSet, const TDs &descSets, const TDo &offsets)
		{
			return RecordBindDescriptorSets(bindPoint, pipelineLayout, firstSet, descSets.size(), descSets.data(), offsets.size(), offsets.data()); //,,);
		}
		virtual bool RecordPushConstants(prosper::Shader &shader, PipelineID pipelineId, ShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data) = 0;
		virtual bool RecordPushConstants(const IShaderPipelineLayout &pipelineLayout, ShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data) = 0;
		bool RecordBindShaderPipeline(prosper::Shader &shader, PipelineID shaderPipelineId);
		bool RecordUnbindShaderPipeline();

		virtual bool RecordSetLineWidth(float lineWidth) = 0;
		virtual bool RecordSetViewport(uint32_t width, uint32_t height, uint32_t x = 0u, uint32_t y = 0u, float minDepth = 0.f, float maxDepth = 0.f) = 0;
		virtual bool RecordSetScissor(uint32_t width, uint32_t height, uint32_t x = 0u, uint32_t y = 0u) = 0;

		virtual bool RecordBeginPipelineStatisticsQuery(const PipelineStatisticsQuery &query) const = 0;
		virtual bool RecordEndPipelineStatisticsQuery(const PipelineStatisticsQuery &query) const = 0;
		virtual bool RecordBeginOcclusionQuery(const OcclusionQuery &query) const = 0;
		virtual bool RecordEndOcclusionQuery(const OcclusionQuery &query) const = 0;
		virtual bool WriteTimestampQuery(const TimestampQuery &query) const = 0;
		virtual bool ResetQuery(const Query &query) const = 0;

		virtual bool RecordPresentImage(IImage &img, IImage &swapchainImg, IFramebuffer &swapchainFramebuffer) = 0;
		bool RecordPresentImage(IImage &img, uint32_t swapchainImgIndex);
		bool RecordPresentImage(IImage &img, Window &window);
		bool RecordPresentImage(IImage &img, Window &window, uint32_t swapchainImgIndex);
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ICommandBuffer, T>>>
		T &GetAPITypeRef()
		{
			return *static_cast<T *>(m_apiTypePtr);
		}
		template<class T, typename = std::enable_if_t<std::is_base_of_v<ICommandBuffer, T>>>
		const T &GetAPITypeRef() const
		{
			return const_cast<ICommandBuffer *>(this)->GetAPITypeRef<T>();
		}

		virtual const void *GetInternalHandle() const { return nullptr; }
		IPrimaryCommandBuffer *GetPrimaryCommandBufferPtr();
		const IPrimaryCommandBuffer *GetPrimaryCommandBufferPtr() const;
		ISecondaryCommandBuffer *GetSecondaryCommandBufferPtr();
		const ISecondaryCommandBuffer *GetSecondaryCommandBufferPtr() const;
	  protected:
		ICommandBuffer(IPrContext &context, prosper::QueueFamilyType queueFamilyType);
		void Initialize();
		friend Shader;
		virtual void ClearBoundPipeline();
		virtual bool DoRecordBindShaderPipeline(prosper::Shader &shader, PipelineID shaderPipelineId, PipelineID pipelineId) = 0;
		virtual bool DoRecordCopyBuffer(const util::BufferCopy &copyInfo, IBuffer &bufferSrc, IBuffer &bufferDst) = 0;
		virtual bool DoRecordCopyImage(const util::CopyInfo &copyInfo, IImage &imgSrc, IImage &imgDst, uint32_t w, uint32_t h) = 0;
		virtual bool DoRecordCopyBufferToImage(const util::BufferImageCopyInfo &copyInfo, IBuffer &bufferSrc, IImage &imgDst) = 0;
		virtual bool DoRecordCopyImageToBuffer(const util::BufferImageCopyInfo &copyInfo, IImage &imgSrc, ImageLayout srcImageLayout, IBuffer &bufferDst) = 0;
		virtual bool DoRecordBlitImage(const util::BlitInfo &blitInfo, IImage &imgSrc, IImage &imgDst, const std::array<Offset3D, 2> &srcOffsets, const std::array<Offset3D, 2> &dstOffsets, std::optional<prosper::ImageAspectFlags> aspectFlags = {}) = 0;
		virtual bool DoRecordResolveImage(IImage &imgSrc, IImage &imgDst, const util::ImageResolve &resolve) = 0;
		void UpdateLastUsageTimes(IDescriptorSet &ds);

		void SetRecording(bool recording) const { m_recording = recording; }

		prosper::QueueFamilyType m_queueFamilyType = prosper::QueueFamilyType::Compute;
		void *m_apiTypePtr = nullptr;
		void *m_cmdBufSpecializationPtr = nullptr; // Pointer to IPrimaryCommandBuffer or ISecondaryCommandBuffer
		mutable bool m_recording = false;
	};

	class DLLPROSPER ICommandBufferPool : public ContextObject, public std::enable_shared_from_this<ICommandBufferPool> {
	  public:
		virtual std::shared_ptr<IPrimaryCommandBuffer> AllocatePrimaryCommandBuffer() const = 0;
		virtual std::shared_ptr<ISecondaryCommandBuffer> AllocateSecondaryCommandBuffer() const = 0;
		prosper::QueueFamilyType GetQueueFamilyType() const { return m_queueFamilyType; }
	  protected:
		ICommandBufferPool(prosper::IPrContext &context, prosper::QueueFamilyType queueFamilyType) : ContextObject {context}, m_queueFamilyType {queueFamilyType} {}
		prosper::QueueFamilyType m_queueFamilyType;
	};

	///////////////////

	class DLLPROSPER IPrimaryCommandBuffer : virtual public ICommandBuffer {
	  public:
		struct RenderTargetInfo {
			std::weak_ptr<prosper::IRenderPass> renderPass;
			uint32_t baseLayer;
			std::weak_ptr<prosper::IImage> image;
			std::weak_ptr<prosper::IFramebuffer> framebuffer;
			std::weak_ptr<prosper::RenderTarget> renderTarget;
		};
		enum class RenderPassFlags : uint8_t { None = 0u, SecondaryCommandBuffers = 1u };

		// If no render pass is specified, the render target's render pass will be used
		bool RecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t layerId, RenderPassFlags renderPassFlags = RenderPassFlags::None, const ClearValue *clearValue = nullptr, prosper::IRenderPass *rp = nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t layerId, const std::vector<ClearValue> &clearValues, RenderPassFlags renderPassFlags = RenderPassFlags::None, prosper::IRenderPass *rp = nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt, RenderPassFlags renderPassFlags = RenderPassFlags::None, const ClearValue *clearValue = nullptr, prosper::IRenderPass *rp = nullptr);
		bool RecordBeginRenderPass(prosper::RenderTarget &rt, const std::vector<ClearValue> &clearValues, RenderPassFlags renderPassFlags = RenderPassFlags::None, prosper::IRenderPass *rp = nullptr);
		bool RecordBeginRenderPass(prosper::IImage &img, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, RenderPassFlags renderPassFlags = RenderPassFlags::None, const std::vector<ClearValue> &clearValues = {});
		virtual bool StartRecording(bool oneTimeSubmit = true, bool simultaneousUseAllowed = false) const;
		virtual bool StopRecording() const override;
		bool RecordEndRenderPass();
		virtual bool RecordNextSubPass() = 0;
		virtual bool ExecuteCommands(prosper::ISecondaryCommandBuffer &cmdBuf) = 0;

		RenderTargetInfo *GetActiveRenderPassTargetInfo() const;
		bool GetActiveRenderPassTarget(prosper::IRenderPass **outRp = nullptr, prosper::IImage **outImg = nullptr, prosper::IFramebuffer **outFb = nullptr, prosper::RenderTarget **outRt = nullptr) const;
		void SetActiveRenderPassTarget(prosper::IRenderPass *outRp, uint32_t layerId, prosper::IImage *outImg = nullptr, prosper::IFramebuffer *outFb = nullptr, prosper::RenderTarget *outRt = nullptr) const;
	  protected:
		bool DoRecordBeginRenderPass(prosper::RenderTarget &rt, uint32_t *layerId, const std::vector<prosper::ClearValue> &clearValues, prosper::IRenderPass *rp, RenderPassFlags renderPassFlags);
		virtual bool DoRecordEndRenderPass() = 0;
		virtual bool DoRecordBeginRenderPass(prosper::IImage &img, prosper::IRenderPass &rp, prosper::IFramebuffer &fb, uint32_t *layerId, const std::vector<prosper::ClearValue> &clearValues, RenderPassFlags renderPassFlags) = 0;

		mutable std::optional<RenderTargetInfo> m_renderTargetInfo {};
	};

	///////////////////

	class DLLPROSPER ISecondaryCommandBuffer : virtual public ICommandBuffer {
	  public:
		using ICommandBuffer::ICommandBuffer;
		virtual bool StartRecording(bool oneTimeSubmit = true, bool simultaneousUseAllowed = false) const;
		virtual bool StartRecording(prosper::IRenderPass &rp, prosper::IFramebuffer &fb, bool oneTimeSubmit = true, bool simultaneousUseAllowed = false) const;
		virtual bool StopRecording() const override;

		prosper::IRenderPass *GetCurrentRenderPass() { return m_currentRenderPass; }
		const prosper::IRenderPass *GetCurrentRenderPass() const { return const_cast<ISecondaryCommandBuffer *>(this)->GetCurrentRenderPass(); }

		prosper::IFramebuffer *GetCurrentFramebuffer() { return m_currentFramebuffer; }
		const prosper::IFramebuffer *GetCurrentFramebuffer() const { return const_cast<ISecondaryCommandBuffer *>(this)->GetCurrentFramebuffer(); }
	  protected:
		mutable prosper::IRenderPass *m_currentRenderPass = nullptr;
		mutable prosper::IFramebuffer *m_currentFramebuffer = nullptr;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::IPrimaryCommandBuffer::RenderPassFlags)
template<typename T>
bool prosper::ICommandBuffer::RecordUpdateBuffer(IBuffer &buffer, uint64_t offset, const T &data)
{
	return RecordUpdateBuffer(buffer, offset, sizeof(data), &data);
}
#pragma warning(pop)

#endif
