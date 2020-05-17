/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_CONTEXT_HPP__
#define __PR_PROSPER_VK_CONTEXT_HPP__

#include "prosper_context.hpp"

namespace Anvil
{
	class BaseDevice;
	class SGPUDevice;
	class RenderPass;
	class Swapchain;
	class Instance;
	struct SurfaceCapabilities;
	struct MipmapRawData;
	class RenderPassCreateInfo;
	class DescriptorSetCreateInfo;
	class PipelineLayout;
	class PhysicalDevice;
	class Queue;
	class RenderingSurface;
	class Window;
	class Fence;
	class Framebuffer;
	class Semaphore;

	typedef std::unique_ptr<BaseDevice,std::function<void(BaseDevice*)>> BaseDeviceUniquePtr;
	typedef std::unique_ptr<Instance,std::function<void(Instance*)>> InstanceUniquePtr;
	typedef std::unique_ptr<Window,std::function<void(Window*)>> WindowUniquePtr;
	typedef std::unique_ptr<Semaphore,std::function<void(Semaphore*)>> SemaphoreUniquePtr;
};

struct VkSurfaceKHR_T;
namespace prosper
{
	class DLLPROSPER VlkContext
		: public IPrContext
	{
	public:
		static std::shared_ptr<VlkContext> Create(const std::string &appName,bool bEnableValidation);
		static IPrContext &GetContext(Anvil::BaseDevice &dev);

		Anvil::SGPUDevice &GetDevice();
		const std::shared_ptr<Anvil::RenderPass> &GetMainRenderPass() const;
		SubPassID GetMainSubPassID() const;
		std::shared_ptr<Anvil::Swapchain> GetSwapchain();

		const Anvil::Instance &GetAnvilInstance() const;
		Anvil::Instance &GetAnvilInstance();

		bool GetSurfaceCapabilities(Anvil::SurfaceCapabilities &caps) const;
		virtual void ReloadWindow() override;
		virtual Vendor GetPhysicalDeviceVendor() const override;
		virtual bool IsPresentationModeSupported(prosper::PresentModeKHR presentMode) const override;
		virtual MemoryRequirements GetMemoryRequirements(IImage &img) override;
		virtual DeviceSize GetBufferAlignment(BufferUsageFlags usageFlags) override;

		using IPrContext::CreateImage;
		std::shared_ptr<IImage> CreateImage(const util::ImageCreateInfo &createInfo,const std::vector<Anvil::MipmapRawData> &data);
		using IPrContext::CreateRenderPass;
		std::shared_ptr<IRenderPass> CreateRenderPass(const prosper::util::RenderPassCreateInfo &renderPassInfo,std::unique_ptr<Anvil::RenderPassCreateInfo> anvRenderPassInfo);
		using IPrContext::CreateDescriptorSetGroup;
		std::shared_ptr<IDescriptorSetGroup> CreateDescriptorSetGroup(const DescriptorSetCreateInfo &descSetCreateInfo,std::unique_ptr<Anvil::DescriptorSetCreateInfo> descSetInfo);

		virtual bool IsImageFormatSupported(
			prosper::Format format,prosper::ImageUsageFlags usageFlags,prosper::ImageType type=prosper::ImageType::e2D,
			prosper::ImageTiling tiling=prosper::ImageTiling::Optimal
		) const override;

		virtual std::shared_ptr<prosper::IPrimaryCommandBuffer> AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex) override;
		virtual std::shared_ptr<prosper::ISecondaryCommandBuffer> AllocateSecondaryLevelCommandBuffer(prosper::QueueFamilyType queueFamilyType,uint32_t &universalQueueFamilyIndex) override;
		virtual bool SavePipelineCache() override;

		virtual Result WaitForFence(const IFence &fence,uint64_t timeout=std::numeric_limits<uint64_t>::max()) const override;
		virtual Result WaitForFences(const std::vector<IFence*> &fences,bool waitAll=true,uint64_t timeout=std::numeric_limits<uint64_t>::max()) const override;
		virtual void DrawFrame(const std::function<void(const std::shared_ptr<prosper::IPrimaryCommandBuffer>&,uint32_t)> &drawFrame) override;
		virtual bool Submit(ICommandBuffer &cmdBuf,bool shouldBlock=false,IFence *optFence=nullptr) override;
		virtual void SubmitCommandBuffer(prosper::ICommandBuffer &cmd,prosper::QueueFamilyType queueFamilyType,bool shouldBlock=false,prosper::IFence *fence=nullptr) override;
		using IPrContext::SubmitCommandBuffer;

		Anvil::PipelineLayout *GetPipelineLayout(bool graphicsShader,PipelineID pipelineId);
	protected:
		VlkContext(const std::string &appName,bool bEnableValidation=false);
		virtual void Release() override;

		virtual void DoKeepResourceAliveUntilPresentationComplete(const std::shared_ptr<void> &resource) override;
		virtual void DoWaitIdle() override;
		virtual void DoFlushSetupCommandBuffer() override;
		void InitCommandBuffers();
		void InitFrameBuffers();
		void InitSemaphores();
		void InitSwapchain();
		void InitWindow();
		void InitVulkan(const CreateInfo &createInfo);
		void InitMainRenderPass();
		virtual void ReloadSwapchain() override;
		virtual void InitAPI(const CreateInfo &createInfo) override;

		Anvil::BaseDeviceUniquePtr m_devicePtr = nullptr;
		Anvil::SGPUDevice *m_pGpuDevice = nullptr;
		Anvil::InstanceUniquePtr m_instancePtr = nullptr;
		const Anvil::PhysicalDevice *m_physicalDevicePtr = nullptr;
		Anvil::Queue *m_presentQueuePtr = nullptr;
		std::shared_ptr<Anvil::RenderingSurface> m_renderingSurfacePtr;
		std::shared_ptr<Anvil::Swapchain> m_swapchainPtr;
		Anvil::WindowUniquePtr m_windowPtr = nullptr;
		std::shared_ptr<Anvil::RenderPass> m_renderPass;
		SubPassID m_mainSubPass = std::numeric_limits<SubPassID>::max();
		VkSurfaceKHR_T *m_surface = nullptr;

		std::vector<std::shared_ptr<Anvil::Fence>> m_cmdFences;
		std::vector<std::shared_ptr<Anvil::Framebuffer>> m_fbos;

		std::vector<Anvil::SemaphoreUniquePtr> m_frameSignalSemaphores;
		std::vector<Anvil::SemaphoreUniquePtr> m_frameWaitSemaphores;
	private:
		void ReleaseSwapchain();
		bool GetUniversalQueueFamilyIndex(prosper::QueueFamilyType queueFamilyType,uint32_t &queueFamilyIndex) const;
		static std::unordered_map<Anvil::BaseDevice*,IPrContext*> s_devToContext;
	};
};

#endif
