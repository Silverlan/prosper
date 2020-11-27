/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_CONTEXT_HPP__
#define __PROSPER_CONTEXT_HPP__

#include "prosper_definitions.hpp"
#include <array>
#include <queue>
#include <memory>
#include <chrono>
#include <optional>
#include "prosper_includes.hpp"
#include "prosper_structs.hpp"
#include "shader/prosper_shader_manager.hpp"
#include "prosper_common_buffer_cache.hpp"

#ifdef __linux__
#include <iglfw/glfw_window.h>
#endif

namespace GLFW
{
	class Window;
	struct WindowCreationInfo;
};

namespace uimg
{
	class ImageBuffer;
	struct TextureInfo;
};

#undef max
#undef CreateEvent

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class ShaderStageProgram
	{
	public:
		ShaderStageProgram()=default;
	};
	enum class Vendor : uint32_t
	{
		AMD = 4098,
		Nvidia = 4318,
		Intel = 32902,
		Unknown = std::numeric_limits<uint32_t>::max()
	};
	namespace util
	{
		struct BufferCreateInfo;
		struct SamplerCreateInfo;
		struct ImageViewCreateInfo;
		struct ImageCreateInfo;
		struct Limits;
		struct PhysicalDeviceImageFormatProperties;
	};

	struct ShaderStageData;
	class Texture;
	class RenderTarget;
	class IBuffer;
	class IUniformResizableBuffer;
	class IDynamicResizableBuffer;
	class ISampler;
	class IImageView;
	class IImage;
	class IRenderPass;
	class IFramebuffer;
	class IDescriptorSetGroup;
	class ICommandBuffer;
	class IQueryPool;
	class IPrimaryCommandBuffer;
	class ISecondaryCommandBuffer;
	class IFence;
	class IEvent;
	class TimestampQuery;
	class Query;
	class PipelineStatisticsQuery;
	class ComputePipelineCreateInfo;
	class GraphicsPipelineCreateInfo;
	class IRenderBuffer;
	struct DescriptorSetInfo;
	struct PipelineStatistics;
	struct ImageFormatPropertiesQuery;

	namespace glsl {struct Definitions;};

	struct DLLPROSPER Callbacks
	{
		std::function<void(prosper::DebugMessageSeverityFlags,const std::string&)> validationCallback = nullptr;
		std::function<void()> onWindowInitialized = nullptr;
		std::function<void()> onClose = nullptr;
		std::function<void(uint32_t,uint32_t)> onResolutionChanged = nullptr;
		std::function<void(prosper::IPrimaryCommandBuffer&,uint32_t)> drawFrame = nullptr;
	};

	using FrameIndex = uint64_t;
	class DLLPROSPER IPrContext
		: public std::enable_shared_from_this<IPrContext>
	{
	public:
		// Max push constant size supported by most vendors / GPUs
		static constexpr uint32_t MAX_COMMON_PUSH_CONSTANT_SIZE = 128;

		enum class StateFlags : uint32_t
		{
			None = 0u,
			IsRecording = 1u,
			ValidationEnabled = IsRecording<<1u,
			Initialized = ValidationEnabled<<1u,
			Idle = Initialized<<1u,
			Closed = Idle<<1u,
			ClearingKeepAliveResources = Closed<<1u
		};

		struct DLLPROSPER CreateInfo
		{
			struct DLLPROSPER DeviceInfo
			{
				DeviceInfo(Vendor vendorId,uint32_t deviceId)
					: vendorId{vendorId},deviceId{deviceId}
				{}
				Vendor vendorId;
				uint32_t deviceId;
			};
			uint32_t width = 0u;
			uint32_t height = 0u;
			prosper::PresentModeKHR presentMode = prosper::PresentModeKHR::Immediate;
			std::optional<DeviceInfo> device = {};
		};

		struct DLLPROSPER BufferUpdateInfo
		{
			std::optional<PipelineStageFlags> srcStageMask = util::PIPELINE_STAGE_SHADER_INPUT_FLAGS | PipelineStageFlags::TransferBit;
			std::optional<AccessFlags> srcAccessMask = AccessFlags::ShaderReadBit | AccessFlags::ShaderWriteBit | AccessFlags::TransferReadBit | AccessFlags::TransferWriteBit;
			std::optional<PipelineStageFlags> postUpdateBarrierStageMask = {};
			std::optional<AccessFlags> postUpdateBarrierAccessMask = {};
		};

		using ImageMipmapData = const uint8_t*;
		using ImageLayerData = std::vector<ImageMipmapData>;
		using ImageData = std::vector<ImageLayerData>;

		template<class TContext>
			static std::shared_ptr<TContext> Create(const std::string &appName,uint32_t width,uint32_t height,bool bEnableValidation=false);
		virtual ~IPrContext();

		virtual void Initialize(const CreateInfo &createInfo);
		virtual bool ApplyGLSLPostProcessing(std::string &inOutGlslCode,std::string &outErrMsg) const {return true;}
		virtual bool InitializeShaderSources(prosper::Shader &shader,bool bReload,std::string &outInfoLog,std::string &outDebugInfoLog,prosper::ShaderStage &outErrStage) const;
		virtual std::string GetAPIIdentifier() const=0;
		virtual std::string GetAPIAbbreviation() const=0;

		void Run();
		void Close();

		// Has to be called before the context has been initialized
		void SetValidationEnabled(bool b);
		bool IsValidationEnabled() const;

		GLFW::Window &GetWindow();
		std::array<uint32_t,2> GetWindowSize() const;
		uint32_t GetWindowWidth() const;
		uint32_t GetWindowHeight() const;
		const std::string &GetAppName() const;
		uint32_t GetSwapchainImageCount() const;
		prosper::IImage *GetSwapchainImage(uint32_t idx);

		virtual bool IsImageFormatSupported(
			prosper::Format format,prosper::ImageUsageFlags usageFlags,prosper::ImageType type=prosper::ImageType::e2D,
			prosper::ImageTiling tiling=prosper::ImageTiling::Optimal
		) const=0;
		virtual uint32_t GetUniversalQueueFamilyIndex() const=0;
		virtual util::Limits GetPhysicalDeviceLimits() const=0;
		virtual std::optional<util::PhysicalDeviceImageFormatProperties> GetPhysicalDeviceImageFormatProperties(const ImageFormatPropertiesQuery &query)=0;

		void ChangeResolution(uint32_t width,uint32_t height);
		void ChangePresentMode(prosper::PresentModeKHR presentMode);
		prosper::PresentModeKHR GetPresentMode() const;

		const GLFW::WindowCreationInfo &GetWindowCreationInfo() const;
		GLFW::WindowCreationInfo &GetWindowCreationInfo();
		virtual void ReloadWindow()=0;

		ShaderManager &GetShaderManager() const;

		void RegisterShader(const std::string &identifier,const std::function<Shader*(IPrContext&,const std::string&)> &fFactory);
		::util::WeakHandle<Shader> GetShader(const std::string &identifier) const;

		void GetScissorViewportInfo(Rect2D *out_scissors,Viewport *out_viewports); // TODO: Deprecated?
		virtual bool IsPresentationModeSupported(prosper::PresentModeKHR presentMode) const=0;
		virtual Vendor GetPhysicalDeviceVendor() const=0;
		virtual MemoryRequirements GetMemoryRequirements(IImage &img)=0;
		// Clamps the specified size in bytes to a percentage of the total available GPU memory
		virtual uint64_t ClampDeviceMemorySize(uint64_t size,float percentageOfGPUMemory,MemoryFeatureFlags featureFlags) const=0;
		virtual DeviceSize CalcBufferAlignment(BufferUsageFlags usageFlags)=0;

		virtual void GetGLSLDefinitions(glsl::Definitions &outDef) const=0;

		FrameIndex GetLastFrameId() const;
		void Draw(uint32_t n_swapchain_image);
		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetSetupCommandBuffer();
		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetDrawCommandBuffer() const;
		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetDrawCommandBuffer(uint32_t swapchainIdx) const;
		void FlushSetupCommandBuffer();

		void KeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource);
		template<class T>
			void ReleaseResource(T *resource)
		{
			KeepResourceAliveUntilPresentationComplete(std::shared_ptr<T>(resource,[](T *p) {delete p;}));
		}
		virtual bool SavePipelineCache()=0;

		const std::shared_ptr<Texture> &GetDummyTexture() const;
		const std::shared_ptr<Texture> &GetDummyCubemapTexture() const;
		const std::shared_ptr<IBuffer> &GetDummyBuffer() const;
		const std::shared_ptr<IDynamicResizableBuffer> &GetTemporaryBuffer() const;
		const std::vector<std::shared_ptr<IDynamicResizableBuffer>> &GetDeviceImageBuffers() const;

		std::shared_ptr<IBuffer> AllocateTemporaryBuffer(DeviceSize size,uint32_t alignment=0,const void *data=nullptr);
		void AllocateTemporaryBuffer(prosper::IImage &img,const void *data=nullptr);

		std::shared_ptr<IBuffer> AllocateDeviceImageBuffer(DeviceSize size,uint32_t alignment=0,const void *data=nullptr);
		void AllocateDeviceImageBuffer(prosper::IImage &img,const void *data=nullptr);

		virtual std::shared_ptr<prosper::IPrimaryCommandBuffer> AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)=0;
		virtual std::shared_ptr<prosper::ISecondaryCommandBuffer> AllocateSecondaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex)=0;
		virtual void SubmitCommandBuffer(prosper::ICommandBuffer &cmd,prosper::QueueFamilyType queueFamilyType,bool shouldBlock=false,prosper::IFence *fence=nullptr)=0;
		void SubmitCommandBuffer(prosper::ICommandBuffer &cmd,bool shouldBlock=false,prosper::IFence *fence=nullptr);

		bool IsRecording() const;

		bool ScheduleRecordUpdateBuffer(
			const std::shared_ptr<IBuffer> &buffer,uint64_t offset,uint64_t size,const void *data,
			const BufferUpdateInfo &updateInfo={}
		);
		template<typename T>
			bool ScheduleRecordUpdateBuffer(
				const std::shared_ptr<IBuffer> &buffer,uint64_t offset,const T &data,
				const BufferUpdateInfo &updateInfo={}
			);

		void WaitIdle();
		virtual void Flush()=0;
		virtual Result WaitForFence(const IFence &fence,uint64_t timeout=std::numeric_limits<uint64_t>::max()) const=0;
		virtual Result WaitForFences(const std::vector<IFence*> &fences,bool waitAll=true,uint64_t timeout=std::numeric_limits<uint64_t>::max()) const=0;
		virtual void DrawFrame(const std::function<void(const std::shared_ptr<prosper::IPrimaryCommandBuffer>&,uint32_t)> &drawFrame)=0;
		virtual bool Submit(ICommandBuffer &cmdBuf,bool shouldBlock=false,IFence *optFence=nullptr)=0;

		void RegisterResource(const std::shared_ptr<void> &resource);
		void ReleaseResource(void *resource);

		virtual std::shared_ptr<IBuffer> CreateBuffer(const util::BufferCreateInfo &createInfo,const void *data=nullptr)=0;
		std::shared_ptr<IUniformResizableBuffer> CreateUniformResizableBuffer(
			util::BufferCreateInfo createInfo,uint64_t bufferInstanceSize,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage=1.f,const void *data=nullptr,
			std::optional<DeviceSize> customAlignment={}
		);
		virtual std::shared_ptr<IDynamicResizableBuffer> CreateDynamicResizableBuffer(
			util::BufferCreateInfo createInfo,
			uint64_t maxTotalSize,float clampSizeToAvailableGPUMemoryPercentage=1.f,const void *data=nullptr
		)=0;
		virtual std::shared_ptr<IEvent> CreateEvent()=0;
		virtual std::shared_ptr<IFence> CreateFence(bool createSignalled=false)=0;
		virtual std::shared_ptr<ISampler> CreateSampler(const util::SamplerCreateInfo &createInfo)=0;
		std::shared_ptr<IImageView> CreateImageView(const util::ImageViewCreateInfo &createInfo,IImage &img);
		virtual std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo,const ImageData &imgData={})=0;
		std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo,const uint8_t *data);
		std::shared_ptr<IImage> CreateImage(uimg::ImageBuffer &imgBuffer,const std::optional<util::ImageCreateInfo> &imgCreateInfo={});
		std::shared_ptr<IImage> CreateCubemap(std::array<std::shared_ptr<uimg::ImageBuffer>,6> &imgBuffers,const std::optional<util::ImageCreateInfo> &imgCreateInfo={});
		virtual std::shared_ptr<IRenderPass> CreateRenderPass(const util::RenderPassCreateInfo &renderPassInfo)=0;
		std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(const DescriptorSetInfo &descSetInfo);
		virtual std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(DescriptorSetCreateInfo &descSetInfo)=0;
		virtual std::shared_ptr<IFramebuffer> CreateFramebuffer(uint32_t width,uint32_t height,uint32_t layers,const std::vector<prosper::IImageView*> &attachments)=0;
		std::shared_ptr<Texture> CreateTexture(
			const util::TextureCreateInfo &createInfo,IImage &img,
			const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo=util::ImageViewCreateInfo{},
			const std::optional<util::SamplerCreateInfo> &samplerCreateInfo=util::SamplerCreateInfo{}
		);
		std::shared_ptr<RenderTarget> CreateRenderTarget(const std::vector<std::shared_ptr<Texture>> &textures,const std::shared_ptr<IRenderPass> &rp=nullptr,const util::RenderTargetCreateInfo &rtCreateInfo={});
		std::shared_ptr<RenderTarget> CreateRenderTarget(Texture &texture,IImageView &imgView,IRenderPass &rp,const util::RenderTargetCreateInfo &rtCreateInfo={});
		virtual std::shared_ptr<IRenderBuffer> CreateRenderBuffer(
			const prosper::GraphicsPipelineCreateInfo &pipelineCreateInfo,const std::vector<prosper::IBuffer*> &buffers,
			const std::vector<prosper::DeviceSize> &offsets={},const std::optional<IndexBufferInfo> &indexBufferInfo={}
		)=0;
		virtual std::unique_ptr<ShaderModule> CreateShaderModuleFromStageData(
			const std::shared_ptr<ShaderStageProgram> &shaderStageProgram,
			prosper::ShaderStage stage,
			const std::string &entrypointName="main"
		)=0;
		virtual std::shared_ptr<ShaderStageProgram> CompileShader(prosper::ShaderStage stage,const std::string &shaderPath,std::string &outInfoLog,std::string &outDebugInfoLog,bool reload=false)=0;
		virtual std::optional<PipelineID> AddPipeline(
			prosper::Shader &shader,PipelineID shaderPipelineId,const prosper::ComputePipelineCreateInfo &createInfo,
			prosper::ShaderStageData &stage,PipelineID basePipelineId=std::numeric_limits<PipelineID>::max()
		)=0;
		virtual std::optional<PipelineID> AddPipeline(
			prosper::Shader &shader,PipelineID shaderPipelineId,
			const prosper::GraphicsPipelineCreateInfo &createInfo,IRenderPass &rp,
			prosper::ShaderStageData *shaderStageFs=nullptr,
			prosper::ShaderStageData *shaderStageVs=nullptr,
			prosper::ShaderStageData *shaderStageGs=nullptr,
			prosper::ShaderStageData *shaderStageTc=nullptr,
			prosper::ShaderStageData *shaderStageTe=nullptr,
			SubPassID subPassId=0,
			PipelineID basePipelineId=std::numeric_limits<PipelineID>::max()
		)=0;
		virtual bool ClearPipeline(bool graphicsShader,PipelineID pipelineId)=0;
		virtual uint32_t GetLastAcquiredSwapchainImageIndex() const=0;

		virtual std::shared_ptr<prosper::IQueryPool> CreateQueryPool(QueryType queryType,uint32_t maxConcurrentQueries)=0;
		virtual std::shared_ptr<prosper::IQueryPool> CreateQueryPool(QueryPipelineStatisticFlags statsFlags,uint32_t maxConcurrentQueries)=0;
		virtual bool QueryResult(const TimestampQuery &query,std::chrono::nanoseconds &outTimestampValue) const=0;
		virtual bool QueryResult(const PipelineStatisticsQuery &query,PipelineStatistics &outStatistics) const=0;
		virtual bool QueryResult(const Query &query,uint32_t &r) const=0;
		virtual bool QueryResult(const Query &query,uint64_t &r) const=0;

		void SetCallbacks(const Callbacks &callbacks);
		virtual void DrawFrame();
		virtual void EndFrame();
		void SetPresentMode(prosper::PresentModeKHR presentMode);

		virtual void AddDebugObjectInformation(std::string &msgValidation) {}
		bool ValidationCallback(
			DebugMessageSeverityFlags severityFlags,
			const std::string &message
		);
		CommonBufferCache &GetCommonBufferCache() const;

		virtual void *GetInternalDevice() const {return nullptr;}
		virtual void *GetInternalPhysicalDevice() const {return nullptr;}
		virtual void *GetInternalInstance() const {return nullptr;}
		virtual void *GetInternalUniversalQueue() const {return nullptr;}
		virtual bool IsDeviceExtensionEnabled(const std::string &ext) const {return false;}
		virtual bool IsInstanceExtensionEnabled(const std::string &ext) const {return false;}
	protected:
		IPrContext(const std::string &appName,bool bEnableValidation=false);
		void CalcAlignedSizes(uint64_t instanceSize,uint64_t &bufferBaseSize,uint64_t &maxTotalSize,uint32_t &alignment,prosper::BufferUsageFlags usageFlags);

		std::shared_ptr<IImage> CreateImage(const std::vector<std::shared_ptr<uimg::ImageBuffer>> &imgBuffer,const std::optional<util::ImageCreateInfo> &createInfo={});
		virtual std::shared_ptr<IImageView> DoCreateImageView(
			const util::ImageViewCreateInfo &createInfo,IImage &img,Format format,ImageViewType imgViewType,prosper::ImageAspectFlags aspectMask,uint32_t numLayers
		)=0;
		virtual void DoKeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource)=0;
		virtual void DoWaitIdle()=0;
		virtual void DoFlushSetupCommandBuffer()=0;
		virtual std::shared_ptr<IUniformResizableBuffer> DoCreateUniformResizableBuffer(
			const util::BufferCreateInfo &createInfo,uint64_t bufferInstanceSize,
			uint64_t maxTotalSize,const void *data,
			prosper::DeviceSize bufferBaseSize,uint32_t alignment
		)=0;
		virtual void OnClose();
		virtual void Release();
		virtual void InitBuffers();
		virtual void InitGfxPipelines();
		virtual void OnResolutionChanged(uint32_t width,uint32_t height);
		virtual void OnWindowInitialized();
		virtual void OnSwapchainInitialized();
		virtual void ReloadSwapchain()=0;

		virtual void DrawFrame(prosper::IPrimaryCommandBuffer &cmd_buffer_ptr,uint32_t n_current_swapchain_image);
	protected: // private: // TODO
		IPrContext(const IPrContext&)=delete;
		IPrContext &operator=(const IPrContext&)=delete;

		void ClearKeepAliveResources();
		void ClearKeepAliveResources(uint32_t n);
		void InitDummyTextures();
		void InitDummyBuffer();
		void InitTemporaryBuffer();
		virtual void InitAPI(const CreateInfo &createInfo)=0;

		prosper::PresentModeKHR m_presentMode = prosper::PresentModeKHR::Immediate;
		StateFlags m_stateFlags = StateFlags::Idle;

		std::string m_appName;
		std::shared_ptr<prosper::IPrimaryCommandBuffer> m_setupCmdBuffer = nullptr;

		Callbacks m_callbacks {};
		std::vector<std::vector<std::shared_ptr<void>>> m_keepAliveResources;
		std::unique_ptr<ShaderManager> m_shaderManager = nullptr;
		std::unique_ptr<GLFW::Window> m_glfwWindow = nullptr;
		std::shared_ptr<IDynamicResizableBuffer> m_tmpBuffer = nullptr;
		std::vector<std::shared_ptr<IDynamicResizableBuffer>> m_deviceImgBuffers = {};
		std::vector<std::shared_ptr<prosper::IImage>> m_swapchainImages {};
		uint32_t m_numSwapchainImages = 0u;

		std::queue<std::function<void(prosper::IPrimaryCommandBuffer&)>> m_scheduledBufferUpdates;
		std::vector<std::shared_ptr<prosper::IPrimaryCommandBuffer>> m_commandBuffers;

		uint32_t m_lastSemaporeUsed;
		uint32_t m_n_swapchain_image = 0u;

		std::shared_ptr<prosper::Texture> m_dummyTexture = nullptr;
		std::shared_ptr<prosper::Texture> m_dummyCubemapTexture = nullptr;
		std::shared_ptr<prosper::IBuffer> m_dummyBuffer = nullptr;

		std::unique_ptr<GLFW::WindowCreationInfo> m_windowCreationInfo = nullptr;
	private:
		mutable FrameIndex m_frameId = 0ull;
		mutable CommonBufferCache m_commonBufferCache;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::IPrContext::StateFlags)
#pragma warning(pop)

template<typename T>
	bool prosper::IPrContext::ScheduleRecordUpdateBuffer(const std::shared_ptr<IBuffer> &buffer,uint64_t offset,const T &data,const BufferUpdateInfo &updateInfo)
{
	return ScheduleRecordUpdateBuffer(buffer,offset,sizeof(data),&data,updateInfo);
}

template<class TContext>
	std::shared_ptr<TContext> prosper::IPrContext::Create(const std::string &appName,uint32_t width,uint32_t height,bool bEnableValidation)
{
	auto r = std::shared_ptr<TContext>(new TContext(appName,bEnableValidation));

	CreateInfo createInfo {};
	createInfo.width = width;
	createInfo.height = height;
	r->Initialize(createInfo);
	return r;
}

#endif
