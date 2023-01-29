/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_HPP__
#define __PROSPER_SHADER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_context_object.hpp"
#include "prosper_enums.hpp"
#include "prosper_structs.hpp"
#include <cinttypes>
#include <sharedutils/util_weak_handle.hpp>
#include <mathutil/umath.h>
#include <unordered_map>
#include <optional>
#include <functional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	class IDescriptorSetGroup;
	class IRenderPass;
	class ICommandBuffer;
	class IBuffer;
	class IDescriptorSet;
	class ShaderManager;
	class IPipelineManager;
	class GraphicsPipelineCreateInfo;
	class ComputePipelineCreateInfo;
	class RayTracingPipelineCreateInfo;
	using ShaderIndex = uint32_t;

	struct DLLPROSPER ShaderBindState {
		ShaderBindState(prosper::ICommandBuffer &cmdBuf);
		prosper::ICommandBuffer &commandBuffer;
		uint32_t pipelineIdx = std::numeric_limits<uint32_t>::max();
	};

	namespace util {
		struct RenderPassCreateInfo;
	};
	class DLLPROSPER Shader : public ContextObject, public std::enable_shared_from_this<Shader> {
	  public:
		static void SetLogCallback(const std::function<void(Shader &, ShaderStage, const std::string &, const std::string &)> &fLogCallback);
		static const std::function<void(Shader &, ShaderStage, const std::string &, const std::string &)> &GetLogCallback();
		static Shader *GetBoundPipeline(prosper::ICommandBuffer &cmdBuffer, uint32_t &outPipelineIdx);
		static void SetRootShaderLocation(const std::string &location);
		static const std::string &GetRootShaderLocation();
		//static int32_t Register(const std::string &identifier,const std::function<Shader*(Context&,const std::string&)> &fFactory);

		Shader(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");
		Shader(IPrContext &context, const std::string &identifier, const std::string &csShader);
		Shader(const Shader &) = delete;
		Shader &operator=(const Shader &) = delete;
		virtual ~Shader() override = default;

		void Release(bool bDelete = true);
		void SetDebugName(uint32_t pipelineIdx, const std::string &name);
		const std::string *GetDebugName(uint32_t pipelineIdx) const;

		::util::WeakHandle<const prosper::Shader> GetHandle() const;
		::util::WeakHandle<prosper::Shader> GetHandle();

		void Initialize(bool bReloadSourceCode = false);
		void FinalizeInitialization();
		void ReloadPipelines(bool bReloadSourceCode = false);
		uint32_t GetPipelineCount() const;

		bool IsGraphicsShader() const;
		bool IsComputeShader() const;
		bool IsRaytracingShader() const;
		PipelineBindPoint GetPipelineBindPoint() const;
		const prosper::ShaderModuleStageEntryPoint *GetModuleStageEntryPoint(prosper::ShaderStage stage, uint32_t pipelineIdx = 0u) const;
		bool GetPipelineId(prosper::PipelineID &pipelineId, uint32_t pipelineIdx = 0u, bool waitForLoad = true) const;

		void BakePipeline(uint32_t pipelineIdx) const;
		void BakePipelines() const;

		// If a base shader is specified, pipelines will be created as derived pipelines
		// of the pipeline of that shader
		void SetBaseShader(Shader &shader);
		template<class TShader>
		void SetBaseShader();
		void ClearBaseShader();

		// Used for identifying groups of shader types (e.g. GUI shaders, 3D shaders, etc.), returns the hash code of the typeid for this base class
		virtual size_t GetBaseTypeHashCode() const;

		bool RecordBindPipeline(ShaderBindState &bindState, uint32_t pipelineIdx = 0u) const;
		template<class T>
		bool RecordPushConstants(ShaderBindState &bindState, const T &data, uint32_t offset = 0u) const;
		bool RecordPushConstants(ShaderBindState &bindState, uint32_t size, const void *data, uint32_t offset = 0u) const;
		bool RecordBindDescriptorSets(ShaderBindState &bindState, const std::vector<prosper::IDescriptorSet *> &descSets, uint32_t firstSet = 0u, const std::vector<uint32_t> &dynamicOffsets = {}) const;
		void AddDescriptorSetGroup(prosper::BasePipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx, DescriptorSetInfo &descSetInfo);
		bool AttachPushConstantRange(prosper::BasePipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx, uint32_t offset, uint32_t size, prosper::ShaderStageFlags stages);
		virtual bool RecordBindDescriptorSet(ShaderBindState &bindState, prosper::IDescriptorSet &descSet, uint32_t firstSet = 0u, const std::vector<uint32_t> &dynamicOffsets = {}) const;

		prosper::ShaderModule *GetModule(ShaderStage stage);
		bool IsValid() const;
		const std::string &GetIdentifier() const;
		ShaderIndex GetIndex() const { return m_shaderIndex; }

		const PipelineInfo *GetPipelineInfo(PipelineID id) const;
		PipelineInfo *GetPipelineInfo(PipelineID id);
		const BasePipelineCreateInfo *GetPipelineCreateInfo(PipelineID id) const;
		BasePipelineCreateInfo *GetPipelineCreateInfo(PipelineID id);
		bool GetSourceFilePath(ShaderStage stage, std::string &sourceFilePath) const;
		std::vector<std::string> GetSourceFilePaths() const;

		std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(uint32_t setIdx, uint32_t pipelineIdx = 0u) const;

		std::array<std::shared_ptr<ShaderStageData>, umath::to_integral(prosper::ShaderStage::Count)> &GetStages();

		void SetMultiThreadedPipelineInitializationEnabled(bool enabled) { m_enableMultiThreadedPipelineInitialization = enabled; }

		// Has to be called before the pipeline is initialized!
		void SetStageSourceFilePath(ShaderStage stage, const std::string &filePath);
		std::optional<std::string> GetStageSourceFilePath(ShaderStage stage) const;
	  protected:
		friend ShaderManager;
		void SetIdentifier(const std::string &identifier);
		void SetPipelineCount(uint32_t count);
		virtual void InitializePipeline();
		virtual void OnPipelineInitialized(uint32_t pipelineIdx);
		// Called when the pipelines have been initialized for the first time
		virtual void OnInitialized();
		virtual void OnPipelinesInitialized();
		virtual bool ShouldInitializePipeline(uint32_t pipelineIdx);
		void ClearPipelines();
		prosper::PipelineID InitPipelineId(uint32_t pipelineIdx);
		void FlushLoad() const;
		std::vector<PipelineInfo> &GetPipelineInfos() { return m_pipelineInfos; }

		ShaderStageData *GetStage(ShaderStage stage);
		const ShaderStageData *GetStage(ShaderStage stage) const;
		void InitializeDescriptorSetGroup(prosper::BasePipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx);
		// Pipeline this pipeline is derived from
		std::weak_ptr<Shader> m_basePipeline = {};
		mutable bool m_loading = false;

		void SetIndex(ShaderIndex shaderIndex) { m_shaderIndex = shaderIndex; }
	  private:
		static std::function<void(Shader &, ShaderStage, const std::string &, const std::string &)> s_logCallback;
		using std::enable_shared_from_this<Shader>::shared_from_this;

		bool InitializeSources(bool bReload = false);
		void InitializeStages();

		std::array<std::shared_ptr<ShaderStageData>, umath::to_integral(prosper::ShaderStage::Count)> m_stages;
		bool m_bValid = false;
		bool m_bFirstTimeInit = true;
		bool m_enableMultiThreadedPipelineInitialization = true;
		ShaderIndex m_shaderIndex = std::numeric_limits<ShaderIndex>::max();
		std::string m_identifier;
		std::vector<PipelineInfo> m_pipelineInfos {};

		PipelineBindPoint m_pipelineBindPoint = static_cast<PipelineBindPoint>(-1);
		std::vector<prosper::PipelineID> m_cachedPipelineIds;
	};

	class IRenderBuffer;
	class DLLPROSPER ShaderGraphics : public Shader {
	  protected:
		virtual void PrepareGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
	  public:
		struct DLLPROSPER VertexBinding {
			VertexBinding() = default;
			VertexBinding(prosper::VertexInputRate inputRate, uint32_t stride = std::numeric_limits<uint32_t>::max());
			VertexBinding(const VertexBinding &vbOther, std::optional<prosper::VertexInputRate> inputRate = {}, std::optional<uint32_t> stride = {});
			uint32_t stride = std::numeric_limits<uint32_t>::max();
			prosper::VertexInputRate inputRate = prosper::VertexInputRate::Vertex;

			uint32_t GetBindingIndex() const;
		  private:
			friend void ShaderGraphics::PrepareGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
			std::function<void()> initializer = nullptr;

			uint32_t bindingIndex = std::numeric_limits<uint32_t>::max();
		};
		struct DLLPROSPER VertexAttribute {
			VertexAttribute() = default;
			VertexAttribute(const VertexBinding &binding, prosper::Format format, size_t startOffset = std::numeric_limits<size_t>::max());
			VertexAttribute(const VertexAttribute &vaOther);
			VertexAttribute(const VertexAttribute &vaOther, const VertexBinding &binding, std::optional<prosper::Format> format = {});
			prosper::Format format = prosper::Format::R8G8B8A8_UNorm;
			uint32_t startOffset = std::numeric_limits<uint32_t>::max();
			const VertexBinding *binding = nullptr;

			uint32_t GetLocation() const;
		  private:
			friend void ShaderGraphics::PrepareGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
			std::function<void()> initializer = nullptr;

			uint32_t location = std::numeric_limits<uint32_t>::max();
		};
		enum class RecordFlags : uint32_t {
			None = 0u,
			RenderPassTargetAsViewport = 1u,
			RenderPassTargetAsScissor = RenderPassTargetAsViewport << 1u,

			RenderPassTargetAsViewportAndScissor = RenderPassTargetAsViewport | RenderPassTargetAsScissor
		};

		ShaderGraphics(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");
		virtual ~ShaderGraphics() override;
		virtual bool RecordBindDescriptorSet(ShaderBindState &bindState, prosper::IDescriptorSet &descSet, uint32_t firstSet = 0u, const std::vector<uint32_t> &dynamicOffsets = {}) const override;
		bool RecordBindRenderBuffer(ShaderBindState &bindState, const IRenderBuffer &renderBuffer) const;
		bool RecordBindVertexBuffers(ShaderBindState &bindState, const std::vector<IBuffer *> &buffers, uint32_t startBinding = 0u, const std::vector<DeviceSize> &offsets = {}) const;
		bool RecordBindVertexBuffer(ShaderBindState &bindState, prosper::IBuffer &buffer, uint32_t startBinding = 0u, DeviceSize offset = 0ull) const;
		bool RecordBindIndexBuffer(ShaderBindState &bindState, prosper::IBuffer &indexBuffer, prosper::IndexType indexType = prosper::IndexType::UInt16, DeviceSize offset = 0ull) const;
		bool RecordDraw(ShaderBindState &bindState, uint32_t vertCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) const;
		bool RecordDrawIndexed(ShaderBindState &bindState, uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, uint32_t firstInstance = 0u) const;
		void AddVertexAttribute(prosper::GraphicsPipelineCreateInfo &pipelineInfo, VertexAttribute &attr);
		bool AddSpecializationConstant(prosper::GraphicsPipelineCreateInfo &pipelineInfo, prosper::ShaderStageFlags stageFlags, uint32_t constantId, uint32_t numBytes, const void *data);
		template<typename T>
		bool AddSpecializationConstant(prosper::GraphicsPipelineCreateInfo &pipelineInfo, prosper::ShaderStageFlags stageFlags, uint32_t constantId, const T &value)
		{
			return AddSpecializationConstant(pipelineInfo, stageFlags, constantId, sizeof(T), &value);
		}
		virtual bool RecordBeginDraw(ShaderBindState &bindState, uint32_t pipelineIdx = 0u, RecordFlags recordFlags = RecordFlags::RenderPassTargetAsViewportAndScissor) const;
		virtual bool RecordDraw(ShaderBindState &bindState) const;
		virtual void RecordEndDraw(ShaderBindState &bindState) const;

		const std::shared_ptr<IRenderPass> &GetRenderPass(uint32_t pipelineIdx = 0u) const;
		template<class TShader>
		static const std::shared_ptr<IRenderPass> &GetRenderPass(prosper::IPrContext &context, uint32_t pipelineIdx = 0u);
	  protected:
		bool RecordBeginDrawViewport(ShaderBindState &bindState, uint32_t width, uint32_t height, uint32_t pipelineIdx = 0u, RecordFlags recordFlags = RecordFlags::RenderPassTargetAsViewportAndScissor) const;
		void SetGenericAlphaColorBlendAttachmentProperties(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
		void ToggleDynamicViewportState(prosper::GraphicsPipelineCreateInfo &pipelineInfo, bool bEnable);
		void ToggleDynamicScissorState(prosper::GraphicsPipelineCreateInfo &pipelineInfo, bool bEnable);
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx);
		virtual void InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx);

		void CreateCachedRenderPass(size_t hashCode, const prosper::util::RenderPassCreateInfo &renderPassInfo, std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx, const std::string &debugName = "");
		template<class TShader>
		void CreateCachedRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo, std::shared_ptr<IRenderPass> &outRenderPass, uint32_t pipelineIdx);
		static const std::shared_ptr<IRenderPass> &GetRenderPass(prosper::IPrContext &context, size_t hashCode, uint32_t pipelineIdx);
	  private:
		virtual void InitializePipeline() override;
		std::vector<std::reference_wrapper<VertexAttribute>> m_vertexAttributes = {};
	};

	class DLLPROSPER ShaderCompute : public Shader {
	  public:
		ShaderCompute(prosper::IPrContext &context, const std::string &identifier, const std::string &csShader);

		bool RecordDispatch(ShaderBindState &bindState, uint32_t x = 1u, uint32_t y = 1u, uint32_t z = 1u) const;
		virtual bool RecordBeginCompute(ShaderBindState &bindState, uint32_t pipelineIdx = 0u) const;
		virtual void RecordCompute(ShaderBindState &bindState) const;
		virtual void RecordEndCompute(ShaderBindState &bindState) const;
		bool AddSpecializationConstant(prosper::ComputePipelineCreateInfo &pipelineInfo, uint32_t constantId, uint32_t numBytes, const void *data);
	  protected:
		virtual void InitializeComputePipeline(prosper::ComputePipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx);
	  private:
		virtual void InitializePipeline() override;
	};

	class DLLPROSPER ShaderRaytracing : public Shader {
	  public:
		ShaderRaytracing(prosper::IPrContext &context, const std::string &identifier, const std::string &rtShader);
	  protected:
		virtual void InitializeRaytracingPipeline(prosper::RayTracingPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx);
	  private:
		virtual void InitializePipeline() override;
	};

	namespace util {
		DLLPROSPER void set_graphics_pipeline_polygon_mode(prosper::GraphicsPipelineCreateInfo &pipelineInfo, prosper::PolygonMode polygonMode);
		DLLPROSPER void set_graphics_pipeline_line_width(prosper::GraphicsPipelineCreateInfo &pipelineInfo, float lineWidth);
		DLLPROSPER void set_graphics_pipeline_cull_mode_flags(prosper::GraphicsPipelineCreateInfo &pipelineInfo, prosper::CullModeFlags cullModeFlags);
		DLLPROSPER void set_graphics_pipeline_front_face(prosper::GraphicsPipelineCreateInfo &pipelineInfo, prosper::FrontFace frontFace);
		DLLPROSPER void set_generic_alpha_color_blend_attachment_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo);

		enum class DynamicStateFlags : uint32_t {
			None = 0u,
			Viewport = 1u,
			Scissor = Viewport << 1u,
			LineWidth = Scissor << 1u,
			DepthBias = LineWidth << 1u,
			BlendConstants = DepthBias << 1u,
			DepthBounds = BlendConstants << 1u,
			StencilCompareMask = DepthBounds << 1u,
			StencilWriteMask = StencilCompareMask << 1u,
			StencilReference = StencilWriteMask << 1u,
#if 0
			ViewportWScalingNV = StencilReference<<1u,
			DiscardRectangleEXT = ViewportWScalingNV<<1u,
			SampleLocationsEXT = DiscardRectangleEXT<<1u,
			ViewportShadingRatePaletteNV = SampleLocationsEXT<<1u,
			ViewportCoarseSampleOrderNV = ViewportShadingRatePaletteNV<<1u,
			ExclusiveScissorNV = ViewportCoarseSampleOrderNV<<1u
#endif
		};
		DLLPROSPER void set_dynamic_states_enabled(prosper::GraphicsPipelineCreateInfo &pipelineInfo, DynamicStateFlags states, bool enabled = true);
		DLLPROSPER bool are_dynamic_states_enabled(prosper::GraphicsPipelineCreateInfo &pipelineInfo, DynamicStateFlags states);
		DLLPROSPER DynamicStateFlags get_enabled_dynamic_states(prosper::GraphicsPipelineCreateInfo &pipelineInfo);
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::DynamicStateFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ShaderGraphics::RecordFlags)

#pragma warning(pop)

#endif
