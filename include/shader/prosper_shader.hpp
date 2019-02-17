/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SHADER_HPP__
#define __PROSPER_SHADER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_context_object.hpp"
#include <config.h>
#include <wrappers/shader_module.h>
#include <misc/graphics_pipeline_create_info.h>
#include <cinttypes>
#include <vulkan/vulkan.hpp>
#include <util/util_weak_handle.hpp>
#include <util/util_null_ref.hpp>
#include <unordered_map>
#include <optional>

#undef max

namespace Anvil
{
	class BasePipelineManager;
};

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	struct DLLPROSPER ShaderStage
	{
		std::unique_ptr<Anvil::ShaderModule,std::function<void(Anvil::ShaderModule*)>> module = nullptr;
		std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint = nullptr;
		Anvil::ShaderStage stage = Anvil::ShaderStage::FIRST;
		std::string path;
		std::vector<uint32_t> spirvBlob;
	};

	class DescriptorSetGroup;
	class RenderPass;
	class CommandBuffer;
	class PrimaryCommandBuffer;
	class ShaderManager;
	namespace util {struct RenderPassCreateInfo;};
	class DLLPROSPER Shader
		: public ContextObject,
		public std::enable_shared_from_this<Shader>
	{
	public:
		static void SetLogCallback(const std::function<void(Shader&,Anvil::ShaderStage,const std::string&,const std::string&)> &fLogCallback);
		static Shader *GetBoundPipeline(prosper::CommandBuffer &cmdBuffer,uint32_t &outPipelineIdx);
		static const std::unordered_map<prosper::CommandBuffer*,std::pair<prosper::Shader*,uint32_t>> &GetBoundPipelines();

		struct DLLPROSPER DescriptorSetInfo
		{
			struct DLLPROSPER Binding
			{
				Binding()=default;
				// If 'bindingIndex' is not specified, it will use the index of the previous binding, incremented by the previous array size
				Binding(Anvil::DescriptorType type,Anvil::ShaderStageFlags shaderStages,uint32_t descriptorArraySize=1u,uint32_t bindingIndex=std::numeric_limits<uint32_t>::max());
				Anvil::DescriptorType type = {};
				Anvil::ShaderStageFlags shaderStages = Anvil::ShaderStageFlagBits::ALL;
				uint32_t bindingIndex = std::numeric_limits<uint32_t>::max();
				uint32_t descriptorArraySize = 1u;
			};
			DescriptorSetInfo()=default;
			DescriptorSetInfo(const std::vector<Binding> &bindings);
			DescriptorSetInfo(const DescriptorSetInfo &)=default;
			DescriptorSetInfo(DescriptorSetInfo *parent,const std::vector<Binding> &bindings={});
			bool WasBaked() const;
			bool IsValid() const;
			mutable DescriptorSetInfo *parent = nullptr;
			std::vector<Binding> bindings;
			uint32_t setIndex = 0u; // This value will be set after the shader has been baked

			Anvil::DescriptorSetCreateInfoUniquePtr ToAnvilDescriptorSetInfo() const;
		private:
			friend Shader;
			Anvil::DescriptorSetCreateInfoUniquePtr Bake();
			bool m_bWasBaked = false;
		};
		//static int32_t Register(const std::string &identifier,const std::function<Shader*(Context&,const std::string&)> &fFactory);

		Shader(Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");
		Shader(Context &context,const std::string &identifier,const std::string &csShader);
		Shader(const Shader&)=delete;
		Shader &operator=(const Shader&)=delete;
		virtual ~Shader() override=default;

		void Release(bool bDelete=true);
		void SetDebugName(uint32_t pipelineIdx,const std::string &name);
		const std::string *GetDebugName(uint32_t pipelineIdx) const;

		::util::WeakHandle<const prosper::Shader> GetHandle() const;
		::util::WeakHandle<prosper::Shader> GetHandle();

		void Initialize(bool bReloadSourceCode=false);
		void ReloadPipelines(bool bReloadSourceCode=false);
		uint32_t GetPipelineCount() const;

		bool IsGraphicsShader() const;
		bool IsComputeShader() const;
		Anvil::PipelineBindPoint GetPipelineBindPoint() const;
		Anvil::PipelineLayout *GetPipelineLayout(uint32_t pipelineIdx=0u) const;
		const Anvil::BasePipelineCreateInfo *GetPipelineInfo(uint32_t pipelineIdx=0u) const;
		bool GetShaderStatistics(vk::ShaderStatisticsInfoAMD &stats,Anvil::ShaderStage stage,uint32_t pipelineIdx=0u) const;
		const Anvil::ShaderModuleStageEntryPoint *GetModuleStageEntryPoint(Anvil::ShaderStage stage,uint32_t pipelineIdx=0u) const;
		bool GetPipelineId(Anvil::PipelineID &pipelineId,uint32_t pipelineIdx=0u) const;

		// If a base shader is specified, pipelines will be created as derived pipelines
		// of the pipeline of that shader
		void SetBaseShader(Shader &shader);
		template<class TShader>
			void SetBaseShader();
		void ClearBaseShader();

		// Used for identifying groups of shader types (e.g. GUI shaders, 3D shaders, etc.), returns the hash code of the typeid for this base class
		virtual size_t GetBaseTypeHashCode() const;

		bool BindPipeline(Anvil::CommandBufferBase &cmdBuffer,uint32_t pipelineIdx=0u);
		template<class T>
			bool RecordPushConstants(const T &data,uint32_t offset=0u);
		bool RecordPushConstants(uint32_t size,const void *data,uint32_t offset=0u);
		bool RecordBindDescriptorSets(const std::vector<Anvil::DescriptorSet*> &descSets,uint32_t firstSet=0u,const std::vector<uint32_t> &dynamicOffsets={});
		void AddDescriptorSetGroup(Anvil::BasePipelineCreateInfo &pipelineInfo,DescriptorSetInfo &descSetInfo);
		bool AttachPushConstantRange(Anvil::BasePipelineCreateInfo &pipelineInfo,uint32_t offset,uint32_t size,Anvil::ShaderStageFlags stages);
		virtual bool RecordBindDescriptorSet(Anvil::DescriptorSet &descSet,uint32_t firstSet=0u,const std::vector<uint32_t> &dynamicOffsets={});

		Anvil::ShaderModule *GetModule(Anvil::ShaderStage stage);
		bool IsValid() const;
		const std::string &GetIdentifier() const;

		bool GetSourceFilePath(Anvil::ShaderStage stage,std::string &sourceFilePath) const;
		std::vector<std::string> GetSourceFilePaths() const;

		std::shared_ptr<DescriptorSetGroup> CreateDescriptorSetGroup(uint32_t setIdx,uint32_t pipelineIdx=0u) const;

		// Has to be called before the pipeline is initialized!
		void SetStageSourceFilePath(Anvil::ShaderStage stage,const std::string &filePath);
	protected:
		virtual void OnPipelineBound() {};
		virtual void OnPipelineUnbound() {};
		void UnbindPipeline();
		void SetIdentifier(const std::string &identifier);
		uint32_t GetCurrentPipelineIndex() const;
		std::shared_ptr<prosper::PrimaryCommandBuffer> GetCurrentCommandBuffer() const;
		void SetPipelineCount(uint32_t count);
		void SetCurrentDrawCommandBuffer(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx=std::numeric_limits<uint32_t>::max());
		virtual void InitializePipeline();
		virtual void OnPipelineInitialized(uint32_t pipelineIdx);
		// Called when the pipelines have been initialized for the first time
		virtual void OnInitialized();
		virtual void OnPipelinesInitialized();
		virtual bool ShouldInitializePipeline(uint32_t pipelineIdx);
		void ClearPipelines();

		struct PipelineInfo
		{
			Anvil::PipelineID id;
			std::shared_ptr<RenderPass> renderPass; // Only used for graphics shader
			std::string debugName;
		};
		ShaderStage *GetStage(Anvil::ShaderStage stage);
		const ShaderStage *GetStage(Anvil::ShaderStage stage) const;
		void InitializeDescriptorSetGroup(Anvil::BasePipelineCreateInfo &pipelineInfo);
		std::vector<PipelineInfo> m_pipelineInfos;
		// Pipeline this pipeline is derived from
		std::weak_ptr<Shader> m_basePipeline = {};
	private:
		std::weak_ptr<prosper::PrimaryCommandBuffer> m_currentCmd = {};
		static std::function<void(Shader&,Anvil::ShaderStage,const std::string&,const std::string&)> s_logCallback;
		using std::enable_shared_from_this<Shader>::shared_from_this;

		virtual Anvil::BasePipelineManager *GetPipelineManager() const=0;
		bool InitializeSources(bool bReload=false);
		void InitializeStages();
		
		std::array<std::shared_ptr<ShaderStage>,umath::to_integral(Anvil::ShaderStage::COUNT)> m_stages;
		std::vector<std::unique_ptr<Anvil::DescriptorSetCreateInfo>> m_dsInfos = {};
		uint32_t m_currentPipelineIdx = std::numeric_limits<uint32_t>::max();
		bool m_bValid = false;
		bool m_bFirstTimeInit = true;
		std::string m_identifier;

		Anvil::PipelineBindPoint m_pipelineBindPoint = Anvil::PipelineBindPoint::UNKNOWN;
	};

	class DLLPROSPER ShaderGraphics
		: public Shader
	{
	protected:
		virtual void PrepareGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);
	public:
		struct DLLPROSPER VertexBinding
		{
			VertexBinding()=default;
			VertexBinding(Anvil::VertexInputRate inputRate,uint32_t stride=std::numeric_limits<uint32_t>::max());
			// TODO: Use std::optional instead of null references
			VertexBinding(const VertexBinding &vbOther,const Anvil::VertexInputRate &inputRate=::util::nullref<Anvil::VertexInputRate>(),const uint32_t &stride=::util::nullref<uint32_t>());
			uint32_t stride = std::numeric_limits<uint32_t>::max();
			Anvil::VertexInputRate inputRate = Anvil::VertexInputRate::VERTEX;

			uint32_t GetBindingIndex() const;
		private:
			friend void ShaderGraphics::PrepareGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);
			std::function<void()> initializer = nullptr;

			uint32_t bindingIndex = std::numeric_limits<uint32_t>::max();
		};
		struct DLLPROSPER VertexAttribute
		{
			VertexAttribute()=default;
			VertexAttribute(const VertexBinding &binding,Anvil::Format format);
			VertexAttribute(
				const VertexAttribute &vaOther,const VertexBinding &binding=::util::nullref<VertexBinding>(),
				const Anvil::Format &format=::util::nullref<Anvil::Format>()
			);
			Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;
			uint32_t startOffset = std::numeric_limits<uint32_t>::max();
			const VertexBinding &binding;

			uint32_t GetLocation() const;
		private:
			friend void ShaderGraphics::PrepareGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);
			std::function<void()> initializer = nullptr;

			uint32_t location = std::numeric_limits<uint32_t>::max();
		};
		enum class RecordFlags : uint32_t
		{
			None = 0u,
			RenderPassTargetAsViewport = 1u,
			RenderPassTargetAsScissor = RenderPassTargetAsViewport<<1u,

			RenderPassTargetAsViewportAndScissor = RenderPassTargetAsViewport | RenderPassTargetAsScissor
		};

		ShaderGraphics(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");
		virtual ~ShaderGraphics() override;
		virtual bool RecordBindDescriptorSet(Anvil::DescriptorSet &descSet,uint32_t firstSet=0u,const std::vector<uint32_t> &dynamicOffsets={}) override;
		bool RecordBindVertexBuffers(const std::vector<Anvil::Buffer*> &buffers,uint32_t startBinding=0u,const std::vector<vk::DeviceSize> &offsets={});
		bool RecordBindVertexBuffer(Anvil::Buffer &buffer,uint32_t startBinding=0u,vk::DeviceSize offset=0ull);
		bool RecordBindIndexBuffer(Anvil::Buffer &indexBuffer,Anvil::IndexType indexType=Anvil::IndexType::UINT16,vk::DeviceSize offset=0ull);
		bool RecordDraw(uint32_t vertCount,uint32_t instanceCount=1u,uint32_t firstVertex=0u,uint32_t firstInstance=0u);
		bool RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount=1u,uint32_t firstIndex=0u,int32_t vertexOffset=0,uint32_t firstInstance=0u);
		void AddVertexAttribute(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,VertexAttribute &attr);
		virtual bool BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx=0u,RecordFlags recordFlags=RecordFlags::RenderPassTargetAsViewportAndScissor);
		virtual bool Draw();
		virtual void EndDraw();

		const std::shared_ptr<RenderPass> &GetRenderPass(uint32_t pipelineIdx=0u) const;
		template<class TShader>
			static const std::shared_ptr<RenderPass> &GetRenderPass(prosper::Context &context,uint32_t pipelineIdx=0u);
	protected:
		void SetGenericAlphaColorBlendAttachmentProperties(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);
		void ToggleDynamicViewportState(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable);
		void ToggleDynamicScissorState(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable);
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx);
		virtual void InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx);

		void CreateCachedRenderPass(size_t hashCode,const prosper::util::RenderPassCreateInfo &renderPassInfo,std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx,const std::string &debugName="");
		template<class TShader>
			void CreateCachedRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo,std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx);
		static const std::shared_ptr<RenderPass> &GetRenderPass(prosper::Context &context,size_t hashCode,uint32_t pipelineIdx);
	private:
		virtual void InitializePipeline() override;
		virtual Anvil::BasePipelineManager *GetPipelineManager() const override;
		std::vector<std::reference_wrapper<VertexAttribute>> m_vertexAttributes = {};
	};

	class DLLPROSPER ShaderCompute
		: public Shader
	{
	public:
		ShaderCompute(prosper::Context &context,const std::string &identifier,const std::string &csShader);

		bool RecordDispatch(uint32_t x=1u,uint32_t y=1u,uint32_t z=1u);
		virtual bool BeginCompute(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx=0u);
		virtual void Compute();
		virtual void EndCompute();
	protected:
		virtual void InitializeComputePipeline(Anvil::ComputePipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx);
	private:
		virtual void InitializePipeline() override;
		virtual Anvil::BasePipelineManager *GetPipelineManager() const override;
	};

	namespace util
	{
		DLLPROSPER std::shared_ptr<DescriptorSetGroup> create_descriptor_set_group(Anvil::BaseDevice &dev,const Shader::DescriptorSetInfo &descSetInfo);
		DLLPROSPER void set_graphics_pipeline_polygon_mode(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::PolygonMode polygonMode);
		DLLPROSPER void set_graphics_pipeline_line_width(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,float lineWidth);
		DLLPROSPER void set_graphics_pipeline_cull_mode_flags(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::CullModeFlags cullModeFlags);
		DLLPROSPER void set_graphics_pipeline_front_face(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::FrontFace frontFace);
		DLLPROSPER void set_generic_alpha_color_blend_attachment_properties(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);

		enum class DynamicStateFlags : uint32_t
		{
			None = 0u,
			Viewport = 1u,
			Scissor = Viewport<<1u,
			LineWidth = Scissor<<1u,
			DepthBias = LineWidth<<1u,
			BlendConstants = DepthBias<<1u,
			DepthBounds = BlendConstants<<1u,
			StencilCompareMask = DepthBounds<<1u,
			StencilWriteMask = StencilCompareMask<<1u,
			StencilReference = StencilWriteMask<<1u,
			ViewportWScalingNV = StencilReference<<1u,
			DiscardRectangleEXT = ViewportWScalingNV<<1u,
			SampleLocationsEXT = DiscardRectangleEXT<<1u,
			ViewportShadingRatePaletteNV = SampleLocationsEXT<<1u,
			ViewportCoarseSampleOrderNV = ViewportShadingRatePaletteNV<<1u,
			ExclusiveScissorNV = ViewportCoarseSampleOrderNV<<1u
		};
		DLLPROSPER void set_dynamic_states_enabled(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states,bool enabled=true);
		DLLPROSPER bool are_dynamic_states_enabled(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states);
		DLLPROSPER DynamicStateFlags get_enabled_dynamic_states(Anvil::GraphicsPipelineCreateInfo &pipelineInfo);
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::DynamicStateFlags)
REGISTER_BASIC_BITWISE_OPERATORS(prosper::ShaderGraphics::RecordFlags)

template<class TShader>
	void prosper::Shader::SetBaseShader()
{
	auto &shaderManager = GetContext().GetShaderManager();
	auto *pShader = shaderManager.template FindShader<TShader>();
	if(pShader == nullptr)
		throw std::logic_error("Cannot derive from base shader which hasn't been registered!");
	SetBaseShader(*pShader);
}

template<class TShader>
	const std::shared_ptr<prosper::RenderPass> &prosper::ShaderGraphics::GetRenderPass(prosper::Context &context,uint32_t pipelineIdx)
{
	return GetRenderPass(context,typeid(TShader).hash_code(),pipelineIdx);
}

template<class TShader>
	void prosper::ShaderGraphics::CreateCachedRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo,std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	return CreateCachedRenderPass(typeid(TShader).hash_code(),renderPassInfo,outRenderPass,pipelineIdx,"shader_" +std::string(typeid(TShader).name()) +std::string("_rp"));
}

template<class T>
	bool prosper::Shader::RecordPushConstants(const T &data,uint32_t offset)
{
	return RecordPushConstants(sizeof(data),&data,offset);
}
#pragma warning(pop)
/*#ifndef PP_CAT
#define PP_CAT(a,b) PP_CAT_I(a,b)
#define PP_CAT_I(a,b) PP_CAT_II(~,a ## b)
#define PP_CAT_II(p,res) res
#endif
#define REGISTER_SHADER(ppIdentifier,ppShader) \
	auto PP_CAT(__shader,__LINE__) = Shader::Register(ppIdentifier,[](Context &context,const std::string &identifier) -> Shader* {return new ppShader(context,identifier);});
	*/
#endif
