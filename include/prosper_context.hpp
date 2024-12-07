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
#include <mutex>
#include <thread>
#include <functional>
#include <iglfw/glfw_window.h>
#include <sharedutils/util_log.hpp>
#include "prosper_includes.hpp"
#include "prosper_structs.hpp"
#include "shader/prosper_shader_manager.hpp"
#include "prosper_common_buffer_cache.hpp"

#ifdef __linux__
#include <iglfw/glfw_window.h>
#endif

namespace GLFW {
	class Window;
};

namespace uimg {
	class ImageBuffer;
	struct TextureInfo;
};

#undef max
#undef CreateEvent
#undef CreateWindow

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	struct WindowSettings;
	class Shader;
	class ShaderStageProgram {
	  public:
		ShaderStageProgram() = default;
	};
	struct DLLPROSPER ShaderPipeline {
		ShaderPipeline() = default;
		ShaderPipeline(prosper::Shader &shader, uint32_t pipeline);
		::util::WeakHandle<prosper::Shader> shader {};
		uint32_t pipeline = 0;
	};
	enum class Vendor : uint32_t { AMD = 4098, Nvidia = 4318, Intel = 32902, Unknown = std::numeric_limits<uint32_t>::max() };
	enum class DescriptorResourceType : uint8_t;
	namespace util {
		struct BufferCreateInfo;
		struct SamplerCreateInfo;
		struct ImageViewCreateInfo;
		struct ImageCreateInfo;
		struct Limits;
		struct PhysicalDeviceImageFormatProperties;
		struct VendorDeviceInfo;
		struct PhysicalDeviceMemoryProperties;
	};

	struct ShaderStageData;
	class Texture;
	class RenderTarget;
	class IBuffer;
	class IDescriptorSet;
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
	class ICommandBufferPool;
	class ISwapCommandBufferGroup;
	class IFence;
	class IEvent;
	class IShaderPipelineLayout;
	class TimestampQuery;
	class Query;
	class PipelineStatisticsQuery;
	class ComputePipelineCreateInfo;
	class RayTracingPipelineCreateInfo;
	class GraphicsPipelineCreateInfo;
	class IRenderBuffer;
	struct DescriptorSetInfo;
	struct PipelineStatistics;
	struct ImageFormatPropertiesQuery;

	namespace glsl {
		struct Definitions;
	};

	struct DLLPROSPER Callbacks {
		std::function<void(prosper::DebugMessageSeverityFlags, const std::string &)> validationCallback = nullptr;
		std::function<void()> onWindowInitialized = nullptr;
		std::function<void()> onClose = nullptr;
		std::function<void(uint32_t, uint32_t)> onResolutionChanged = nullptr;
		std::function<void()> drawFrame = nullptr;
	};

	namespace detail {
		struct DLLPROSPER BufferUpdateInfo {
			std::optional<PipelineStageFlags> srcStageMask = util::PIPELINE_STAGE_SHADER_INPUT_FLAGS | PipelineStageFlags::TransferBit;
			std::optional<AccessFlags> srcAccessMask = AccessFlags::ShaderReadBit | AccessFlags::ShaderWriteBit | AccessFlags::TransferReadBit | AccessFlags::TransferWriteBit;
			std::optional<PipelineStageFlags> postUpdateBarrierStageMask = {};
			std::optional<AccessFlags> postUpdateBarrierAccessMask = {};
		};
	};

	using FrameIndex = uint64_t;
	class Window;
	class ShaderPipelineLoader;
	class DLLPROSPER IPrContext : public std::enable_shared_from_this<IPrContext> {
	  public:
		// Max push constant size supported by most vendors / GPUs
		static constexpr uint32_t MAX_COMMON_PUSH_CONSTANT_SIZE = 128;

		enum class StateFlags : uint32_t {
			None = 0u,
			IsRecording = 1u,
			ValidationEnabled = IsRecording << 1u,
			Initialized = ValidationEnabled << 1u,
			Idle = Initialized << 1u,
			Closed = Idle << 1u,
			ClearingKeepAliveResources = Closed << 1u,
			EnableMultiThreadedRendering = ClearingKeepAliveResources << 1u,
			WindowScheduledForClosing = EnableMultiThreadedRendering << 1u
		};

		enum class ExtensionAvailability : uint8_t {
			EnableIfAvailable,
			Ignore,
			Require,
		};

		using BufferUpdateInfo = detail::BufferUpdateInfo; // Workaround for gcc bug, see https://stackoverflow.com/a/53423881/2482983
		struct DLLPROSPER CreateInfo {
			struct DLLPROSPER DeviceInfo {
				DeviceInfo(Vendor vendorId, uint32_t deviceId) : vendorId {vendorId}, deviceId {deviceId} {}
				Vendor vendorId;
				uint32_t deviceId;
			};
			uint32_t width = 0u;
			uint32_t height = 0u;
			prosper::PresentModeKHR presentMode = prosper::PresentModeKHR::Immediate;
			bool windowless = false;
			std::optional<DeviceInfo> device = {};

			std::unordered_map<std::string, ExtensionAvailability> extensions;
			std::vector<std::string> layers;
			std::vector<LayerSetting> layerSettings;
		};

		using ImageMipmapData = const uint8_t *;
		using ImageLayerData = std::vector<ImageMipmapData>;
		using ImageData = std::vector<ImageLayerData>;

		template<class TContext>
		static std::shared_ptr<TContext> Create(const std::string &appName, uint32_t width, uint32_t height, bool bEnableValidation = false);
		virtual ~IPrContext();

		virtual void Initialize(const CreateInfo &createInfo);
		virtual bool ApplyGLSLPostProcessing(std::string &inOutGlslCode, std::string &outErrMsg) const { return true; }
		virtual bool InitializeShaderSources(prosper::Shader &shader, bool bReload, std::string &outInfoLog, std::string &outDebugInfoLog, prosper::ShaderStage &outErrStage, const std::string &prefixCode = {}, const std::unordered_map<std::string, std::string> &definitions = {}) const;
		virtual std::string GetAPIIdentifier() const = 0;
		virtual std::string GetAPIAbbreviation() const = 0;
		virtual bool WaitForCurrentSwapchainCommandBuffer(std::string &outErrMsg) = 0;
		virtual std::shared_ptr<Window> CreateWindow(const WindowSettings &windowCreationInfo) = 0;

		void SetPreDeviceCreationCallback(const std::function<void(const prosper::util::VendorDeviceInfo &)> &callback);

		void SetLogHandler(const ::util::LogHandler &logHandler, const std::function<bool(::util::LogSeverity)> &getLevel = nullptr);
		bool ShouldLog() const;
		bool ShouldLog(::util::LogSeverity level) const;
		void Log(const std::string &msg, ::util::LogSeverity level = ::util::LogSeverity::Info) const;

		void SetProfilingHandler(const std::function<void(const char *)> &startProfling, const std::function<void()> &endProfling);
		void StartProfiling(const char *name) const;
		void EndProfiling() const;

		void Run();
		void Close();

		// Has to be called before the context has been initialized
		void SetValidationEnabled(bool b);
		bool IsValidationEnabled() const;

		// Invoke a crash intentionally to test crash handling
		void Crash();

		virtual bool ShouldFlipTexturesOnLoad() const { return false; }
		virtual uint32_t GetReservedDescriptorResourceCount(DescriptorResourceType resType) const { return 0; }

		std::thread::id GetMainThreadId() const { return m_mainThreadId; }

		Window &GetWindow();
		const Window &GetWindow() const { return const_cast<IPrContext *>(this)->GetWindow(); }
		const std::vector<std::shared_ptr<Window>> &GetWindows() const { return const_cast<IPrContext *>(this)->GetWindows(); }
		std::vector<std::shared_ptr<Window>> &GetWindows() { return m_windows; }
		std::array<uint32_t, 2> GetWindowSize() const;
		uint32_t GetWindowWidth() const;
		uint32_t GetWindowHeight() const;
		const std::string &GetAppName() const;
		uint32_t GetPrimaryWindowSwapchainImageCount() const;
		prosper::IImage *GetSwapchainImage(uint32_t idx);
		prosper::IFramebuffer *GetSwapchainFramebuffer(uint32_t idx);

		virtual bool IsImageFormatSupported(prosper::Format format, prosper::ImageUsageFlags usageFlags, prosper::ImageType type = prosper::ImageType::e2D, prosper::ImageTiling tiling = prosper::ImageTiling::Optimal) const = 0;
		virtual uint32_t GetUniversalQueueFamilyIndex() const = 0;
		virtual util::Limits GetPhysicalDeviceLimits() const = 0;
		virtual std::optional<util::PhysicalDeviceImageFormatProperties> GetPhysicalDeviceImageFormatProperties(const ImageFormatPropertiesQuery &query) = 0;
		std::optional<util::PhysicalDeviceImageFormatProperties> GetPhysicalDeviceImageFormatProperties(const util::ImageCreateInfo &imgCreateInfo);
		// If image tiling is not specified, return value will refer to buffer feature support
		virtual prosper::FeatureSupport AreFormatFeaturesSupported(Format format, FormatFeatureFlags featureFlags, std::optional<ImageTiling> tiling) const = 0;
		virtual void BakeShaderPipeline(prosper::PipelineID pipelineId, prosper::PipelineBindPoint pipelineType) = 0;

		void ChangeResolution(uint32_t width, uint32_t height);
		void ChangePresentMode(prosper::PresentModeKHR presentMode);
		prosper::PresentModeKHR GetPresentMode() const;

		const WindowSettings &GetInitialWindowSettings() const;
		WindowSettings &GetInitialWindowSettings();
		virtual void ReloadWindow() = 0;

		ShaderManager &GetShaderManager() const;

		void RegisterShader(const std::string &identifier, const std::function<Shader *(IPrContext &, const std::string &)> &fFactory);
		::util::WeakHandle<Shader> GetShader(const std::string &identifier) const;

		void GetScissorViewportInfo(Rect2D *out_scissors, Viewport *out_viewports); // TODO: Deprecated?
		virtual bool IsPresentationModeSupported(prosper::PresentModeKHR presentMode) const = 0;
		virtual Vendor GetPhysicalDeviceVendor() const = 0;
		virtual MemoryRequirements GetMemoryRequirements(IImage &img) = 0;
		// Clamps the specified size in bytes to a percentage of the total available GPU memory
		virtual uint64_t ClampDeviceMemorySize(uint64_t size, float percentageOfGPUMemory, MemoryFeatureFlags featureFlags) const = 0;
		virtual DeviceSize CalcBufferAlignment(BufferUsageFlags usageFlags) = 0;

		virtual void GetGLSLDefinitions(glsl::Definitions &outDef) const = 0;

		FrameIndex GetLastFrameId() const;
		void Draw();
		const std::shared_ptr<prosper::IPrimaryCommandBuffer> &GetSetupCommandBuffer();
		void FlushCommandBuffer(prosper::ICommandBuffer &cmd);
		void FlushSetupCommandBuffer();

		void KeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource);
		template<class T>
		void ReleaseResource(T *resource)
		{
			KeepResourceAliveUntilPresentationComplete(std::shared_ptr<T>(resource, [](T *p) { delete p; }));
		}
		virtual bool SavePipelineCache() = 0;

		const std::shared_ptr<Texture> &GetDummyTexture() const;
		const std::shared_ptr<Texture> &GetDummyCubemapTexture() const;
		const std::shared_ptr<IBuffer> &GetDummyBuffer() const;
		const std::shared_ptr<IDynamicResizableBuffer> &GetTemporaryBuffer() const;
		const std::vector<std::shared_ptr<IDynamicResizableBuffer>> &GetDeviceImageBuffers() const;

		std::shared_ptr<IBuffer> AllocateTemporaryBuffer(DeviceSize size, uint32_t alignment = 0, const void *data = nullptr);
		void AllocateTemporaryBuffer(prosper::IImage &img, const void *data = nullptr);

		std::shared_ptr<IBuffer> AllocateDeviceImageBuffer(DeviceSize size, uint32_t alignment = 0, const void *data = nullptr);
		void AllocateDeviceImageBuffer(prosper::IImage &img, const void *data = nullptr);

		virtual std::shared_ptr<prosper::IPrimaryCommandBuffer> AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType, uint32_t &universalQueueFamilyIndex) = 0;
		virtual std::shared_ptr<prosper::ISecondaryCommandBuffer> AllocateSecondaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType, uint32_t &universalQueueFamilyIndex) = 0;
		virtual std::shared_ptr<prosper::ICommandBufferPool> CreateCommandBufferPool(prosper::QueueFamilyType queueFamilyType) = 0;
		virtual void SubmitCommandBuffer(prosper::ICommandBuffer &cmd, prosper::QueueFamilyType queueFamilyType, bool shouldBlock = false, prosper::IFence *fence = nullptr) = 0;
		void SubmitCommandBuffer(prosper::ICommandBuffer &cmd, bool shouldBlock = false, prosper::IFence *fence = nullptr);

		bool IsRecording() const;

		bool ScheduleRecordUpdateBuffer(const std::shared_ptr<IBuffer> &buffer, uint64_t offset, uint64_t size, const void *data, const BufferUpdateInfo &updateInfo = {});
		template<typename T>
		bool ScheduleRecordUpdateBuffer(const std::shared_ptr<IBuffer> &buffer, uint64_t offset, const T &data, const BufferUpdateInfo &updateInfo = {});

		void WaitIdle(bool forceWait = true);
		virtual void Flush() = 0;
		virtual Result WaitForFence(const IFence &fence, uint64_t timeout = std::numeric_limits<uint64_t>::max()) const = 0;
		virtual Result WaitForFences(const std::vector<IFence *> &fences, bool waitAll = true, uint64_t timeout = std::numeric_limits<uint64_t>::max()) const = 0;
		virtual void DrawFrame(const std::function<void()> &drawFrame) = 0;
		virtual bool Submit(ICommandBuffer &cmdBuf, bool shouldBlock = false, IFence *optFence = nullptr) = 0;

		void RegisterResource(const std::shared_ptr<void> &resource);
		void ReleaseResource(void *resource);

		virtual std::shared_ptr<IBuffer> CreateBuffer(const util::BufferCreateInfo &createInfo, const void *data = nullptr) = 0;
		std::shared_ptr<IUniformResizableBuffer> CreateUniformResizableBuffer(util::BufferCreateInfo createInfo, uint64_t bufferInstanceSize, uint64_t maxTotalSize, float clampSizeToAvailableGPUMemoryPercentage = 1.f, const void *data = nullptr,
		  std::optional<DeviceSize> customAlignment = {});
		virtual std::shared_ptr<IDynamicResizableBuffer> CreateDynamicResizableBuffer(util::BufferCreateInfo createInfo, uint64_t maxTotalSize, float clampSizeToAvailableGPUMemoryPercentage = 1.f, const void *data = nullptr) = 0;
		virtual std::shared_ptr<IEvent> CreateEvent() = 0;
		virtual std::shared_ptr<IFence> CreateFence(bool createSignalled = false) = 0;
		virtual std::shared_ptr<ISampler> CreateSampler(const util::SamplerCreateInfo &createInfo) = 0;
		std::shared_ptr<IImageView> CreateImageView(const util::ImageViewCreateInfo &createInfo, IImage &img);
		virtual std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo, const std::function<const uint8_t *(uint32_t layer, uint32_t mipmap, uint32_t &dataSize, uint32_t &rowSize)> &getImageData = nullptr) = 0;
		std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo, const ImageData &imgData);
		std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo, const uint8_t *data);
		std::shared_ptr<IImage> CreateImage(uimg::ImageBuffer &imgBuffer, const std::optional<util::ImageCreateInfo> &imgCreateInfo = {});
		std::shared_ptr<IImage> CreateCubemap(std::array<std::shared_ptr<uimg::ImageBuffer>, 6> &imgBuffers, const std::optional<util::ImageCreateInfo> &imgCreateInfo = {});
		virtual std::shared_ptr<IRenderPass> CreateRenderPass(const util::RenderPassCreateInfo &renderPassInfo) = 0;
		std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(const DescriptorSetInfo &descSetInfo);
		virtual std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(DescriptorSetCreateInfo &descSetInfo) = 0;
		virtual std::shared_ptr<ISwapCommandBufferGroup> CreateSwapCommandBufferGroup(Window &window, bool allowMt = true, const std::string &debugName = {}) = 0;
		virtual std::shared_ptr<IFramebuffer> CreateFramebuffer(uint32_t width, uint32_t height, uint32_t layers, const std::vector<prosper::IImageView *> &attachments) = 0;
		virtual std::unique_ptr<IShaderPipelineLayout> GetShaderPipelineLayout(const Shader &shader, uint32_t pipelineIdx = 0u) const = 0;
		std::shared_ptr<Texture> CreateTexture(const util::TextureCreateInfo &createInfo, IImage &img, const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo = util::ImageViewCreateInfo {},
		  const std::optional<util::SamplerCreateInfo> &samplerCreateInfo = util::SamplerCreateInfo {});
		std::shared_ptr<RenderTarget> CreateRenderTarget(const std::vector<std::shared_ptr<Texture>> &textures, const std::shared_ptr<IRenderPass> &rp = nullptr, const util::RenderTargetCreateInfo &rtCreateInfo = {});
		std::shared_ptr<RenderTarget> CreateRenderTarget(Texture &texture, IImageView &imgView, IRenderPass &rp, const util::RenderTargetCreateInfo &rtCreateInfo = {});
		virtual std::shared_ptr<IRenderBuffer> CreateRenderBuffer(const prosper::GraphicsPipelineCreateInfo &pipelineCreateInfo, const std::vector<prosper::IBuffer *> &buffers, const std::vector<prosper::DeviceSize> &offsets = {}, const std::optional<IndexBufferInfo> &indexBufferInfo = {})
		  = 0;
		virtual std::unique_ptr<ShaderModule> CreateShaderModuleFromStageData(const std::shared_ptr<ShaderStageProgram> &shaderStageProgram, prosper::ShaderStage stage, const std::string &entrypointName = "main") = 0;
		virtual std::shared_ptr<ShaderStageProgram> CompileShader(prosper::ShaderStage stage, const std::string &shaderPath, std::string &outInfoLog, std::string &outDebugInfoLog, bool reload = false, const std::string &prefixCode = {},
		  const std::unordered_map<std::string, std::string> &definitions = {})
		  = 0;
		virtual std::optional<std::unordered_map<prosper::ShaderStage, std::string>> OptimizeShader(const std::unordered_map<prosper::ShaderStage, std::string> &shaderStages, std::string &outInfoLog) { return {}; }
		virtual bool GetParsedShaderSourceCode(prosper::Shader &shader, std::vector<std::string> &outGlslCodePerStage, std::vector<prosper::ShaderStage> &outGlslCodeStages, std::string &outInfoLog, std::string &outDebugInfoLog, prosper::ShaderStage &outErrStage) const = 0;
		std::optional<std::string> FindShaderFile(prosper::ShaderStage stage, const std::string &fileName, std::string *optOutExt = nullptr);
		virtual std::optional<PipelineID> AddPipeline(prosper::Shader &shader, PipelineID shaderPipelineId, const prosper::ComputePipelineCreateInfo &createInfo, prosper::ShaderStageData &stage, PipelineID basePipelineId = std::numeric_limits<PipelineID>::max()) = 0;
		virtual std::optional<PipelineID> AddPipeline(prosper::Shader &shader, PipelineID shaderPipelineId, const prosper::RayTracingPipelineCreateInfo &createInfo, prosper::ShaderStageData &stage, PipelineID basePipelineId = std::numeric_limits<PipelineID>::max()) = 0;
		virtual std::optional<PipelineID> AddPipeline(prosper::Shader &shader, PipelineID shaderPipelineId, const prosper::GraphicsPipelineCreateInfo &createInfo, IRenderPass &rp, prosper::ShaderStageData *shaderStageFs = nullptr, prosper::ShaderStageData *shaderStageVs = nullptr,
		  prosper::ShaderStageData *shaderStageGs = nullptr, prosper::ShaderStageData *shaderStageTc = nullptr, prosper::ShaderStageData *shaderStageTe = nullptr, SubPassID subPassId = 0, PipelineID basePipelineId = std::numeric_limits<PipelineID>::max())
		  = 0;
		prosper::Shader *GetShaderPipeline(PipelineID id, uint32_t &outPipelineIdx) const;
		virtual bool ClearPipeline(bool graphicsShader, PipelineID pipelineId) = 0;
		uint32_t GetLastAcquiredPrimaryWindowSwapchainImageIndex() const;

		virtual std::shared_ptr<prosper::IQueryPool> CreateQueryPool(QueryType queryType, uint32_t maxConcurrentQueries) = 0;
		virtual std::shared_ptr<prosper::IQueryPool> CreateQueryPool(QueryPipelineStatisticFlags statsFlags, uint32_t maxConcurrentQueries) = 0;
		virtual bool QueryResult(const TimestampQuery &query, std::chrono::nanoseconds &outTimestampValue) const = 0;
		virtual bool QueryResult(const PipelineStatisticsQuery &query, PipelineStatistics &outStatistics) const = 0;
		virtual bool QueryResult(const Query &query, uint32_t &r) const = 0;
		virtual bool QueryResult(const Query &query, uint64_t &r) const = 0;

		void SetCallbacks(const Callbacks &callbacks);
		virtual void DrawFrameCore();
		virtual void EndFrame();
		void SetPresentMode(prosper::PresentModeKHR presentMode);

		virtual std::optional<std::string> DumpMemoryBudget() const { return {}; }
		virtual std::optional<std::string> DumpMemoryStats() const { return {}; }
		virtual std::optional<std::string> DumpLimits() const { return {}; }
		virtual std::optional<std::string> DumpFeatures() const { return {}; }
		virtual std::optional<std::string> DumpFormatProperties() const { return {}; }
		virtual std::optional<std::string> DumpImageFormatProperties() const { return {}; }
		virtual std::optional<std::string> DumpLayers() const { return {}; }
		virtual std::optional<std::string> DumpExtensions() const { return {}; }
		virtual std::optional<util::VendorDeviceInfo> GetVendorDeviceInfo() const { return {}; }
		virtual std::optional<std::vector<util::VendorDeviceInfo>> GetAvailableVendorDevices() const { return {}; }
		virtual std::optional<util::PhysicalDeviceMemoryProperties> GetPhysicslDeviceMemoryProperties() const { return {}; }

		//DLLPROSPER_VK std::vector<util::VendorDeviceInfo> get_available_vendor_devices(const IPrContext &context);
		//DLLPROSPER_VK std::optional<util::PhysicalDeviceMemoryProperties> get_physical_device_memory_properties(const IPrContext &context);

		virtual void AddDebugObjectInformation(std::string &msgValidation) {}
		bool ValidationCallback(DebugMessageSeverityFlags severityFlags, const std::string &message);
		CommonBufferCache &GetCommonBufferCache() const;

		ShaderPipelineLoader &GetPipelineLoader();
		const ShaderPipelineLoader &GetPipelineLoader() const { return const_cast<IPrContext *>(this)->GetPipelineLoader(); }

		virtual void *GetInternalDevice() const { return nullptr; }
		virtual void *GetInternalPhysicalDevice() const { return nullptr; }
		virtual void *GetInternalInstance() const { return nullptr; }
		virtual void *GetInternalUniversalQueue() const { return nullptr; }
		virtual bool IsDeviceExtensionEnabled(const std::string &ext) const { return false; }
		virtual bool IsInstanceExtensionEnabled(const std::string &ext) const { return false; }

		void SetMultiThreadedRenderingEnabled(bool enabled);
		bool IsMultiThreadedRenderingEnabled() const { return umath::is_flag_set(m_stateFlags, StateFlags::EnableMultiThreadedRendering); }

		virtual bool SupportsMultiThreadedResourceAllocation() const { return false; }

		struct ShaderDescriptorSetBindingInfo {
			enum class Type : uint8_t { Buffer = 0, Image };
			uint32_t bindingPoint = 0;
			uint32_t bindingCount = 1;
			std::vector<std::string> args;
			Type type = Type::Buffer;
			std::string namedType;
		};
		struct ShaderDescriptorSetInfo {
			std::vector<ShaderDescriptorSetBindingInfo> bindingPoints {};
		};
		struct ShaderMacroLocation {
			size_t macroStart = 0;
			size_t macroEnd = 0;
			uint32_t setIndexIndex = 0;
			uint32_t bindingIndexIndex = 0;
		};
		static void ParseShaderUniforms(const std::string &glslShader, std::unordered_map<std::string, int32_t> &outDefinitions, std::vector<std::optional<ShaderDescriptorSetInfo>> &outDescSetInfos, std::vector<ShaderMacroLocation> &outMacroLocations);
		std::optional<std::chrono::steady_clock::time_point> GetLastUsageTime(IImage &img);
		std::optional<std::chrono::steady_clock::time_point> GetLastUsageTime(IBuffer &buf);
		void UpdateLastUsageTime(IImage &img);
		void UpdateLastUsageTime(IBuffer &buf);
		void UpdateLastUsageTimes(IDescriptorSet &ds);

		// Internal use only
		PipelineID ReserveShaderPipeline();
		void SetWindowScheduledForClosing();
		void CloseWindowsScheduledForClosing();
	  protected:
		IPrContext(const std::string &appName, bool bEnableValidation = false);
		void CalcAlignedSizes(uint64_t instanceSize, uint64_t &bufferBaseSize, uint64_t &maxTotalSize, uint32_t &alignment, prosper::BufferUsageFlags usageFlags);
		virtual void UpdateMultiThreadedRendering(bool mtEnabled);
		void ReloadPipelineLoader();
		void CheckDeviceLimits();

		std::shared_ptr<IImage> CreateImage(const std::vector<std::shared_ptr<uimg::ImageBuffer>> &imgBuffer, const std::optional<util::ImageCreateInfo> &createInfo = {});
		virtual std::shared_ptr<IImageView> DoCreateImageView(const util::ImageViewCreateInfo &createInfo, IImage &img, Format format, ImageViewType imgViewType, prosper::ImageAspectFlags aspectMask, uint32_t numLayers) = 0;
		virtual void DoKeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource) = 0;
		virtual void DoWaitIdle() = 0;
		virtual void DoFlushCommandBuffer(ICommandBuffer &cmd) = 0;
		virtual std::shared_ptr<IUniformResizableBuffer> DoCreateUniformResizableBuffer(const util::BufferCreateInfo &createInfo, uint64_t bufferInstanceSize, uint64_t maxTotalSize, const void *data, prosper::DeviceSize bufferBaseSize, uint32_t alignment) = 0;
		virtual void OnClose();
		virtual void Release();
		virtual void InitBuffers();
		virtual void InitGfxPipelines();
		virtual void OnResolutionChanged(uint32_t width, uint32_t height);
		virtual void OnWindowInitialized();
		virtual void OnSwapchainInitialized();
		virtual void ReloadSwapchain() = 0;

		virtual void DrawFrame();
	  protected: // private: // TODO
		IPrContext(const IPrContext &) = delete;
		IPrContext &operator=(const IPrContext &) = delete;

		void AddShaderPipeline(prosper::Shader &shader, uint32_t shaderPipelineIdx, PipelineID pipelineId);
		void ClearKeepAliveResources();
		void ClearKeepAliveResources(uint32_t n);
		void InitDummyTextures();
		void InitDummyBuffer();
		void InitTemporaryBuffer();
		void InitWindow();
		virtual void InitAPI(const CreateInfo &createInfo) = 0;
		virtual void OnSwapchainResourcesCleared(uint32_t swapchainIdx) {}

		prosper::PresentModeKHR m_presentMode = prosper::PresentModeKHR::Immediate;
		StateFlags m_stateFlags = StateFlags::Idle;
		std::mutex m_waitIdleMutex;

		std::string m_appName;
		std::shared_ptr<prosper::IPrimaryCommandBuffer> m_setupCmdBuffer = nullptr;
		std::vector<ShaderPipeline> m_shaderPipelines;
		std::unique_ptr<ShaderPipelineLoader> m_pipelineLoader;

		std::function<void(const prosper::util::VendorDeviceInfo &)> m_preDeviceCreationCallback = nullptr;

		Callbacks m_callbacks {};
		std::vector<std::vector<std::shared_ptr<void>>> m_keepAliveResources;
		std::unique_ptr<ShaderManager> m_shaderManager = nullptr;
		std::shared_ptr<Window> m_window = nullptr;
		std::vector<std::shared_ptr<Window>> m_windows {};
		std::shared_ptr<IDynamicResizableBuffer> m_tmpBuffer = nullptr;
		std::mutex m_tmpBufferMutex;
		std::vector<std::shared_ptr<IDynamicResizableBuffer>> m_deviceImgBuffers = {};
		std::mutex m_aliveResourceMutex;
		::util::LogHandler m_logHandler;
		std::function<bool(::util::LogSeverity)> m_logHandlerLevel;
		std::function<void(const char *)> m_startProfiling;
		std::function<void()> m_endProfiling;

		std::queue<std::function<void(prosper::IPrimaryCommandBuffer &)>> m_scheduledBufferUpdates;

		WindowSettings m_initialWindowSettings {};

		std::thread::id m_mainThreadId;
		bool m_useReservedDeviceLocalImageBuffer = true;
		std::shared_ptr<prosper::Texture> m_dummyTexture = nullptr;
		std::shared_ptr<prosper::Texture> m_dummyCubemapTexture = nullptr;
		std::shared_ptr<prosper::IBuffer> m_dummyBuffer = nullptr;

		struct ValidationData {
			std::unordered_map<prosper::IImage *, std::chrono::steady_clock::time_point> lastImageUsage;
			std::unordered_map<prosper::IBuffer *, std::chrono::steady_clock::time_point> lastBufferUsage;
			std::mutex mutex;
		};
		std::unique_ptr<ValidationData> m_validationData = nullptr;
	  private:
		mutable FrameIndex m_frameId = 0ull;
		mutable CommonBufferCache m_commonBufferCache;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::IPrContext::StateFlags)
#pragma warning(pop)

template<typename T>
bool prosper::IPrContext::ScheduleRecordUpdateBuffer(const std::shared_ptr<IBuffer> &buffer, uint64_t offset, const T &data, const BufferUpdateInfo &updateInfo)
{
	return ScheduleRecordUpdateBuffer(buffer, offset, sizeof(data), &data, updateInfo);
}

template<class TContext>
std::shared_ptr<TContext> prosper::IPrContext::Create(const std::string &appName, uint32_t width, uint32_t height, bool bEnableValidation)
{
	auto r = std::shared_ptr<TContext>(new TContext(appName, bEnableValidation));

	CreateInfo createInfo {};
	createInfo.width = width;
	createInfo.height = height;
	r->Initialize(createInfo);
	return r;
}

#endif
