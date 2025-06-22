/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_context.hpp"
#include "prosper_util.hpp"
#include "prosper_util_image_buffer.hpp"
#include "image/prosper_render_target.hpp"
#include "buffers/prosper_buffer.hpp"
#include "prosper_descriptor_set_group.hpp"
#include "image/prosper_sampler.hpp"
#include "image/prosper_msaa_texture.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"
#include "shader/prosper_shader.hpp"
#include "image/prosper_image_view.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "prosper_swap_command_buffer.hpp"
#include "prosper_memory_tracker.hpp"
#include "prosper_util_gli.hpp"
#include <fsys/filesystem.h>
#include <sharedutils/util.h>
#include <sharedutils/magic_enum.hpp>
#include <util_image_buffer.hpp>
#include <util_image.hpp>
#include <util_texture_info.hpp>
#include <sstream>

#ifdef DEBUG_VERBOSE
#include <iostream>
#endif

bool prosper::util::RenderPassCreateInfo::AttachmentInfo::operator==(const AttachmentInfo &other) const
{
	return format == other.format && sampleCount == other.sampleCount && loadOp == other.loadOp && storeOp == other.storeOp && initialLayout == other.initialLayout && finalLayout == other.finalLayout;
}
bool prosper::util::RenderPassCreateInfo::AttachmentInfo::operator!=(const AttachmentInfo &other) const { return !operator==(other); }

/////////////////

bool prosper::util::RenderPassCreateInfo::SubPass::operator==(const SubPass &other) const { return colorAttachments == other.colorAttachments && useDepthStencilAttachment == other.useDepthStencilAttachment && dependencies == other.dependencies; }
bool prosper::util::RenderPassCreateInfo::SubPass::operator!=(const SubPass &other) const { return !operator==(other); }

/////////////////

bool prosper::util::RenderPassCreateInfo::SubPass::Dependency::operator==(const Dependency &other) const
{
	return sourceSubPassId == other.sourceSubPassId && destinationSubPassId == other.destinationSubPassId && sourceStageMask == other.sourceStageMask && destinationStageMask == other.destinationStageMask && sourceAccessMask == other.sourceAccessMask
	  && destinationAccessMask == other.destinationAccessMask;
}
bool prosper::util::RenderPassCreateInfo::SubPass::Dependency::operator!=(const Dependency &other) const { return !operator==(other); }

/////////////////

prosper::util::RenderPassCreateInfo::AttachmentInfo::AttachmentInfo(prosper::Format format, prosper::ImageLayout initialLayout, prosper::AttachmentLoadOp loadOp, prosper::AttachmentStoreOp storeOp, prosper::SampleCountFlags sampleCount, prosper::ImageLayout finalLayout,
  prosper::AttachmentLoadOp stencilLoadOp, prosper::AttachmentStoreOp stencilStoreOp)
    : format {format}, initialLayout {initialLayout}, loadOp {loadOp}, storeOp {storeOp}, sampleCount {sampleCount}, finalLayout {finalLayout}, stencilLoadOp {stencilLoadOp}, stencilStoreOp {stencilStoreOp}
{
}
bool prosper::util::RenderPassCreateInfo::operator==(const RenderPassCreateInfo &other) const { return attachments == other.attachments && subPasses == other.subPasses; }
bool prosper::util::RenderPassCreateInfo::operator!=(const RenderPassCreateInfo &other) const { return !operator==(other); }

prosper::util::RenderPassCreateInfo::SubPass::SubPass(const std::vector<std::size_t> &colorAttachments, bool useDepthStencilAttachment, std::vector<Dependency> dependencies)
    : colorAttachments {colorAttachments}, useDepthStencilAttachment {useDepthStencilAttachment}, dependencies {dependencies}
{
}
prosper::util::RenderPassCreateInfo::SubPass::Dependency::Dependency(std::size_t sourceSubPassId, std::size_t destinationSubPassId, prosper::PipelineStageFlags sourceStageMask, prosper::PipelineStageFlags destinationStageMask, prosper::AccessFlags sourceAccessMask,
  prosper::AccessFlags destinationAccessMask)
    : sourceSubPassId {sourceSubPassId}, destinationSubPassId {destinationSubPassId}, sourceStageMask {sourceStageMask}, destinationStageMask {destinationStageMask}, sourceAccessMask {sourceAccessMask}, destinationAccessMask {destinationAccessMask}
{
}
prosper::util::RenderPassCreateInfo::RenderPassCreateInfo(const std::vector<AttachmentInfo> &attachments, const std::vector<SubPass> &subPasses) : attachments {attachments}, subPasses {subPasses} {}

std::shared_ptr<prosper::IImageView> prosper::IPrContext::CreateImageView(const util::ImageViewCreateInfo &createInfo, IImage &img)
{
	auto format = createInfo.format;
	if(format == Format::Unknown)
		format = img.GetFormat();

	auto aspectMask = createInfo.aspectFlags.has_value() ? *createInfo.aspectFlags : util::get_aspect_mask(format);
	auto imgType = img.GetType();
	auto numLayers = createInfo.levelCount;
	auto type = ImageViewType::e2D;
	if(numLayers > 1u && img.IsCubemap())
		type = (numLayers > 6u) ? ImageViewType::CubeArray : ImageViewType::Cube;
	else {
		switch(imgType) {
		case ImageType::e1D:
			type = (numLayers > 1u) ? ImageViewType::e1DArray : ImageViewType::e1D;
			break;
		case ImageType::e2D:
			type = (numLayers > 1u) ? ImageViewType::e2DArray : ImageViewType::e2D;
			break;
		case ImageType::e3D:
			type = ImageViewType::e3D;
			break;
		}
	}
	return DoCreateImageView(createInfo, img, format, type, aspectMask, numLayers);
}

std::shared_ptr<prosper::IImage> prosper::IPrContext::CreateImage(const util::ImageCreateInfo &createInfo, const ImageData &imgData)
{
	return CreateImage(createInfo, [&imgData](uint32_t layer, uint32_t mipmap, uint32_t &dataSize, uint32_t &rotSize) -> const uint8_t * {
		if(layer >= imgData.size())
			return nullptr;
		auto &layerData = imgData[layer];
		if(mipmap >= layerData.size())
			return nullptr;
		return layerData[mipmap];
	});
}
std::shared_ptr<prosper::IImage> prosper::IPrContext::CreateImage(uimg::ImageBuffer &imgBuffer, const std::optional<util::ImageCreateInfo> &optCreateInfo)
{
	auto createInfo = optCreateInfo.has_value() ? *optCreateInfo : util::get_image_create_info(imgBuffer, false);
	return CreateImage(createInfo, static_cast<uint8_t *>(imgBuffer.GetData()));
}
std::shared_ptr<prosper::IImage> prosper::IPrContext::CreateImage(const util::ImageCreateInfo &createInfo, const uint8_t *data) { return CreateImage(createInfo, ImageData {ImageLayerData {data}}); }
std::shared_ptr<prosper::IImage> prosper::IPrContext::CreateImage(const std::vector<std::shared_ptr<uimg::ImageBuffer>> &imgBuffer, const std::optional<util::ImageCreateInfo> &optCreateInfo)
{
	auto createInfo = optCreateInfo.has_value() ? *optCreateInfo : util::get_image_create_info(*imgBuffer.front(), false);
	ImageData imgData {};
	imgData.reserve(imgBuffer.size());
	for(auto &imgBuf : imgBuffer)
		imgData.push_back(ImageLayerData {static_cast<const uint8_t *>(imgBuf->GetData())});
	return CreateImage(createInfo, imgData);
}
prosper::Format prosper::util::get_prosper_format(const uimg::ImageBuffer &imgBuffer)
{
	auto format = imgBuffer.GetFormat();
	prosper::Format prosperFormat = prosper::Format::Unknown;
	switch(format) {
	case uimg::Format::R8:
		prosperFormat = prosper::Format::R8_UNorm;
		break;
	case uimg::Format::R16:
		prosperFormat = prosper::Format::R16_SFloat;
		break;
	case uimg::Format::R32:
		prosperFormat = prosper::Format::R32_SFloat;
		break;

	case uimg::Format::RG8:
		prosperFormat = prosper::Format::R8G8_UNorm;
		break;
	case uimg::Format::RG16:
		prosperFormat = prosper::Format::R16G16_SFloat;
		break;
	case uimg::Format::RG32:
		prosperFormat = prosper::Format::R32G32_SFloat;
		break;

	case uimg::Format::RGB8:
		prosperFormat = prosper::Format::R8G8B8_UNorm_PoorCoverage;
		break;
	case uimg::Format::RGB16:
		prosperFormat = prosper::Format::R16G16B16_SFloat_PoorCoverage;
		break;
	case uimg::Format::RGB32:
		prosperFormat = prosper::Format::R32G32B32_SFloat;
		break;

	case uimg::Format::RGBA8:
		prosperFormat = prosper::Format::R8G8B8A8_UNorm;
		break;
	case uimg::Format::RGBA16:
		prosperFormat = prosper::Format::R16G16B16A16_SFloat;
		break;
	case uimg::Format::RGBA32:
		prosperFormat = prosper::Format::R32G32B32A32_SFloat;
		break;
	}
	return prosperFormat;
}
prosper::util::ImageCreateInfo prosper::util::get_image_create_info(const uimg::ImageBuffer &imgBuffer, bool cubemap)
{
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = get_prosper_format(imgBuffer);
	imgCreateInfo.width = imgBuffer.GetWidth();
	imgCreateInfo.height = imgBuffer.GetHeight();
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ShaderReadOnlyOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
	imgCreateInfo.layers = cubemap ? 6 : 1;
	if(cubemap)
		imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;
	return imgCreateInfo;
}
std::shared_ptr<prosper::IImage> prosper::IPrContext::CreateCubemap(std::array<std::shared_ptr<uimg::ImageBuffer>, 6> &imgBuffers, const std::optional<util::ImageCreateInfo> &createInfo)
{
	auto imgCreateInfo = createInfo.has_value() ? util::get_image_create_info(*imgBuffers.front(), true) : *createInfo;
	ImageData imgData {};
	imgData.reserve(imgBuffers.size());
	for(auto &imgBuf : imgBuffers)
		imgData.push_back({static_cast<uint8_t *>(imgBuf->GetData())});
	return CreateImage(imgCreateInfo, imgData);
}
std::shared_ptr<prosper::IDescriptorSetGroup> prosper::IPrContext::CreateDescriptorSetGroup(const DescriptorSetInfo &descSetInfo)
{
	auto descSetCreateInfo = descSetInfo.ToProsperDescriptorSetInfo();
	return CreateDescriptorSetGroup(*descSetCreateInfo);
	//return static_cast<VlkContext*>(this)->CreateDescriptorSetGroup(std::move(descSetInfo.ToAnvilDescriptorSetInfo()));
}
std::shared_ptr<prosper::Texture> prosper::IPrContext::CreateTexture(const util::TextureCreateInfo &createInfo, IImage &img, const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo, const std::optional<util::SamplerCreateInfo> &samplerCreateInfo)
{
	auto imageView = createInfo.imageView;
	auto sampler = createInfo.sampler;

	if(imageView == nullptr && imageViewCreateInfo.has_value()) {
		auto originalLevelCount = const_cast<util::ImageViewCreateInfo &>(*imageViewCreateInfo).levelCount;
		if(img.IsCubemap() && imageViewCreateInfo->baseLayer.has_value() == false)
			const_cast<util::ImageViewCreateInfo &>(*imageViewCreateInfo).levelCount = 6u; // TODO: Do this properly
		imageView = CreateImageView(*imageViewCreateInfo, img);
		const_cast<util::ImageViewCreateInfo &>(*imageViewCreateInfo).levelCount = originalLevelCount;
	}

	std::vector<std::shared_ptr<IImageView>> imageViews = {imageView};
	if((createInfo.flags & util::TextureCreateInfo::Flags::CreateImageViewForEachLayer) != util::TextureCreateInfo::Flags::None) {
		if(imageViewCreateInfo.has_value() == false)
			throw std::invalid_argument("If 'CreateImageViewForEachLayer' flag is set, a valid ImageViewCreateInfo struct has to be supplied!");
		auto numLayers = img.GetLayerCount();
		imageViews.reserve(imageViews.size() + numLayers);
		for(auto i = decltype(numLayers) {0}; i < numLayers; ++i) {
			auto imgViewCreateInfoLayer = *imageViewCreateInfo;
			imgViewCreateInfoLayer.baseLayer = i;
			imageViews.push_back(CreateImageView(imgViewCreateInfoLayer, img));
		}
	}

	if(sampler == nullptr && samplerCreateInfo.has_value())
		sampler = CreateSampler(*samplerCreateInfo);
	if(img.GetSampleCount() != SampleCountFlags::e1Bit && (createInfo.flags & util::TextureCreateInfo::Flags::Resolvable) != util::TextureCreateInfo::Flags::None) {
		if(util::is_depth_format(img.GetFormat()))
			throw std::logic_error("Cannot create resolvable multi-sampled depth image: Multi-sample depth images cannot be resolved!");
		auto extents = img.GetExtents();
		prosper::util::ImageCreateInfo imgCreateInfo {};
		imgCreateInfo.format = img.GetFormat();
		imgCreateInfo.width = extents.width;
		imgCreateInfo.height = extents.height;
		imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
		imgCreateInfo.type = img.GetType();
		imgCreateInfo.usage = img.GetUsageFlags() | ImageUsageFlags::TransferDstBit;
		imgCreateInfo.tiling = img.GetTiling();
		imgCreateInfo.layers = img.GetLayerCount();
		imgCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
		auto resolvedImg = CreateImage(imgCreateInfo);
		auto resolvedTexture = CreateTexture({}, *resolvedImg, imageViewCreateInfo, samplerCreateInfo);
		return std::shared_ptr<MSAATexture> {new MSAATexture {*this, img, imageViews, sampler.get(), resolvedTexture}, [](MSAATexture *msaaTex) {
			                                     msaaTex->OnRelease();
			                                     delete msaaTex;
		                                     }};
	}
	return std::shared_ptr<Texture>(new Texture {*this, img, imageViews, sampler.get()}, [](Texture *tex) {
		tex->OnRelease();
		delete tex;
	});
}

std::shared_ptr<prosper::RenderTarget> prosper::IPrContext::CreateRenderTarget(const std::vector<std::shared_ptr<Texture>> &textures, const std::shared_ptr<IRenderPass> &rp, const util::RenderTargetCreateInfo &rtCreateInfo)
{
	if(textures.empty())
		return nullptr;
	auto &tex = textures.front();
	if(tex == nullptr)
		return nullptr;
	auto &context = tex->GetContext();
	auto &img = tex->GetImage();
	auto extents = img.GetExtents();
	auto numLayers = img.GetLayerCount();
	std::vector<std::shared_ptr<IFramebuffer>> framebuffers;
	std::vector<prosper::IImageView *> attachments;
	attachments.reserve(textures.size());
	for(auto &tex : textures)
		attachments.push_back(tex->GetImageView());
	framebuffers.push_back(context.CreateFramebuffer(extents.width, extents.height, 1u, attachments));
	if(rtCreateInfo.useLayerFramebuffers == true) {
		framebuffers.reserve(framebuffers.size() + numLayers);
		for(auto i = decltype(numLayers) {0}; i < numLayers; ++i) {
			attachments = {tex->GetImageView(i)};
			framebuffers.push_back(context.CreateFramebuffer(extents.width, extents.height, 1u, attachments));
		}
	}
	return std::shared_ptr<RenderTarget> {new RenderTarget {context, textures, framebuffers, *rp}, [](RenderTarget *rt) {
		                                      rt->OnRelease();
		                                      delete rt;
	                                      }};
}
std::shared_ptr<prosper::RenderTarget> prosper::IPrContext::CreateRenderTarget(Texture &texture, IImageView &imgView, IRenderPass &rp, const util::RenderTargetCreateInfo &rtCreateInfo)
{
	auto &img = imgView.GetImage();
	auto &context = rp.GetContext();
	auto extents = img.GetExtents();
	std::vector<prosper::IImageView *> attachments = {&imgView};
	std::vector<std::shared_ptr<IFramebuffer>> framebuffers;
	framebuffers.push_back(context.CreateFramebuffer(extents.width, extents.height, 1u, attachments));
	return std::shared_ptr<RenderTarget> {new RenderTarget {context, std::vector<std::shared_ptr<Texture>> {texture.shared_from_this()}, framebuffers, rp}, [](RenderTarget *rt) {
		                                      rt->OnRelease();
		                                      delete rt;
	                                      }};
}

prosper::Format prosper::util::get_vk_format(uimg::Format format)
{
	switch(format) {
	case uimg::Format::R8:
		return prosper::Format::R8_UNorm;
	case uimg::Format::RG8:
		return prosper::Format::R8G8_UNorm;
	case uimg::Format::RGB8:
		return prosper::Format::R8G8B8_UNorm_PoorCoverage;
	case uimg::Format::RGBA8:
		return prosper::Format::R8G8B8A8_UNorm;
	case uimg::Format::R16:
		return prosper::Format::R16_SFloat;
	case uimg::Format::RG16:
		return prosper::Format::R16G16_SFloat;
	case uimg::Format::RGB16:
		return prosper::Format::R16G16B16_SFloat_PoorCoverage;
	case uimg::Format::RGBA16:
		return prosper::Format::R16G16B16A16_SFloat;
	case uimg::Format::R32:
		return prosper::Format::R32_SFloat;
	case uimg::Format::RG32:
		return prosper::Format::R32G32_SFloat;
	case uimg::Format::RGB32:
		return prosper::Format::R32G32B32_SFloat;
	case uimg::Format::RGBA32:
		return prosper::Format::R32G32B32A32_SFloat;
	}
	static_assert(umath::to_integral(uimg::Format::Count) == 13);
	return prosper::Format::Unknown;
}

void prosper::util::calculate_mipmap_size(uint32_t w, uint32_t h, uint32_t *wMipmap, uint32_t *hMipmap, uint32_t level)
{
	*wMipmap = w;
	*hMipmap = h;
	int scale = static_cast<int>(std::pow(2, level));
	*wMipmap /= scale;
	*hMipmap /= scale;
	if(*wMipmap == 0)
		*wMipmap = 1;
	if(*hMipmap == 0)
		*hMipmap = 1;
}

uint32_t prosper::util::calculate_mipmap_size(uint32_t v, uint32_t level)
{
	auto r = v;
	auto scale = static_cast<int>(std::pow(2, level));
	r /= scale;
	if(r == 0)
		r = 1;
	return r;
}

uint32_t prosper::util::calculate_mipmap_count(uint32_t w, uint32_t h) { return 1 + static_cast<uint32_t>(floor(log2(fmaxf(static_cast<float>(w), static_cast<float>(h))))); }

static std::string to_string(prosper::Result value)
{
	switch(value) {
	case prosper::Result::Success:
		return "Success";
	case prosper::Result::NotReady:
		return "NotReady";
	case prosper::Result::Timeout:
		return "Timeout";
	case prosper::Result::EventSet:
		return "EventSet";
	case prosper::Result::EventReset:
		return "EventReset";
	case prosper::Result::Incomplete:
		return "Incomplete";
	case prosper::Result::ErrorOutOfHostMemory:
		return "ErrorOutOfHostMemory";
	case prosper::Result::ErrorOutOfDeviceMemory:
		return "ErrorOutOfDeviceMemory";
	case prosper::Result::ErrorInitializationFailed:
		return "ErrorInitializationFailed";
	case prosper::Result::ErrorDeviceLost:
		return "ErrorDeviceLost";
	case prosper::Result::ErrorMemoryMapFailed:
		return "ErrorMemoryMapFailed";
	case prosper::Result::ErrorLayerNotPresent:
		return "ErrorLayerNotPresent";
	case prosper::Result::ErrorExtensionNotPresent:
		return "ErrorExtensionNotPresent";
	case prosper::Result::ErrorFeatureNotPresent:
		return "ErrorFeatureNotPresent";
	case prosper::Result::ErrorIncompatibleDriver:
		return "ErrorIncompatibleDriver";
	case prosper::Result::ErrorTooManyObjects:
		return "ErrorTooManyObjects";
	case prosper::Result::ErrorFormatNotSupported:
		return "ErrorFormatNotSupported";
	case prosper::Result::ErrorFragmentedPool:
		return "ErrorFragmentedPool";
	case prosper::Result::ErrorOutOfPoolMemory:
		return "ErrorOutOfPoolMemory";
	case prosper::Result::ErrorInvalidExternalHandle:
		return "ErrorInvalidExternalHandle";
	case prosper::Result::ErrorSurfaceLostKHR:
		return "ErrorSurfaceLostKHR";
	case prosper::Result::ErrorNativeWindowInUseKHR:
		return "ErrorNativeWindowInUseKHR";
	case prosper::Result::SuboptimalKHR:
		return "SuboptimalKHR";
	case prosper::Result::ErrorOutOfDateKHR:
		return "ErrorOutOfDateKHR";
	case prosper::Result::ErrorIncompatibleDisplayKHR:
		return "ErrorIncompatibleDisplayKHR";
	case prosper::Result::ErrorValidationFailedEXT:
		return "ErrorValidationFailedEXT";
	case prosper::Result::ErrorInvalidShaderNV:
		return "ErrorInvalidShaderNV";
	case prosper::Result::ErrorInvalidDrmFormatModifierPlaneLayoutEXT:
		return "ErrorInvalidDrmFormatModifierPlaneLayoutEXT";
	case prosper::Result::ErrorFragmentationEXT:
		return "ErrorFragmentationEXT";
	case prosper::Result::ErrorNotPermittedEXT:
		return "ErrorNotPermittedEXT";
	case prosper::Result::ErrorInvalidDeviceAddressEXT:
		return "ErrorInvalidDeviceAddressEXT";
	case prosper::Result::ErrorFullScreenExclusiveModeLostEXT:
		return "ErrorFullScreenExclusiveModeLostEXT";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(Result r) { return ::to_string(r); }

static std::string to_string(prosper::DebugReportFlags value)
{
	switch(value) {
	case prosper::DebugReportFlags::InformationBit:
		return "Information";
	case prosper::DebugReportFlags::WarningBit:
		return "Warning";
	case prosper::DebugReportFlags::PerformanceWarningBit:
		return "PerformanceWarning";
	case prosper::DebugReportFlags::ErrorBit:
		return "Error";
	case prosper::DebugReportFlags::DebugBit:
		return "Debug";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(DebugReportFlags flags)
{
	auto values = umath::get_power_of_2_values(static_cast<uint32_t>(flags));
	std::string r;
	for(auto it = values.begin(); it != values.end(); ++it) {
		if(it != values.begin())
			r += " | ";
		r += ::to_string(static_cast<prosper::DebugReportFlags>(*it));
	}
	return r;
}
static std::string to_string(prosper::DebugReportObjectTypeEXT value)
{
	switch(value) {
	case prosper::DebugReportObjectTypeEXT::Unknown:
		return "Unknown";
	case prosper::DebugReportObjectTypeEXT::Instance:
		return "Instance";
	case prosper::DebugReportObjectTypeEXT::PhysicalDevice:
		return "PhysicalDevice";
	case prosper::DebugReportObjectTypeEXT::Device:
		return "Device";
	case prosper::DebugReportObjectTypeEXT::Queue:
		return "Queue";
	case prosper::DebugReportObjectTypeEXT::Semaphore:
		return "Semaphore";
	case prosper::DebugReportObjectTypeEXT::CommandBuffer:
		return "CommandBuffer";
	case prosper::DebugReportObjectTypeEXT::Fence:
		return "Fence";
	case prosper::DebugReportObjectTypeEXT::DeviceMemory:
		return "DeviceMemory";
	case prosper::DebugReportObjectTypeEXT::Buffer:
		return "Buffer";
	case prosper::DebugReportObjectTypeEXT::Image:
		return "Image";
	case prosper::DebugReportObjectTypeEXT::Event:
		return "Event";
	case prosper::DebugReportObjectTypeEXT::QueryPool:
		return "QueryPool";
	case prosper::DebugReportObjectTypeEXT::BufferView:
		return "BufferView";
	case prosper::DebugReportObjectTypeEXT::ImageView:
		return "ImageView";
	case prosper::DebugReportObjectTypeEXT::ShaderModule:
		return "ShaderModule";
	case prosper::DebugReportObjectTypeEXT::PipelineCache:
		return "PipelineCache";
	case prosper::DebugReportObjectTypeEXT::PipelineLayout:
		return "PipelineLayout";
	case prosper::DebugReportObjectTypeEXT::RenderPass:
		return "RenderPass";
	case prosper::DebugReportObjectTypeEXT::Pipeline:
		return "Pipeline";
	case prosper::DebugReportObjectTypeEXT::DescriptorSetLayout:
		return "DescriptorSetLayout";
	case prosper::DebugReportObjectTypeEXT::Sampler:
		return "Sampler";
	case prosper::DebugReportObjectTypeEXT::DescriptorPool:
		return "DescriptorPool";
	case prosper::DebugReportObjectTypeEXT::DescriptorSet:
		return "DescriptorSet";
	case prosper::DebugReportObjectTypeEXT::Framebuffer:
		return "Framebuffer";
	case prosper::DebugReportObjectTypeEXT::CommandPool:
		return "CommandPool";
	case prosper::DebugReportObjectTypeEXT::SurfaceKHR:
		return "SurfaceKHR";
	case prosper::DebugReportObjectTypeEXT::SwapchainKHR:
		return "SwapchainKHR";
	case prosper::DebugReportObjectTypeEXT::DebugReportCallbackEXT:
		return "DebugReportCallbackEXT";
	case prosper::DebugReportObjectTypeEXT::DisplayKHR:
		return "DisplayKHR";
	case prosper::DebugReportObjectTypeEXT::DisplayModeKHR:
		return "DisplayModeKHR";
	case prosper::DebugReportObjectTypeEXT::ObjectTableNVX:
		return "ObjectTableNVX";
	case prosper::DebugReportObjectTypeEXT::IndirectCommandsLayoutNVX:
		return "IndirectCommandsLayoutNVX";
	case prosper::DebugReportObjectTypeEXT::ValidationCacheEXT:
		return "ValidationCacheEXT";
	case prosper::DebugReportObjectTypeEXT::SamplerYcbcrConversion:
		return "SamplerYcbcrConversion";
	case prosper::DebugReportObjectTypeEXT::DescriptorUpdateTemplate:
		return "DescriptorUpdateTemplate";
	case prosper::DebugReportObjectTypeEXT::AccelerationStructureNV:
		return "AccelerationStructureNV";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::DebugReportObjectTypeEXT type) { return ::to_string(type); }
static std::string to_string(prosper::Format value)
{
	using namespace prosper;
	switch(value) {
	case Format::Unknown:
		return "Unknown";
	case Format::R4G4_UNorm_Pack8:
		return "R4G4_UNorm_Pack8";
	case Format::R4G4B4A4_UNorm_Pack16:
		return "R4G4B4A4_UNorm_Pack16";
	case Format::B4G4R4A4_UNorm_Pack16:
		return "B4G4R4A4_UNorm_Pack16";
	case Format::R5G6B5_UNorm_Pack16:
		return "R5G6B5_UNorm_Pack16";
	case Format::B5G6R5_UNorm_Pack16:
		return "B5G6R5_UNorm_Pack16";
	case Format::R5G5B5A1_UNorm_Pack16:
		return "R5G5B5A1_UNorm_Pack16";
	case Format::B5G5R5A1_UNorm_Pack16:
		return "B5G5R5A1_UNorm_Pack16";
	case Format::A1R5G5B5_UNorm_Pack16:
		return "A1R5G5B5_UNorm_Pack16";
	case Format::R8_UNorm:
		return "R8_UNorm";
	case Format::R8_SNorm:
		return "R8_SNorm";
	case Format::R8_UScaled_PoorCoverage:
		return "R8_UScaled_PoorCoverage";
	case Format::R8_SScaled_PoorCoverage:
		return "R8_SScaled_PoorCoverage";
	case Format::R8_UInt:
		return "R8_UInt";
	case Format::R8_SInt:
		return "R8_SInt";
	case Format::R8_SRGB:
		return "R8_SRGB";
	case Format::R8G8_UNorm:
		return "R8G8_UNorm";
	case Format::R8G8_SNorm:
		return "R8G8_SNorm";
	case Format::R8G8_UScaled_PoorCoverage:
		return "R8G8_UScaled_PoorCoverage";
	case Format::R8G8_SScaled_PoorCoverage:
		return "R8G8_SScaled_PoorCoverage";
	case Format::R8G8_UInt:
		return "R8G8_UInt";
	case Format::R8G8_SInt:
		return "R8G8_SInt";
	case Format::R8G8_SRGB_PoorCoverage:
		return "R8G8_SRGB_PoorCoverage";
	case Format::R8G8B8_UNorm_PoorCoverage:
		return "R8G8B8_UNorm_PoorCoverage";
	case Format::R8G8B8_SNorm_PoorCoverage:
		return "R8G8B8_SNorm_PoorCoverage";
	case Format::R8G8B8_UScaled_PoorCoverage:
		return "R8G8B8_UScaled_PoorCoverage";
	case Format::R8G8B8_SScaled_PoorCoverage:
		return "R8G8B8_SScaled_PoorCoverage";
	case Format::R8G8B8_UInt_PoorCoverage:
		return "R8G8B8_UInt_PoorCoverage";
	case Format::R8G8B8_SInt_PoorCoverage:
		return "R8G8B8_SInt_PoorCoverage";
	case Format::R8G8B8_SRGB_PoorCoverage:
		return "R8G8B8_SRGB_PoorCoverage";
	case Format::B8G8R8_UNorm_PoorCoverage:
		return "B8G8R8_UNorm_PoorCoverage";
	case Format::B8G8R8_SNorm_PoorCoverage:
		return "B8G8R8_SNorm_PoorCoverage";
	case Format::B8G8R8_UScaled_PoorCoverage:
		return "B8G8R8_UScaled_PoorCoverage";
	case Format::B8G8R8_SScaled_PoorCoverage:
		return "B8G8R8_SScaled_PoorCoverage";
	case Format::B8G8R8_UInt_PoorCoverage:
		return "B8G8R8_UInt_PoorCoverage";
	case Format::B8G8R8_SInt_PoorCoverage:
		return "B8G8R8_SInt_PoorCoverage";
	case Format::B8G8R8_SRGB_PoorCoverage:
		return "B8G8R8_SRGB_PoorCoverage";
	case Format::R8G8B8A8_UNorm:
		return "R8G8B8A8_UNorm";
	case Format::R8G8B8A8_SNorm:
		return "R8G8B8A8_SNorm";
	case Format::R8G8B8A8_UScaled_PoorCoverage:
		return "R8G8B8A8_UScaled_PoorCoverage";
	case Format::R8G8B8A8_SScaled_PoorCoverage:
		return "R8G8B8A8_SScaled_PoorCoverage";
	case Format::R8G8B8A8_UInt:
		return "R8G8B8A8_UInt";
	case Format::R8G8B8A8_SInt:
		return "R8G8B8A8_SInt";
	case Format::R8G8B8A8_SRGB:
		return "R8G8B8A8_SRGB";
	case Format::B8G8R8A8_UNorm:
		return "B8G8R8A8_UNorm";
	case Format::B8G8R8A8_SNorm:
		return "B8G8R8A8_SNorm";
	case Format::B8G8R8A8_UScaled_PoorCoverage:
		return "B8G8R8A8_UScaled_PoorCoverage";
	case Format::B8G8R8A8_SScaled_PoorCoverage:
		return "B8G8R8A8_SScaled_PoorCoverage";
	case Format::B8G8R8A8_UInt:
		return "B8G8R8A8_UInt";
	case Format::B8G8R8A8_SInt:
		return "B8G8R8A8_SInt";
	case Format::B8G8R8A8_SRGB:
		return "B8G8R8A8_SRGB";
	case Format::A8B8G8R8_UNorm_Pack32:
		return "A8B8G8R8_UNorm_Pack32";
	case Format::A8B8G8R8_SNorm_Pack32:
		return "A8B8G8R8_SNorm_Pack32";
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
		return "A8B8G8R8_UScaled_Pack32_PoorCoverage";
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
		return "A8B8G8R8_SScaled_Pack32_PoorCoverage";
	case Format::A8B8G8R8_UInt_Pack32:
		return "A8B8G8R8_UInt_Pack32";
	case Format::A8B8G8R8_SInt_Pack32:
		return "A8B8G8R8_SInt_Pack32";
	case Format::A8B8G8R8_SRGB_Pack32:
		return "A8B8G8R8_SRGB_Pack32";
	case Format::A2R10G10B10_UNorm_Pack32:
		return "A2R10G10B10_UNorm_Pack32";
	case Format::A2R10G10B10_SNorm_Pack32_PoorCoverage:
		return "A2R10G10B10_SNorm_Pack32_PoorCoverage";
	case Format::A2R10G10B10_UScaled_Pack32_PoorCoverage:
		return "A2R10G10B10_UScaled_Pack32_PoorCoverage";
	case Format::A2R10G10B10_SScaled_Pack32_PoorCoverage:
		return "A2R10G10B10_SScaled_Pack32_PoorCoverage";
	case Format::A2R10G10B10_UInt_Pack32:
		return "A2R10G10B10_UInt_Pack32";
	case Format::A2R10G10B10_SInt_Pack32_PoorCoverage:
		return "A2R10G10B10_SInt_Pack32_PoorCoverage";
	case Format::A2B10G10R10_UNorm_Pack32:
		return "A2B10G10R10_UNorm_Pack32";
	case Format::A2B10G10R10_SNorm_Pack32_PoorCoverage:
		return "A2B10G10R10_SNorm_Pack32_PoorCoverage";
	case Format::A2B10G10R10_UScaled_Pack32_PoorCoverage:
		return "A2B10G10R10_UScaled_Pack32_PoorCoverage";
	case Format::A2B10G10R10_SScaled_Pack32_PoorCoverage:
		return "A2B10G10R10_SScaled_Pack32_PoorCoverage";
	case Format::A2B10G10R10_UInt_Pack32:
		return "A2B10G10R10_UInt_Pack32";
	case Format::A2B10G10R10_SInt_Pack32_PoorCoverage:
		return "A2B10G10R10_SInt_Pack32_PoorCoverage";
	case Format::R16_UNorm:
		return "R16_UNorm";
	case Format::R16_SNorm:
		return "R16_SNorm";
	case Format::R16_UScaled_PoorCoverage:
		return "R16_UScaled_PoorCoverage";
	case Format::R16_SScaled_PoorCoverage:
		return "R16_SScaled_PoorCoverage";
	case Format::R16_UInt:
		return "R16_UInt";
	case Format::R16_SInt:
		return "R16_SInt";
	case Format::R16_SFloat:
		return "R16_SFloat";
	case Format::R16G16_UNorm:
		return "R16G16_UNorm";
	case Format::R16G16_SNorm:
		return "R16G16_SNorm";
	case Format::R16G16_UScaled_PoorCoverage:
		return "R16G16_UScaled_PoorCoverage";
	case Format::R16G16_SScaled_PoorCoverage:
		return "R16G16_SScaled_PoorCoverage";
	case Format::R16G16_UInt:
		return "R16G16_UInt";
	case Format::R16G16_SInt:
		return "R16G16_SInt";
	case Format::R16G16_SFloat:
		return "R16G16_SFloat";
	case Format::R16G16B16_UNorm_PoorCoverage:
		return "R16G16B16_UNorm_PoorCoverage";
	case Format::R16G16B16_SNorm_PoorCoverage:
		return "R16G16B16_SNorm_PoorCoverage";
	case Format::R16G16B16_UScaled_PoorCoverage:
		return "R16G16B16_UScaled_PoorCoverage";
	case Format::R16G16B16_SScaled_PoorCoverage:
		return "R16G16B16_SScaled_PoorCoverage";
	case Format::R16G16B16_UInt_PoorCoverage:
		return "R16G16B16_UInt_PoorCoverage";
	case Format::R16G16B16_SInt_PoorCoverage:
		return "R16G16B16_SInt_PoorCoverage";
	case Format::R16G16B16_SFloat_PoorCoverage:
		return "R16G16B16_SFloat_PoorCoverage";
	case Format::R16G16B16A16_UNorm:
		return "R16G16B16A16_UNorm";
	case Format::R16G16B16A16_SNorm:
		return "R16G16B16A16_SNorm";
	case Format::R16G16B16A16_UScaled_PoorCoverage:
		return "R16G16B16A16_UScaled_PoorCoverage";
	case Format::R16G16B16A16_SScaled_PoorCoverage:
		return "R16G16B16A16_SScaled_PoorCoverage";
	case Format::R16G16B16A16_UInt:
		return "R16G16B16A16_UInt";
	case Format::R16G16B16A16_SInt:
		return "R16G16B16A16_SInt";
	case Format::R16G16B16A16_SFloat:
		return "R16G16B16A16_SFloat";
	case Format::R32_UInt:
		return "R32_UInt";
	case Format::R32_SInt:
		return "R32_SInt";
	case Format::R32_SFloat:
		return "R32_SFloat";
	case Format::R32G32_UInt:
		return "R32G32_UInt";
	case Format::R32G32_SInt:
		return "R32G32_SInt";
	case Format::R32G32_SFloat:
		return "R32G32_SFloat";
	case Format::R32G32B32_UInt:
		return "R32G32B32_UInt";
	case Format::R32G32B32_SInt:
		return "R32G32B32_SInt";
	case Format::R32G32B32_SFloat:
		return "R32G32B32_SFloat";
	case Format::R32G32B32A32_UInt:
		return "R32G32B32A32_UInt";
	case Format::R32G32B32A32_SInt:
		return "R32G32B32A32_SInt";
	case Format::R32G32B32A32_SFloat:
		return "R32G32B32A32_SFloat";
	case Format::R64_UInt_PoorCoverage:
		return "R64_UInt_PoorCoverage";
	case Format::R64_SInt_PoorCoverage:
		return "R64_SInt_PoorCoverage";
	case Format::R64_SFloat_PoorCoverage:
		return "R64_SFloat_PoorCoverage";
	case Format::R64G64_UInt_PoorCoverage:
		return "R64G64_UInt_PoorCoverage";
	case Format::R64G64_SInt_PoorCoverage:
		return "R64G64_SInt_PoorCoverage";
	case Format::R64G64_SFloat_PoorCoverage:
		return "R64G64_SFloat_PoorCoverage";
	case Format::R64G64B64_UInt_PoorCoverage:
		return "R64G64B64_UInt_PoorCoverage";
	case Format::R64G64B64_SInt_PoorCoverage:
		return "R64G64B64_SInt_PoorCoverage";
	case Format::R64G64B64_SFloat_PoorCoverage:
		return "R64G64B64_SFloat_PoorCoverage";
	case Format::R64G64B64A64_UInt_PoorCoverage:
		return "R64G64B64A64_UInt_PoorCoverage";
	case Format::R64G64B64A64_SInt_PoorCoverage:
		return "R64G64B64A64_SInt_PoorCoverage";
	case Format::R64G64B64A64_SFloat_PoorCoverage:
		return "R64G64B64A64_SFloat_PoorCoverage";
	case Format::B10G11R11_UFloat_Pack32:
		return "B10G11R11_UFloat_Pack32";
	case Format::E5B9G9R9_UFloat_Pack32:
		return "E5B9G9R9_UFloat_Pack32";
	case Format::D16_UNorm:
		return "D16_UNorm";
	case Format::X8_D24_UNorm_Pack32_PoorCoverage:
		return "X8_D24_UNorm_Pack32_PoorCoverage";
	case Format::D32_SFloat:
		return "D32_SFloat";
	case Format::S8_UInt_PoorCoverage:
		return "S8_UInt_PoorCoverage";
	case Format::D16_UNorm_S8_UInt_PoorCoverage:
		return "D16_UNorm_S8_UInt_PoorCoverage";
	case Format::D24_UNorm_S8_UInt_PoorCoverage:
		return "D24_UNorm_S8_UInt_PoorCoverage";
	case Format::D32_SFloat_S8_UInt:
		return "D32_SFloat_S8_UInt";
	case Format::BC1_RGB_UNorm_Block:
		return "BC1_RGB_UNorm_Block";
	case Format::BC1_RGB_SRGB_Block:
		return "BC1_RGB_SRGB_Block";
	case Format::BC1_RGBA_UNorm_Block:
		return "BC1_RGBA_UNorm_Block";
	case Format::BC1_RGBA_SRGB_Block:
		return "BC1_RGBA_SRGB_Block";
	case Format::BC2_UNorm_Block:
		return "BC2_UNorm_Block";
	case Format::BC2_SRGB_Block:
		return "BC2_SRGB_Block";
	case Format::BC3_UNorm_Block:
		return "BC3_UNorm_Block";
	case Format::BC3_SRGB_Block:
		return "BC3_SRGB_Block";
	case Format::BC4_UNorm_Block:
		return "BC4_UNorm_Block";
	case Format::BC4_SNorm_Block:
		return "BC4_SNorm_Block";
	case Format::BC5_UNorm_Block:
		return "BC5_UNorm_Block";
	case Format::BC5_SNorm_Block:
		return "BC5_SNorm_Block";
	case Format::BC6H_UFloat_Block:
		return "BC6H_UFloat_Block";
	case Format::BC6H_SFloat_Block:
		return "BC6H_SFloat_Block";
	case Format::BC7_UNorm_Block:
		return "BC7_UNorm_Block";
	case Format::BC7_SRGB_Block:
		return "BC7_SRGB_Block";
	case Format::ETC2_R8G8B8_UNorm_Block_PoorCoverage:
		return "ETC2_R8G8B8_UNorm_Block_PoorCoverage";
	case Format::ETC2_R8G8B8_SRGB_Block_PoorCoverage:
		return "ETC2_R8G8B8_SRGB_Block_PoorCoverage";
	case Format::ETC2_R8G8B8A1_UNorm_Block_PoorCoverage:
		return "ETC2_R8G8B8A1_UNorm_Block_PoorCoverage";
	case Format::ETC2_R8G8B8A1_SRGB_Block_PoorCoverage:
		return "ETC2_R8G8B8A1_SRGB_Block_PoorCoverage";
	case Format::ETC2_R8G8B8A8_UNorm_Block_PoorCoverage:
		return "ETC2_R8G8B8A8_UNorm_Block_PoorCoverage";
	case Format::ETC2_R8G8B8A8_SRGB_Block_PoorCoverage:
		return "ETC2_R8G8B8A8_SRGB_Block_PoorCoverage";
	case Format::EAC_R11_UNorm_Block_PoorCoverage:
		return "EAC_R11_UNorm_Block_PoorCoverage";
	case Format::EAC_R11_SNorm_Block_PoorCoverage:
		return "EAC_R11_SNorm_Block_PoorCoverage";
	case Format::EAC_R11G11_UNorm_Block_PoorCoverage:
		return "EAC_R11G11_UNorm_Block_PoorCoverage";
	case Format::EAC_R11G11_SNorm_Block_PoorCoverage:
		return "EAC_R11G11_SNorm_Block_PoorCoverage";
	case Format::ASTC_4x4_UNorm_Block_PoorCoverage:
		return "ASTC_4x4_UNorm_Block_PoorCoverage";
	case Format::ASTC_4x4_SRGB_Block_PoorCoverage:
		return "ASTC_4x4_SRGB_Block_PoorCoverage";
	case Format::ASTC_5x4_UNorm_Block_PoorCoverage:
		return "ASTC_5x4_UNorm_Block_PoorCoverage";
	case Format::ASTC_5x4_SRGB_Block_PoorCoverage:
		return "ASTC_5x4_SRGB_Block_PoorCoverage";
	case Format::ASTC_5x5_UNorm_Block_PoorCoverage:
		return "ASTC_5x5_UNorm_Block_PoorCoverage";
	case Format::ASTC_5x5_SRGB_Block_PoorCoverage:
		return "ASTC_5x5_SRGB_Block_PoorCoverage";
	case Format::ASTC_6x5_UNorm_Block_PoorCoverage:
		return "ASTC_6x5_UNorm_Block_PoorCoverage";
	case Format::ASTC_6x5_SRGB_Block_PoorCoverage:
		return "ASTC_6x5_SRGB_Block_PoorCoverage";
	case Format::ASTC_6x6_UNorm_Block_PoorCoverage:
		return "ASTC_6x6_UNorm_Block_PoorCoverage";
	case Format::ASTC_6x6_SRGB_Block_PoorCoverage:
		return "ASTC_6x6_SRGB_Block_PoorCoverage";
	case Format::ASTC_8x5_UNorm_Block_PoorCoverage:
		return "ASTC_8x5_UNorm_Block_PoorCoverage";
	case Format::ASTC_8x5_SRGB_Block_PoorCoverage:
		return "ASTC_8x5_SRGB_Block_PoorCoverage";
	case Format::ASTC_8x6_UNorm_Block_PoorCoverage:
		return "ASTC_8x6_UNorm_Block_PoorCoverage";
	case Format::ASTC_8x6_SRGB_Block_PoorCoverage:
		return "ASTC_8x6_SRGB_Block_PoorCoverage";
	case Format::ASTC_8x8_UNorm_Block_PoorCoverage:
		return "ASTC_8x8_UNorm_Block_PoorCoverage";
	case Format::ASTC_8x8_SRGB_Block_PoorCoverage:
		return "ASTC_8x8_SRGB_Block_PoorCoverage";
	case Format::ASTC_10x5_UNorm_Block_PoorCoverage:
		return "ASTC_10x5_UNorm_Block_PoorCoverage";
	case Format::ASTC_10x5_SRGB_Block_PoorCoverage:
		return "ASTC_10x5_SRGB_Block_PoorCoverage";
	case Format::ASTC_10x6_UNorm_Block_PoorCoverage:
		return "ASTC_10x6_UNorm_Block_PoorCoverage";
	case Format::ASTC_10x6_SRGB_Block_PoorCoverage:
		return "ASTC_10x6_SRGB_Block_PoorCoverage";
	case Format::ASTC_10x8_UNorm_Block_PoorCoverage:
		return "ASTC_10x8_UNorm_Block_PoorCoverage";
	case Format::ASTC_10x8_SRGB_Block_PoorCoverage:
		return "ASTC_10x8_SRGB_Block_PoorCoverage";
	case Format::ASTC_10x10_UNorm_Block_PoorCoverage:
		return "ASTC_10x10_UNorm_Block_PoorCoverage";
	case Format::ASTC_10x10_SRGB_Block_PoorCoverage:
		return "ASTC_10x10_SRGB_Block_PoorCoverage";
	case Format::ASTC_12x10_UNorm_Block_PoorCoverage:
		return "ASTC_12x10_UNorm_Block_PoorCoverage";
	case Format::ASTC_12x10_SRGB_Block_PoorCoverage:
		return "ASTC_12x10_SRGB_Block_PoorCoverage";
	case Format::ASTC_12x12_UNorm_Block_PoorCoverage:
		return "ASTC_12x12_UNorm_Block_PoorCoverage";
	case Format::ASTC_12x12_SRGB_Block_PoorCoverage:
		return "ASTC_12x12_SRGB_Block_PoorCoverage";
	}
	return "Invalid";
}
std::string prosper::util::to_string(prosper::Format format) { return ::to_string(format); }
static std::string to_string(prosper::ShaderStageFlags value)
{
	switch(value) {
	case prosper::ShaderStageFlags::VertexBit:
		return "Vertex";
	case prosper::ShaderStageFlags::TessellationControlBit:
		return "TessellationControl";
	case prosper::ShaderStageFlags::TessellationEvaluationBit:
		return "TessellationEvaluation";
	case prosper::ShaderStageFlags::GeometryBit:
		return "Geometry";
	case prosper::ShaderStageFlags::FragmentBit:
		return "Fragment";
	case prosper::ShaderStageFlags::ComputeBit:
		return "Compute";
	case prosper::ShaderStageFlags::AllGraphics:
		return "AllGraphics";
	case prosper::ShaderStageFlags::All:
		return "All";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::ShaderStageFlags shaderStage) { return ::to_string(shaderStage); }
static std::string to_string(prosper::ImageUsageFlags value)
{
	switch(value) {
	case prosper::ImageUsageFlags::TransferSrcBit:
		return "TransferSrc";
	case prosper::ImageUsageFlags::TransferDstBit:
		return "TransferDst";
	case prosper::ImageUsageFlags::SampledBit:
		return "Sampled";
	case prosper::ImageUsageFlags::StorageBit:
		return "Storage";
	case prosper::ImageUsageFlags::ColorAttachmentBit:
		return "ColorAttachment";
	case prosper::ImageUsageFlags::DepthStencilAttachmentBit:
		return "DepthStencilAttachment";
	case prosper::ImageUsageFlags::TransientAttachmentBit:
		return "TransientAttachment";
	case prosper::ImageUsageFlags::InputAttachmentBit:
		return "InputAttachment";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::ImageUsageFlags usage)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(usage));
	std::string r;
	for(auto it = values.begin(); it != values.end(); ++it) {
		if(it != values.begin())
			r += " | ";
		r += ::to_string(static_cast<prosper::ImageUsageFlags>(*it));
	}
	return r;
}
static std::string to_string(prosper::ImageCreateFlags value)
{
	switch(value) {
	case prosper::ImageCreateFlags::SparseBindingBit:
		return "SparseBinding";
	case prosper::ImageCreateFlags::SparseResidencyBit:
		return "SparseResidency";
	case prosper::ImageCreateFlags::SparseAliasedBit:
		return "SparseAliased";
	case prosper::ImageCreateFlags::MutableFormatBit:
		return "MutableFormat";
	case prosper::ImageCreateFlags::CubeCompatibleBit:
		return "CubeCompatible";
	case prosper::ImageCreateFlags::AliasBit:
		return "Alias";
	case prosper::ImageCreateFlags::SplitInstanceBindRegionsBit:
		return "SplitInstanceBindRegions";
	case prosper::ImageCreateFlags::e2DArrayCompatibleBit:
		return "2DArrayCompatible";
	case prosper::ImageCreateFlags::BlockTexelViewCompatibleBit:
		return "BlockTexelViewCompatible";
	case prosper::ImageCreateFlags::ExtendedUsageBit:
		return "ExtendedUsage";
	case prosper::ImageCreateFlags::ProtectedBit:
		return "Protected";
	case prosper::ImageCreateFlags::DisjointBit:
		return "Disjoint";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::ImageCreateFlags createFlags)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(createFlags));
	std::string r;
	for(auto it = values.begin(); it != values.end(); ++it) {
		if(it != values.begin())
			r += " | ";
		r += ::to_string(static_cast<prosper::ImageCreateFlags>(*it));
	}
	return r;
}
static std::string to_string(prosper::ImageType value)
{
	switch(value) {
	case prosper::ImageType::e1D:
		return "1D";
	case prosper::ImageType::e2D:
		return "2D";
	case prosper::ImageType::e3D:
		return "3D";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::ImageType type) { return ::to_string(type); }
std::string prosper::util::to_string(prosper::ShaderStage stage) { return std::string {magic_enum::enum_name(stage)}; }
static std::string to_string(prosper::PipelineStageFlags value)
{
	switch(value) {
	case prosper::PipelineStageFlags::TopOfPipeBit:
		return "TopOfPipe";
	case prosper::PipelineStageFlags::DrawIndirectBit:
		return "DrawIndirect";
	case prosper::PipelineStageFlags::VertexInputBit:
		return "VertexInput";
	case prosper::PipelineStageFlags::VertexShaderBit:
		return "VertexShader";
	case prosper::PipelineStageFlags::TessellationControlShaderBit:
		return "TessellationControlShader";
	case prosper::PipelineStageFlags::TessellationEvaluationShaderBit:
		return "TessellationEvaluationShader";
	case prosper::PipelineStageFlags::GeometryShaderBit:
		return "GeometryShader";
	case prosper::PipelineStageFlags::FragmentShaderBit:
		return "FragmentShader";
	case prosper::PipelineStageFlags::EarlyFragmentTestsBit:
		return "EarlyFragmentTests";
	case prosper::PipelineStageFlags::LateFragmentTestsBit:
		return "LateFragmentTests";
	case prosper::PipelineStageFlags::ColorAttachmentOutputBit:
		return "ColorAttachmentOutput";
	case prosper::PipelineStageFlags::ComputeShaderBit:
		return "ComputeShader";
	case prosper::PipelineStageFlags::TransferBit:
		return "Transfer";
	case prosper::PipelineStageFlags::BottomOfPipeBit:
		return "BottomOfPipe";
	case prosper::PipelineStageFlags::HostBit:
		return "Host";
	case prosper::PipelineStageFlags::AllGraphics:
		return "AllGraphics";
	case prosper::PipelineStageFlags::AllCommands:
		return "AllCommands";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::PipelineStageFlags stage) { return ::to_string(stage); }
static std::string to_string(prosper::ImageTiling value)
{
	switch(value) {
	case prosper::ImageTiling::Optimal:
		return "Optimal";
	case prosper::ImageTiling::Linear:
		return "Linear";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(prosper::ImageTiling tiling) { return ::to_string(tiling); }
static std::string to_string(prosper::PhysicalDeviceType value)
{
	switch(value) {
	case prosper::PhysicalDeviceType::Other:
		return "Other";
	case prosper::PhysicalDeviceType::IntegratedGPU:
		return "IntegratedGpu";
	case prosper::PhysicalDeviceType::DiscreteGPU:
		return "DiscreteGpu";
	case prosper::PhysicalDeviceType::VirtualGPU:
		return "VirtualGpu";
	case prosper::PhysicalDeviceType::CPU:
		return "Cpu";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(PhysicalDeviceType type) { return ::to_string(type); }
static std::string to_string(prosper::ImageLayout value)
{
	switch(value) {
	case prosper::ImageLayout::Undefined:
		return "Undefined";
	case prosper::ImageLayout::General:
		return "General";
	case prosper::ImageLayout::ColorAttachmentOptimal:
		return "ColorAttachmentOptimal";
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		return "DepthStencilAttachmentOptimal";
	case prosper::ImageLayout::DepthStencilReadOnlyOptimal:
		return "DepthStencilReadOnlyOptimal";
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		return "ShaderReadOnlyOptimal";
	case prosper::ImageLayout::TransferSrcOptimal:
		return "TransferSrcOptimal";
	case prosper::ImageLayout::TransferDstOptimal:
		return "TransferDstOptimal";
	case prosper::ImageLayout::Preinitialized:
		return "Preinitialized";
	// case prosper::ImageLayout::DepthReadOnlyStencilAttachmentOptimal : return "DepthReadOnlyStencilAttachmentOptimal";
	// case prosper::ImageLayout::DepthAttachmentStencilReadOnlyOptimal : return "DepthAttachmentStencilReadOnlyOptimal";
	case prosper::ImageLayout::PresentSrcKHR:
		return "PresentSrcKHR";
	default:
		return "invalid";
	}
}
std::string prosper::util::to_string(ImageLayout layout) { return ::to_string(layout); }
std::string prosper::util::to_string(Vendor vendor)
{
	switch(vendor) {
	case Vendor::AMD:
		return "AMD";
	case Vendor::Nvidia:
		return "NVIDIA";
	case Vendor::Intel:
		return "Intel";
	}
	return "Unknown";
}
std::string prosper::util::to_string(SampleCountFlags samples) { return std::to_string(umath::to_integral(samples)); }
void prosper::util::to_string(const BufferCreateInfo &createInfo, std::stringstream &out)
{
	out << "size: " << createInfo.size << "\n";
	out << "queueFamilyMask: " << magic_enum::flags::enum_name(createInfo.queueFamilyMask) << "\n";
	out << "flags: " << magic_enum::flags::enum_name(createInfo.flags) << "\n";
	out << "usageFlags: " << magic_enum::flags::enum_name(createInfo.usageFlags) << "\n";
	out << "memoryFeatures: " << magic_enum::flags::enum_name(createInfo.memoryFeatures) << "\n";
}
void prosper::util::to_string(const ImageCreateInfo &createInfo, std::stringstream &out)
{
	out << "type: " << magic_enum::enum_name(createInfo.type) << "\n";
	out << "width: " << createInfo.width << "\n";
	out << "height: " << createInfo.height << "\n";
	out << "format: " << umath::to_integral(createInfo.format) << "\n";
	out << "layers: " << createInfo.layers << "\n";
	out << "usage: " << magic_enum::flags::enum_name(createInfo.usage) << "\n";
	out << "samples: " << magic_enum::enum_name(createInfo.samples) << "\n";
	out << "postCreateLayout: " << magic_enum::enum_name(createInfo.postCreateLayout) << "\n";
	out << "flags: " << magic_enum::flags::enum_name(createInfo.flags) << "\n";
	out << "queueFamilyMask: " << magic_enum::flags::enum_name(createInfo.queueFamilyMask) << "\n";
	out << "memoryFeatures: " << magic_enum::flags::enum_name(createInfo.memoryFeatures) << "\n";
}

bool prosper::util::has_alpha(Format format)
{
	switch(format) {
	case Format::BC1_RGBA_UNorm_Block:
	case Format::BC1_RGBA_SRGB_Block:
	case Format::BC2_UNorm_Block:
	case Format::BC2_SRGB_Block:
	case Format::BC3_UNorm_Block:
	case Format::BC3_SRGB_Block:
	case Format::ETC2_R8G8B8A1_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_SRGB_Block_PoorCoverage:
	case Format::R4G4B4A4_UNorm_Pack16:
	case Format::B4G4R4A4_UNorm_Pack16:
	case Format::R5G5B5A1_UNorm_Pack16:
	case Format::B5G5R5A1_UNorm_Pack16:
	case Format::A1R5G5B5_UNorm_Pack16:
	case Format::R8G8B8A8_UNorm:
	case Format::R8G8B8A8_SNorm:
	case Format::R8G8B8A8_UScaled_PoorCoverage:
	case Format::R8G8B8A8_SScaled_PoorCoverage:
	case Format::R8G8B8A8_UInt:
	case Format::R8G8B8A8_SInt:
	case Format::R8G8B8A8_SRGB:
	case Format::B8G8R8A8_UNorm:
	case Format::B8G8R8A8_SNorm:
	case Format::B8G8R8A8_UScaled_PoorCoverage:
	case Format::B8G8R8A8_SScaled_PoorCoverage:
	case Format::B8G8R8A8_UInt:
	case Format::B8G8R8A8_SInt:
	case Format::B8G8R8A8_SRGB:
	case Format::A8B8G8R8_UNorm_Pack32:
	case Format::A8B8G8R8_SNorm_Pack32:
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UInt_Pack32:
	case Format::A8B8G8R8_SInt_Pack32:
	case Format::A8B8G8R8_SRGB_Pack32:
	case Format::A2R10G10B10_UNorm_Pack32:
	case Format::A2R10G10B10_SNorm_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_SScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UInt_Pack32:
	case Format::A2R10G10B10_SInt_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UNorm_Pack32:
	case Format::A2B10G10R10_SNorm_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_SScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UInt_Pack32:
	case Format::A2B10G10R10_SInt_Pack32_PoorCoverage:
	case Format::R16G16B16A16_UNorm:
	case Format::R16G16B16A16_SNorm:
	case Format::R16G16B16A16_UScaled_PoorCoverage:
	case Format::R16G16B16A16_SScaled_PoorCoverage:
	case Format::R16G16B16A16_UInt:
	case Format::R16G16B16A16_SInt:
	case Format::R16G16B16A16_SFloat:
	case Format::R32G32B32A32_UInt:
	case Format::R32G32B32A32_SInt:
	case Format::R32G32B32A32_SFloat:
	case Format::R64G64B64A64_UInt_PoorCoverage:
	case Format::R64G64B64A64_SInt_PoorCoverage:
	case Format::R64G64B64A64_SFloat_PoorCoverage:
	case Format::BC7_UNorm_Block:
	case Format::BC7_SRGB_Block:
	case Format::ASTC_4x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_4x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x12_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x12_SRGB_Block_PoorCoverage:
		return true;
	}
	return false;
}

bool prosper::util::is_depth_format(Format format)
{
	switch(format) {
	case Format::D16_UNorm:
	case Format::D16_UNorm_S8_UInt_PoorCoverage:
	case Format::D24_UNorm_S8_UInt_PoorCoverage:
	case Format::D32_SFloat:
	case Format::D32_SFloat_S8_UInt:
	case Format::X8_D24_UNorm_Pack32_PoorCoverage:
		return true;
	};
	return false;
}

bool prosper::util::is_compressed_format(Format format)
{
	switch(format) {
	case Format::BC1_RGB_UNorm_Block:
	case Format::BC1_RGB_SRGB_Block:
	case Format::BC1_RGBA_UNorm_Block:
	case Format::BC1_RGBA_SRGB_Block:
	case Format::BC2_UNorm_Block:
	case Format::BC2_SRGB_Block:
	case Format::BC3_UNorm_Block:
	case Format::BC3_SRGB_Block:
	case Format::BC4_UNorm_Block:
	case Format::BC4_SNorm_Block:
	case Format::BC5_UNorm_Block:
	case Format::BC5_SNorm_Block:
	case Format::BC6H_UFloat_Block:
	case Format::BC6H_SFloat_Block:
	case Format::BC7_UNorm_Block:
	case Format::BC7_SRGB_Block:
	case Format::ETC2_R8G8B8_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_SRGB_Block_PoorCoverage:
	case Format::EAC_R11_UNorm_Block_PoorCoverage:
	case Format::EAC_R11_SNorm_Block_PoorCoverage:
	case Format::EAC_R11G11_UNorm_Block_PoorCoverage:
	case Format::EAC_R11G11_SNorm_Block_PoorCoverage:
	case Format::ASTC_4x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_4x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x12_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x12_SRGB_Block_PoorCoverage:
		return true;
	};
	return false;
}
bool prosper::util::is_uncompressed_format(Format format)
{
	if(format == Format::Unknown)
		return false;
	return !is_compressed_format(format);
}

uint32_t prosper::util::get_bit_size(Format format)
{
	switch(format) {
	// Sub-Byte formats
	case Format::Unknown:
		return 0;
	case Format::R4G4_UNorm_Pack8:
		return 8;
	case Format::R4G4B4A4_UNorm_Pack16:
	case Format::B4G4R4A4_UNorm_Pack16:
	case Format::R5G6B5_UNorm_Pack16:
	case Format::B5G6R5_UNorm_Pack16:
	case Format::R5G5B5A1_UNorm_Pack16:
	case Format::B5G5R5A1_UNorm_Pack16:
	case Format::A1R5G5B5_UNorm_Pack16:
		return 16;
	//
	case Format::R8_UNorm:
	case Format::R8_SNorm:
	case Format::R8_UScaled_PoorCoverage:
	case Format::R8_SScaled_PoorCoverage:
	case Format::R8_UInt:
	case Format::R8_SInt:
	case Format::R8_SRGB:
		return 8;
	case Format::R8G8_UNorm:
	case Format::R8G8_SNorm:
	case Format::R8G8_UScaled_PoorCoverage:
	case Format::R8G8_SScaled_PoorCoverage:
	case Format::R8G8_UInt:
	case Format::R8G8_SInt:
	case Format::R8G8_SRGB_PoorCoverage:
		return 16;
	case Format::R8G8B8_UNorm_PoorCoverage:
	case Format::R8G8B8_SNorm_PoorCoverage:
	case Format::R8G8B8_UScaled_PoorCoverage:
	case Format::R8G8B8_SScaled_PoorCoverage:
	case Format::R8G8B8_UInt_PoorCoverage:
	case Format::R8G8B8_SInt_PoorCoverage:
	case Format::R8G8B8_SRGB_PoorCoverage:
	case Format::B8G8R8_UNorm_PoorCoverage:
	case Format::B8G8R8_SNorm_PoorCoverage:
	case Format::B8G8R8_UScaled_PoorCoverage:
	case Format::B8G8R8_SScaled_PoorCoverage:
	case Format::B8G8R8_UInt_PoorCoverage:
	case Format::B8G8R8_SInt_PoorCoverage:
	case Format::B8G8R8_SRGB_PoorCoverage:
		return 24;
	case Format::R8G8B8A8_UNorm:
	case Format::R8G8B8A8_SNorm:
	case Format::R8G8B8A8_UScaled_PoorCoverage:
	case Format::R8G8B8A8_SScaled_PoorCoverage:
	case Format::R8G8B8A8_UInt:
	case Format::R8G8B8A8_SInt:
	case Format::R8G8B8A8_SRGB:
	case Format::B8G8R8A8_UNorm:
	case Format::B8G8R8A8_SNorm:
	case Format::B8G8R8A8_UScaled_PoorCoverage:
	case Format::B8G8R8A8_SScaled_PoorCoverage:
	case Format::B8G8R8A8_UInt:
	case Format::B8G8R8A8_SInt:
	case Format::B8G8R8A8_SRGB:
	case Format::A8B8G8R8_UNorm_Pack32:
	case Format::A8B8G8R8_SNorm_Pack32:
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UInt_Pack32:
	case Format::A8B8G8R8_SInt_Pack32:
	case Format::A8B8G8R8_SRGB_Pack32:
		return 32;
	case Format::A2R10G10B10_UNorm_Pack32:
	case Format::A2R10G10B10_SNorm_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_SScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UInt_Pack32:
	case Format::A2R10G10B10_SInt_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UNorm_Pack32:
	case Format::A2B10G10R10_SNorm_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_SScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UInt_Pack32:
	case Format::A2B10G10R10_SInt_Pack32_PoorCoverage:
		return 32;
	case Format::R16_UNorm:
	case Format::R16_SNorm:
	case Format::R16_UScaled_PoorCoverage:
	case Format::R16_SScaled_PoorCoverage:
	case Format::R16_UInt:
	case Format::R16_SInt:
	case Format::R16_SFloat:
		return 16;
	case Format::R16G16_UNorm:
	case Format::R16G16_SNorm:
	case Format::R16G16_UScaled_PoorCoverage:
	case Format::R16G16_SScaled_PoorCoverage:
	case Format::R16G16_UInt:
	case Format::R16G16_SInt:
	case Format::R16G16_SFloat:
		return 32;
	case Format::R16G16B16_UNorm_PoorCoverage:
	case Format::R16G16B16_SNorm_PoorCoverage:
	case Format::R16G16B16_UScaled_PoorCoverage:
	case Format::R16G16B16_SScaled_PoorCoverage:
	case Format::R16G16B16_UInt_PoorCoverage:
	case Format::R16G16B16_SInt_PoorCoverage:
	case Format::R16G16B16_SFloat_PoorCoverage:
		return 48;
	case Format::R16G16B16A16_UNorm:
	case Format::R16G16B16A16_SNorm:
	case Format::R16G16B16A16_UScaled_PoorCoverage:
	case Format::R16G16B16A16_SScaled_PoorCoverage:
	case Format::R16G16B16A16_UInt:
	case Format::R16G16B16A16_SInt:
	case Format::R16G16B16A16_SFloat:
		return 64;
	case Format::R32_UInt:
	case Format::R32_SInt:
	case Format::R32_SFloat:
		return 32;
	case Format::R32G32_UInt:
	case Format::R32G32_SInt:
	case Format::R32G32_SFloat:
		return 64;
	case Format::R32G32B32_UInt:
	case Format::R32G32B32_SInt:
	case Format::R32G32B32_SFloat:
		return 96;
	case Format::R32G32B32A32_UInt:
	case Format::R32G32B32A32_SInt:
	case Format::R32G32B32A32_SFloat:
		return 128;
	case Format::R64_UInt_PoorCoverage:
	case Format::R64_SInt_PoorCoverage:
	case Format::R64_SFloat_PoorCoverage:
		return 64;
	case Format::R64G64_UInt_PoorCoverage:
	case Format::R64G64_SInt_PoorCoverage:
	case Format::R64G64_SFloat_PoorCoverage:
		return 128;
	case Format::R64G64B64_UInt_PoorCoverage:
	case Format::R64G64B64_SInt_PoorCoverage:
	case Format::R64G64B64_SFloat_PoorCoverage:
		return 192;
	case Format::R64G64B64A64_UInt_PoorCoverage:
	case Format::R64G64B64A64_SInt_PoorCoverage:
	case Format::R64G64B64A64_SFloat_PoorCoverage:
		return 256;
	case Format::B10G11R11_UFloat_Pack32:
	case Format::E5B9G9R9_UFloat_Pack32:
		return 32;
	case Format::D16_UNorm:
		return 16;
	case Format::X8_D24_UNorm_Pack32_PoorCoverage:
		return 32;
	case Format::D32_SFloat:
		return 32;
	case Format::S8_UInt_PoorCoverage:
		return 8;
	case Format::D16_UNorm_S8_UInt_PoorCoverage:
		return 16;
	case Format::D24_UNorm_S8_UInt_PoorCoverage:
		return 24;
	case Format::D32_SFloat_S8_UInt:
		return 32;
	};
	return 0;
}

bool prosper::util::is_8bit_format(Format format)
{
	switch(format) {
	case Format::R8_UNorm:
	case Format::R8_SNorm:
	case Format::R8_UScaled_PoorCoverage:
	case Format::R8_SScaled_PoorCoverage:
	case Format::R8_UInt:
	case Format::R8_SInt:
	case Format::R8_SRGB:
	case Format::R8G8_UNorm:
	case Format::R8G8_SNorm:
	case Format::R8G8_UScaled_PoorCoverage:
	case Format::R8G8_SScaled_PoorCoverage:
	case Format::R8G8_UInt:
	case Format::R8G8_SInt:
	case Format::R8G8_SRGB_PoorCoverage:
	case Format::R8G8B8_UNorm_PoorCoverage:
	case Format::R8G8B8_SNorm_PoorCoverage:
	case Format::R8G8B8_UScaled_PoorCoverage:
	case Format::R8G8B8_SScaled_PoorCoverage:
	case Format::R8G8B8_UInt_PoorCoverage:
	case Format::R8G8B8_SInt_PoorCoverage:
	case Format::R8G8B8_SRGB_PoorCoverage:
	case Format::B8G8R8_UNorm_PoorCoverage:
	case Format::B8G8R8_SNorm_PoorCoverage:
	case Format::B8G8R8_UScaled_PoorCoverage:
	case Format::B8G8R8_SScaled_PoorCoverage:
	case Format::B8G8R8_UInt_PoorCoverage:
	case Format::B8G8R8_SInt_PoorCoverage:
	case Format::B8G8R8_SRGB_PoorCoverage:
	case Format::R8G8B8A8_UNorm:
	case Format::R8G8B8A8_SNorm:
	case Format::R8G8B8A8_UScaled_PoorCoverage:
	case Format::R8G8B8A8_SScaled_PoorCoverage:
	case Format::R8G8B8A8_UInt:
	case Format::R8G8B8A8_SInt:
	case Format::R8G8B8A8_SRGB:
	case Format::B8G8R8A8_UNorm:
	case Format::B8G8R8A8_SNorm:
	case Format::B8G8R8A8_UScaled_PoorCoverage:
	case Format::B8G8R8A8_SScaled_PoorCoverage:
	case Format::B8G8R8A8_UInt:
	case Format::B8G8R8A8_SInt:
	case Format::B8G8R8A8_SRGB:
	case Format::A8B8G8R8_UNorm_Pack32:
	case Format::A8B8G8R8_SNorm_Pack32:
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UInt_Pack32:
	case Format::A8B8G8R8_SInt_Pack32:
	case Format::A8B8G8R8_SRGB_Pack32:
	case Format::S8_UInt_PoorCoverage:
		return true;
	};
	return false;
}
bool prosper::util::is_16bit_format(Format format)
{
	switch(format) {
	case Format::R16_UNorm:
	case Format::R16_SNorm:
	case Format::R16_UScaled_PoorCoverage:
	case Format::R16_SScaled_PoorCoverage:
	case Format::R16_UInt:
	case Format::R16_SInt:
	case Format::R16_SFloat:
	case Format::R16G16_UNorm:
	case Format::R16G16_SNorm:
	case Format::R16G16_UScaled_PoorCoverage:
	case Format::R16G16_SScaled_PoorCoverage:
	case Format::R16G16_UInt:
	case Format::R16G16_SInt:
	case Format::R16G16_SFloat:
	case Format::R16G16B16_UNorm_PoorCoverage:
	case Format::R16G16B16_SNorm_PoorCoverage:
	case Format::R16G16B16_UScaled_PoorCoverage:
	case Format::R16G16B16_SScaled_PoorCoverage:
	case Format::R16G16B16_UInt_PoorCoverage:
	case Format::R16G16B16_SInt_PoorCoverage:
	case Format::R16G16B16_SFloat_PoorCoverage:
	case Format::R16G16B16A16_UNorm:
	case Format::R16G16B16A16_SNorm:
	case Format::R16G16B16A16_UScaled_PoorCoverage:
	case Format::R16G16B16A16_SScaled_PoorCoverage:
	case Format::R16G16B16A16_UInt:
	case Format::R16G16B16A16_SInt:
	case Format::R16G16B16A16_SFloat:
	case Format::D16_UNorm:
	case Format::D16_UNorm_S8_UInt_PoorCoverage:
		return true;
	};
	return false;
}
bool prosper::util::is_32bit_format(Format format)
{
	switch(format) {
	case Format::R16_UNorm:
	case Format::R16_SNorm:
	case Format::R16_UScaled_PoorCoverage:
	case Format::R16_SScaled_PoorCoverage:
	case Format::R16_UInt:
	case Format::R16_SInt:
	case Format::R16_SFloat:
	case Format::R16G16_UNorm:
	case Format::R16G16_SNorm:
	case Format::R16G16_UScaled_PoorCoverage:
	case Format::R16G16_SScaled_PoorCoverage:
	case Format::R16G16_UInt:
	case Format::R16G16_SInt:
	case Format::R16G16_SFloat:
	case Format::R16G16B16_UNorm_PoorCoverage:
	case Format::R16G16B16_SNorm_PoorCoverage:
	case Format::R16G16B16_UScaled_PoorCoverage:
	case Format::R16G16B16_SScaled_PoorCoverage:
	case Format::R16G16B16_UInt_PoorCoverage:
	case Format::R16G16B16_SInt_PoorCoverage:
	case Format::R16G16B16_SFloat_PoorCoverage:
	case Format::R16G16B16A16_UNorm:
	case Format::R16G16B16A16_SNorm:
	case Format::R16G16B16A16_UScaled_PoorCoverage:
	case Format::R16G16B16A16_SScaled_PoorCoverage:
	case Format::R16G16B16A16_UInt:
	case Format::R16G16B16A16_SInt:
	case Format::R16G16B16A16_SFloat:
	case Format::D16_UNorm:
	case Format::D16_UNorm_S8_UInt_PoorCoverage:
		return true;
	};
	return false;
}
bool prosper::util::is_64bit_format(Format format)
{
	switch(format) {
	case Format::R64_UInt_PoorCoverage:
	case Format::R64_SInt_PoorCoverage:
	case Format::R64_SFloat_PoorCoverage:
	case Format::R64G64_UInt_PoorCoverage:
	case Format::R64G64_SInt_PoorCoverage:
	case Format::R64G64_SFloat_PoorCoverage:
	case Format::R64G64B64_UInt_PoorCoverage:
	case Format::R64G64B64_SInt_PoorCoverage:
	case Format::R64G64B64_SFloat_PoorCoverage:
	case Format::R64G64B64A64_UInt_PoorCoverage:
	case Format::R64G64B64A64_SInt_PoorCoverage:
	case Format::R64G64B64A64_SFloat_PoorCoverage:
		return true;
	};
	return false;
}

uint32_t prosper::util::get_byte_size(Format format)
{
	auto numBits = get_bit_size(format);
	if(numBits == 0 || (numBits % 8) != 0)
		return 0;
	return numBits / 8;
}

uint32_t prosper::util::get_block_size(Format format) { return gli_wrapper::get_block_size(format); }

uint32_t prosper::util::get_pixel_size(Format format) { return is_compressed_format(format) ? get_block_size(format) : get_byte_size(format); }

uint32_t prosper::util::get_component_count(Format format) { return gli_wrapper::get_component_count(format); }

bool prosper::util::get_format_channel_mask(Format format, uimg::ChannelMask &outChannelMask)
{
	switch(format) {
	case Format::R8_UNorm:
	case Format::R8_SNorm:
	case Format::R8_UScaled_PoorCoverage:
	case Format::R8_SScaled_PoorCoverage:
	case Format::R8_UInt:
	case Format::R8_SInt:
	case Format::R8_SRGB:
	case Format::R8G8_UNorm:
	case Format::R8G8_SNorm:
	case Format::R8G8_UScaled_PoorCoverage:
	case Format::R8G8_SScaled_PoorCoverage:
	case Format::R8G8_UInt:
	case Format::R8G8_SInt:
	case Format::R8G8_SRGB_PoorCoverage:
	case Format::R8G8B8_UNorm_PoorCoverage:
	case Format::R8G8B8_SNorm_PoorCoverage:
	case Format::R8G8B8_UScaled_PoorCoverage:
	case Format::R8G8B8_SScaled_PoorCoverage:
	case Format::R8G8B8_UInt_PoorCoverage:
	case Format::R8G8B8_SInt_PoorCoverage:
	case Format::R8G8B8_SRGB_PoorCoverage:
	case Format::R8G8B8A8_UNorm:
	case Format::R8G8B8A8_SNorm:
	case Format::R8G8B8A8_UScaled_PoorCoverage:
	case Format::R8G8B8A8_SScaled_PoorCoverage:
	case Format::R8G8B8A8_UInt:
	case Format::R8G8B8A8_SInt:
	case Format::R8G8B8A8_SRGB:
	case Format::R16_UNorm:
	case Format::R16_SNorm:
	case Format::R16_UScaled_PoorCoverage:
	case Format::R16_SScaled_PoorCoverage:
	case Format::R16_UInt:
	case Format::R16_SInt:
	case Format::R16_SFloat:
	case Format::R16G16_UNorm:
	case Format::R16G16_SNorm:
	case Format::R16G16_UScaled_PoorCoverage:
	case Format::R16G16_SScaled_PoorCoverage:
	case Format::R16G16_UInt:
	case Format::R16G16_SInt:
	case Format::R16G16_SFloat:
	case Format::R16G16B16_UNorm_PoorCoverage:
	case Format::R16G16B16_SNorm_PoorCoverage:
	case Format::R16G16B16_UScaled_PoorCoverage:
	case Format::R16G16B16_SScaled_PoorCoverage:
	case Format::R16G16B16_UInt_PoorCoverage:
	case Format::R16G16B16_SInt_PoorCoverage:
	case Format::R16G16B16_SFloat_PoorCoverage:
	case Format::R16G16B16A16_UNorm:
	case Format::R16G16B16A16_SNorm:
	case Format::R16G16B16A16_UScaled_PoorCoverage:
	case Format::R16G16B16A16_SScaled_PoorCoverage:
	case Format::R16G16B16A16_UInt:
	case Format::R16G16B16A16_SInt:
	case Format::R16G16B16A16_SFloat:
	case Format::R32_UInt:
	case Format::R32_SInt:
	case Format::R32_SFloat:
	case Format::R32G32_UInt:
	case Format::R32G32_SInt:
	case Format::R32G32_SFloat:
	case Format::R32G32B32_UInt:
	case Format::R32G32B32_SInt:
	case Format::R32G32B32_SFloat:
	case Format::R32G32B32A32_UInt:
	case Format::R32G32B32A32_SInt:
	case Format::R32G32B32A32_SFloat:
	case Format::R64_UInt_PoorCoverage:
	case Format::R64_SInt_PoorCoverage:
	case Format::R64_SFloat_PoorCoverage:
	case Format::R64G64_UInt_PoorCoverage:
	case Format::R64G64_SInt_PoorCoverage:
	case Format::R64G64_SFloat_PoorCoverage:
	case Format::R64G64B64_UInt_PoorCoverage:
	case Format::R64G64B64_SInt_PoorCoverage:
	case Format::R64G64B64_SFloat_PoorCoverage:
	case Format::R64G64B64A64_UInt_PoorCoverage:
	case Format::R64G64B64A64_SInt_PoorCoverage:
	case Format::R64G64B64A64_SFloat_PoorCoverage:
	case Format::BC1_RGB_UNorm_Block:
	case Format::BC1_RGB_SRGB_Block:
	case Format::BC1_RGBA_UNorm_Block:
	case Format::BC1_RGBA_SRGB_Block:
	case Format::BC2_UNorm_Block:
	case Format::BC2_SRGB_Block:
	case Format::BC3_UNorm_Block:
	case Format::BC3_SRGB_Block:
	case Format::BC4_UNorm_Block:
	case Format::BC4_SNorm_Block:
	case Format::BC5_UNorm_Block:
	case Format::BC5_SNorm_Block:
	case Format::BC6H_UFloat_Block:
	case Format::BC6H_SFloat_Block:
	case Format::BC7_UNorm_Block:
	case Format::BC7_SRGB_Block:
	case Format::ETC2_R8G8B8_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_UNorm_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_SRGB_Block_PoorCoverage:
	case Format::EAC_R11_UNorm_Block_PoorCoverage:
	case Format::EAC_R11_SNorm_Block_PoorCoverage:
	case Format::EAC_R11G11_UNorm_Block_PoorCoverage:
	case Format::EAC_R11G11_SNorm_Block_PoorCoverage:
	case Format::ASTC_4x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_4x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x4_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_5x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_6x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_8x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x5_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x6_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x8_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_10x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x10_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x12_UNorm_Block_PoorCoverage:
	case Format::ASTC_12x12_SRGB_Block_PoorCoverage:
		outChannelMask = {uimg::Channel::R, uimg::Channel::G, uimg::Channel::B, uimg::Channel::A};
		return true;
	case Format::B8G8R8_UNorm_PoorCoverage:
	case Format::B8G8R8_SNorm_PoorCoverage:
	case Format::B8G8R8_UScaled_PoorCoverage:
	case Format::B8G8R8_SScaled_PoorCoverage:
	case Format::B8G8R8_UInt_PoorCoverage:
	case Format::B8G8R8_SInt_PoorCoverage:
	case Format::B8G8R8_SRGB_PoorCoverage:
	case Format::B8G8R8A8_UNorm:
	case Format::B8G8R8A8_SNorm:
	case Format::B8G8R8A8_UScaled_PoorCoverage:
	case Format::B8G8R8A8_SScaled_PoorCoverage:
	case Format::B8G8R8A8_UInt:
	case Format::B8G8R8A8_SInt:
	case Format::B8G8R8A8_SRGB:
		outChannelMask = {uimg::Channel::B, uimg::Channel::G, uimg::Channel::R, uimg::Channel::A};
		return true;
	// Note: Pack formats have reverse order due to endianness! Unused channels are set to uimg::Channel::A (unless all channels are used)
	case Format::R4G4_UNorm_Pack8:
		outChannelMask = {uimg::Channel::G, uimg::Channel::R, uimg::Channel::A, uimg::Channel::A};
		return true;
	case Format::R4G4B4A4_UNorm_Pack16:
	case Format::R5G5B5A1_UNorm_Pack16:
		outChannelMask = {uimg::Channel::A, uimg::Channel::B, uimg::Channel::G, uimg::Channel::R};
		return true;
	case Format::R5G6B5_UNorm_Pack16:
		outChannelMask = {uimg::Channel::B, uimg::Channel::G, uimg::Channel::R, uimg::Channel::A};
		return true;
	case Format::B5G6R5_UNorm_Pack16:
		outChannelMask = {uimg::Channel::R, uimg::Channel::G, uimg::Channel::B, uimg::Channel::A};
		return true;
	case Format::B4G4R4A4_UNorm_Pack16:
	case Format::B5G5R5A1_UNorm_Pack16:
		outChannelMask = {uimg::Channel::A, uimg::Channel::R, uimg::Channel::G, uimg::Channel::B};
		return true;
	case Format::B10G11R11_UFloat_Pack32:
		outChannelMask = {uimg::Channel::R, uimg::Channel::G, uimg::Channel::B, uimg::Channel::A};
		return true;
	case Format::A1R5G5B5_UNorm_Pack16:
	case Format::A2R10G10B10_UNorm_Pack32:
	case Format::A2R10G10B10_SNorm_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_SScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UInt_Pack32:
	case Format::A2R10G10B10_SInt_Pack32_PoorCoverage:
		outChannelMask = {uimg::Channel::B, uimg::Channel::G, uimg::Channel::R, uimg::Channel::A};
		return true;
	case Format::A8B8G8R8_UNorm_Pack32:
	case Format::A8B8G8R8_SNorm_Pack32:
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UInt_Pack32:
	case Format::A8B8G8R8_SInt_Pack32:
	case Format::A8B8G8R8_SRGB_Pack32:
	case Format::A2B10G10R10_UNorm_Pack32:
	case Format::A2B10G10R10_SNorm_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_SScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UInt_Pack32:
	case Format::A2B10G10R10_SInt_Pack32_PoorCoverage:
		outChannelMask = {uimg::Channel::R, uimg::Channel::G, uimg::Channel::B, uimg::Channel::A};
		return true;
	}
	return false;
}

prosper::AccessFlags prosper::util::get_read_access_mask()
{
	return prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::DepthStencilAttachmentReadBit | prosper::AccessFlags::HostReadBit | prosper::AccessFlags::IndexReadBit | prosper::AccessFlags::IndirectCommandReadBit | prosper::AccessFlags::InputAttachmentReadBit
	  | prosper::AccessFlags::MemoryReadBit | prosper::AccessFlags::ShaderReadBit | prosper::AccessFlags::TransferReadBit | prosper::AccessFlags::UniformReadBit | prosper::AccessFlags::VertexAttributeReadBit;
}
prosper::AccessFlags prosper::util::get_write_access_mask()
{
	return prosper::AccessFlags::ColorAttachmentWriteBit | prosper::AccessFlags::DepthStencilAttachmentWriteBit | prosper::AccessFlags::HostWriteBit | prosper::AccessFlags::MemoryWriteBit | prosper::AccessFlags::ShaderWriteBit | prosper::AccessFlags::TransferWriteBit;
}

prosper::AccessFlags prosper::util::get_image_read_access_mask()
{
	return prosper::AccessFlags::ColorAttachmentReadBit | prosper::AccessFlags::HostReadBit | prosper::AccessFlags::MemoryReadBit | prosper::AccessFlags::ShaderReadBit | prosper::AccessFlags::TransferReadBit | prosper::AccessFlags::UniformReadBit;
}
prosper::AccessFlags prosper::util::get_image_write_access_mask() { return prosper::AccessFlags::ColorAttachmentWriteBit | prosper::AccessFlags::HostWriteBit | prosper::AccessFlags::MemoryWriteBit | prosper::AccessFlags::ShaderWriteBit | prosper::AccessFlags::TransferWriteBit; }

prosper::PipelineBindPoint prosper::util::get_pipeline_bind_point(prosper::ShaderStageFlags shaderStages) { return ((shaderStages & prosper::ShaderStageFlags::ComputeBit) != prosper::ShaderStageFlags(0)) ? prosper::PipelineBindPoint::Compute : prosper::PipelineBindPoint::Graphics; }

prosper::ShaderStage prosper::util::shader_stage_flag_to_shader_stage(prosper::ShaderStageFlags flag)
{
	switch(flag) {
	case ShaderStageFlags::ComputeBit:
		return ShaderStage::Compute;
	case ShaderStageFlags::FragmentBit:
		return ShaderStage::Fragment;
	case ShaderStageFlags::GeometryBit:
		return ShaderStage::Geometry;
	case ShaderStageFlags::TessellationControlBit:
		return ShaderStage::TessellationControl;
	case ShaderStageFlags::TessellationEvaluationBit:
		return ShaderStage::TessellationEvaluation;
	case ShaderStageFlags::VertexBit:
		return ShaderStage::Vertex;
	}
	throw std::logic_error {std::to_string(umath::to_integral(flag)) + " is not a unique shader stage flag"};
}

std::vector<prosper::ShaderStage> prosper::util::shader_stage_flags_to_shader_stages(prosper::ShaderStageFlags flags)
{
	std::vector<prosper::ShaderStage> stages;
	auto values = umath::get_power_of_2_values(umath::to_integral(flags));
	stages.reserve(values.size());
	for(auto v : values)
		stages.push_back(shader_stage_flag_to_shader_stage(static_cast<prosper::ShaderStageFlags>(v)));
	return stages;
}

prosper::ImageAspectFlags prosper::util::get_aspect_mask(Format format)
{
	auto aspectMask = ImageAspectFlags::ColorBit;
	if(is_depth_format(format))
		aspectMask = ImageAspectFlags::DepthBit;
	return aspectMask;
}

uint32_t prosper::util::get_offset_alignment_padding(uint32_t offset, uint32_t alignment)
{
	if(alignment == 0u)
		return 0u;
	auto r = offset % alignment;
	if(r == 0u)
		return 0u;
	return alignment - r;
}
uint32_t prosper::util::get_aligned_size(uint32_t size, uint32_t alignment)
{
	if(alignment == 0u)
		return size;
	auto r = size % alignment;
	if(r == 0u)
		return size;
	return size + (alignment - r);
}

prosper::ImageAspectFlags prosper::util::get_aspect_mask(IImage &img) { return get_aspect_mask(img.GetFormat()); }

void prosper::util::get_image_layout_transition_access_masks(prosper::ImageLayout oldLayout, prosper::ImageLayout newLayout, prosper::AccessFlags &readAccessMask, prosper::AccessFlags &writeAccessMask)
{
	// Source Layout
	switch(oldLayout) {
		// Undefined layout
		// Only allowed as initial layout!
		// Make sure any writes to the image have been finished
	case prosper::ImageLayout::Preinitialized:
		readAccessMask = prosper::AccessFlags::HostWriteBit | prosper::AccessFlags::TransferWriteBit;
		break;
		// Old layout is color attachment
		// Make sure any writes to the color buffer have been finished
	case prosper::ImageLayout::ColorAttachmentOptimal:
		readAccessMask = prosper::AccessFlags::ColorAttachmentWriteBit;
		break;
		// Old layout is depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		readAccessMask = prosper::AccessFlags::DepthStencilAttachmentWriteBit;
		break;
		// Old layout is transfer source
		// Make sure any reads from the image have been finished
	case prosper::ImageLayout::TransferSrcOptimal:
		readAccessMask = prosper::AccessFlags::TransferReadBit;
		break;
		// Old layout is shader read (sampler, input attachment)
		// Make sure any shader reads from the image have been finished
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		readAccessMask = prosper::AccessFlags::ShaderReadBit;
		break;
	};

	// Target Layout
	switch(newLayout) {
		// New layout is transfer destination (copy, blit)
		// Make sure any copyies to the image have been finished
	case prosper::ImageLayout::TransferDstOptimal:
		writeAccessMask = prosper::AccessFlags::TransferWriteBit;
		break;
		// New layout is transfer source (copy, blit)
		// Make sure any reads from and writes to the image have been finished
	case prosper::ImageLayout::TransferSrcOptimal:
		readAccessMask = readAccessMask | prosper::AccessFlags::TransferReadBit;
		writeAccessMask = prosper::AccessFlags::TransferReadBit;
		break;
		// New layout is color attachment
		// Make sure any writes to the color buffer hav been finished
	case prosper::ImageLayout::ColorAttachmentOptimal:
		writeAccessMask = prosper::AccessFlags::ColorAttachmentWriteBit;
		readAccessMask = readAccessMask | prosper::AccessFlags::TransferReadBit;
		break;
		// New layout is depth attachment
		// Make sure any writes to depth/stencil buffer have been finished
	case prosper::ImageLayout::DepthStencilAttachmentOptimal:
		writeAccessMask = writeAccessMask | prosper::AccessFlags::DepthStencilAttachmentWriteBit;
		break;
		// New layout is shader read (sampler, input attachment)
		// Make sure any writes to the image have been finished
	case prosper::ImageLayout::ShaderReadOnlyOptimal:
		readAccessMask = readAccessMask | prosper::AccessFlags::HostWriteBit | prosper::AccessFlags::TransferWriteBit;
		writeAccessMask = prosper::AccessFlags::ShaderReadBit;
		break;
	case prosper::ImageLayout::DepthStencilReadOnlyOptimal:
		readAccessMask = readAccessMask | prosper::AccessFlags::ShaderWriteBit;
		writeAccessMask = prosper::AccessFlags::ShaderReadBit;
		break;
	};
}

static prosper::Format get_prosper_format(uimg::TextureInfo::InputFormat format)
{
	prosper::Format anvFormat = prosper::Format::R8G8B8A8_UNorm;
	switch(format) {
	case uimg::TextureInfo::InputFormat::R8G8B8A8_UInt:
		anvFormat = prosper::Format::R8G8B8A8_UNorm;
		break;
	case uimg::TextureInfo::InputFormat::B8G8R8A8_UInt:
		anvFormat = prosper::Format::B8G8R8A8_UNorm;
		break;
	case uimg::TextureInfo::InputFormat::R16G16B16A16_Float:
		anvFormat = prosper::Format::R16G16B16A16_SFloat;
		break;
	case uimg::TextureInfo::InputFormat::R32G32B32A32_Float:
		anvFormat = prosper::Format::R32G32B32A32_SFloat;
		break;
	case uimg::TextureInfo::InputFormat::R32_Float:
		anvFormat = prosper::Format::R32_SFloat;
		break;
	case uimg::TextureInfo::InputFormat::KeepInputImageFormat:
		break;
	}
	static_assert(umath::to_integral(uimg::TextureInfo::InputFormat::Count) == 6, "Update this list!");
	return anvFormat;
}
static prosper::Format get_prosper_format(uimg::TextureInfo::OutputFormat format)
{
	prosper::Format anvFormat = prosper::Format::R8G8B8A8_UNorm;
	switch(format) {
	case uimg::TextureInfo::OutputFormat::RGB:
	case uimg::TextureInfo::OutputFormat::RGBA:
		return prosper::Format::R8G8B8A8_UNorm;
	case uimg::TextureInfo::OutputFormat::BC1:
	// case uimg::TextureInfo::OutputFormat::DXT1n:
	case uimg::TextureInfo::OutputFormat::CTX1:
	case uimg::TextureInfo::OutputFormat::BC1a:
		return prosper::Format::BC1_RGBA_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC2:
		return prosper::Format::BC2_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC3:
	case uimg::TextureInfo::OutputFormat::BC3n:
		// case uimg::TextureInfo::OutputFormat::BC3_RGBM:
		return prosper::Format::BC3_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC4:
		return prosper::Format::BC4_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC5:
		return prosper::Format::BC5_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC6:
		return prosper::Format::BC6H_SFloat_Block;
	case uimg::TextureInfo::OutputFormat::BC7:
		return prosper::Format::BC7_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC1:
	case uimg::TextureInfo::OutputFormat::ETC2_R:
	case uimg::TextureInfo::OutputFormat::ETC2_RG:
	case uimg::TextureInfo::OutputFormat::ETC2_RGB:
	case uimg::TextureInfo::OutputFormat::ETC2_RGBA:
	case uimg::TextureInfo::OutputFormat::ETC2_RGB_A1:
		// case uimg::TextureInfo::OutputFormat::ETC2_RGBM:
		return prosper::Format::ETC2_R8G8B8A8_UNorm_Block_PoorCoverage;
	}
	return anvFormat;
}
static bool is_compatible(prosper::Format imgFormat, uimg::TextureInfo::OutputFormat outputFormat)
{
	if(outputFormat == uimg::TextureInfo::OutputFormat::KeepInputImageFormat)
		return true;
	switch(outputFormat) {
	// case uimg::TextureInfo::OutputFormat::DXT1n:
	case uimg::TextureInfo::OutputFormat::BC1:
	case uimg::TextureInfo::OutputFormat::BC1a:
		return imgFormat == prosper::Format::BC1_RGBA_SRGB_Block || imgFormat == prosper::Format::BC1_RGBA_UNorm_Block || imgFormat == prosper::Format::BC1_RGB_SRGB_Block || imgFormat == prosper::Format::BC1_RGB_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC2:
		return imgFormat == prosper::Format::BC2_SRGB_Block || imgFormat == prosper::Format::BC2_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC3:
	case uimg::TextureInfo::OutputFormat::BC3n:
	// case uimg::TextureInfo::OutputFormat::BC3_RGBM:
	//	return imgFormat == prosper::Format::BC3_SRGB_Block || imgFormat == prosper::Format::BC3_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC4:
		return imgFormat == prosper::Format::BC4_SNorm_Block || imgFormat == prosper::Format::BC4_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC5:
		return imgFormat == prosper::Format::BC5_SNorm_Block || imgFormat == prosper::Format::BC5_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::BC6:
		return imgFormat == prosper::Format::BC6H_SFloat_Block || imgFormat == prosper::Format::BC6H_UFloat_Block;
	case uimg::TextureInfo::OutputFormat::BC7:
		return imgFormat == prosper::Format::BC7_SRGB_Block || imgFormat == prosper::Format::BC7_UNorm_Block;
#if 0 // Disabled due to poor coverage
	case uimg::TextureInfo::OutputFormat::ETC2_R:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RG:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGB:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGBA:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGB_A1:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
	case uimg::TextureInfo::OutputFormat::ETC2_RGBM:
		return imgFormat == prosper::Format::ETC2_R8G8B8A1_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A1_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8A8_UNorm_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_SRGB_Block ||
			imgFormat == prosper::Format::ETC2_R8G8B8_UNorm_Block;
#endif
	case uimg::TextureInfo::OutputFormat::RGB:
	case uimg::TextureInfo::OutputFormat::RGBA:
		return prosper::util::is_uncompressed_format(static_cast<prosper::Format>(imgFormat));
	case uimg::TextureInfo::OutputFormat::CTX1:
	case uimg::TextureInfo::OutputFormat::ETC1:
	default:
		return false;
	}
	return false;
}
static bool is_compatible(prosper::Format imgFormat, prosper::Format outputFormat)
{
	switch(outputFormat) {
	case prosper::Format::BC1_RGBA_SRGB_Block:
	case prosper::Format::BC1_RGBA_UNorm_Block:
	case prosper::Format::BC1_RGB_SRGB_Block:
	case prosper::Format::BC1_RGB_UNorm_Block:
		return imgFormat == prosper::Format::BC1_RGBA_SRGB_Block || imgFormat == prosper::Format::BC1_RGBA_UNorm_Block || imgFormat == prosper::Format::BC1_RGB_SRGB_Block || imgFormat == prosper::Format::BC1_RGB_UNorm_Block;
	case prosper::Format::BC2_SRGB_Block:
	case prosper::Format::BC2_UNorm_Block:
		return imgFormat == prosper::Format::BC2_SRGB_Block || imgFormat == prosper::Format::BC2_UNorm_Block;
	case prosper::Format::BC3_SRGB_Block:
	case prosper::Format::BC3_UNorm_Block:
		return imgFormat == prosper::Format::BC3_SRGB_Block || imgFormat == prosper::Format::BC3_UNorm_Block;
	case prosper::Format::BC4_SNorm_Block:
	case prosper::Format::BC4_UNorm_Block:
		return imgFormat == prosper::Format::BC4_SNorm_Block || imgFormat == prosper::Format::BC4_UNorm_Block;
	case prosper::Format::BC5_SNorm_Block:
	case prosper::Format::BC5_UNorm_Block:
		return imgFormat == prosper::Format::BC5_SNorm_Block || imgFormat == prosper::Format::BC5_UNorm_Block;
	case prosper::Format::BC6H_SFloat_Block:
	case prosper::Format::BC6H_UFloat_Block:
		return imgFormat == prosper::Format::BC6H_SFloat_Block || imgFormat == prosper::Format::BC6H_UFloat_Block;
	case prosper::Format::BC7_SRGB_Block:
	case prosper::Format::BC7_UNorm_Block:
		return imgFormat == prosper::Format::BC7_SRGB_Block || imgFormat == prosper::Format::BC7_UNorm_Block;
	default:
		return prosper::util::is_uncompressed_format(imgFormat);
	}
	return false;
}
std::function<const uint8_t *(uint32_t, uint32_t, std::function<void(void)> &)> prosper::util::image_to_data(prosper::IImage &image, const std::optional<prosper::Format> &pdstFormat)
{
	auto dstFormat = pdstFormat;
	if(dstFormat.has_value() && *dstFormat == image.GetFormat())
		dstFormat = {};
	std::shared_ptr<prosper::IImage> imgRead = image.shared_from_this();
	std::shared_ptr<prosper::IBuffer> buf = nullptr;

	if(!dstFormat.has_value() || is_compressed_format(imgRead->GetFormat())) {
		std::optional<prosper::Format> conversionFormat {};
		if(dstFormat.has_value() && is_compatible(imgRead->GetFormat(), *dstFormat) == false) {
			// Convert the image into the target format
			auto &context = image.GetContext();

			uint32_t famIdx;
			auto setupCmd = context.AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType::Universal, famIdx);
			setupCmd->StartRecording();

			// Note: We should just be able to convert the image to
			// the destination format directly, but for some reason this code
			// causes issues when dealing with compressed formats. So instead,
			// we convert it to a generic color format, and then redirect the image compression
			// to the util_image library instead of gli.
			// TODO: It would be much more efficient to use gli for this, too, find out why this isn't working?
			auto copyCreateInfo = image.GetCreateInfo();
			//copyCreateInfo.format = Anvil::Format::BC1_RGBA_UNORM_BLOCK;//get_anvil_format(texInfo.outputFormat);
			copyCreateInfo.format = Format::R32G32B32A32_SFloat;
			copyCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
			copyCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
			copyCreateInfo.tiling = ImageTiling::Optimal; // Needs to be in optimal tiling because some GPUs do not support linear tiling with mipmaps
			setupCmd->RecordImageBarrier(image, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::TransferSrcOptimal);
			imgRead = image.Copy(*setupCmd, copyCreateInfo);
			setupCmd->RecordImageBarrier(image, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);
			setupCmd->RecordImageBarrier(*imgRead, ImageLayout::TransferDstOptimal, ImageLayout::ShaderReadOnlyOptimal);

			context.FlushCommandBuffer(*setupCmd);

			conversionFormat = prosper::Format::R32G32B32A32_SFloat;
		}

		// No conversion needed, just copy the image data to a buffer and save it directly
		auto extents = imgRead->GetExtents();
		auto numLayers = imgRead->GetLayerCount();
		auto numLevels = imgRead->GetMipmapCount();
		auto &context = imgRead->GetContext();
		uint32_t famIdx;
		auto setupCmd = context.AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType::Universal, famIdx);
		setupCmd->StartRecording();

		std::vector<gli_wrapper::GliTextureWrapper> gliTex;
		gliTex.reserve(numLayers);
		for(auto i = decltype(numLayers) {0u}; i < numLayers; ++i)
			gliTex.push_back(gli_wrapper::GliTextureWrapper {extents.width, extents.height, imgRead->GetFormat(), numLevels});

		prosper::util::BufferCreateInfo bufCreateInfo {};
		bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUToCPU;
		bufCreateInfo.size = gliTex[0].size() * gliTex.size();
		bufCreateInfo.usageFlags = prosper::BufferUsageFlags::TransferDstBit;
		bufCreateInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
		auto buf = context.CreateBuffer(bufCreateInfo);
		buf->SetPermanentlyMapped(true, prosper::IBuffer::MapFlags::ReadBit | prosper::IBuffer::MapFlags::WriteBit);

		setupCmd->RecordImageBarrier(*imgRead, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::TransferSrcOptimal);
		// Initialize buffer with image data
		size_t bufferOffset = 0;
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex[iLayer].size(iMipmap);
				prosper::util::BufferImageCopyInfo copyInfo {};
				copyInfo.baseArrayLayer = iLayer;
				copyInfo.bufferOffset = bufferOffset;
				copyInfo.dstImageLayout = ImageLayout::TransferSrcOptimal;
				copyInfo.imageExtent = {extents.width, extents.height};
				copyInfo.layerCount = 1;
				copyInfo.mipLevel = iMipmap;
				setupCmd->RecordCopyImageToBuffer(copyInfo, *imgRead, ImageLayout::TransferSrcOptimal, *buf);

				bufferOffset += mipmapSize;
			}
		}
		setupCmd->RecordImageBarrier(*imgRead, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);
		context.FlushCommandBuffer(*setupCmd);

		// Copy the data to a gli texture object
		bufferOffset = 0ull;
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex[iLayer].size(iMipmap);
				auto *dstData = gliTex[iLayer].data(0u, 0u /* face */, iMipmap);
				memset(dstData, 1, mipmapSize);
				buf->Read(bufferOffset, mipmapSize, dstData);
				bufferOffset += mipmapSize;
			}
		}
		if(conversionFormat.has_value()) {
			std::vector<std::vector<const void *>> layerMipmapData {};
			layerMipmapData.reserve(numLayers);
			for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
				layerMipmapData.push_back({});
				auto &mipmapData = layerMipmapData.back();
				mipmapData.reserve(numLevels);
				for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
					auto *dstData = gliTex[iLayer].data(0u, 0u /* face */, iMipmap);
					mipmapData.push_back(dstData);
				}
			}
			return [layerMipmapData](uint32_t iLayer, uint32_t iMipmap, std::function<void(void)> &outDeleter) -> const uint8_t * {
				outDeleter = nullptr;
				return static_cast<const uint8_t *>(layerMipmapData[iLayer][iMipmap]);
			};
		}
		return [gliTex = std::move(gliTex)](uint32_t iLayer, uint32_t iMipmap, std::function<void(void)> &outDeleter) -> const uint8_t * {
			outDeleter = nullptr;
			return static_cast<const uint8_t *>(gliTex[iLayer].data(0u, 0u /* face */, iMipmap));
		};
		/*auto fullFileName = uimg::get_absolute_path(fileName,texInfo.containerFormat);
		switch(texInfo.containerFormat)
		{
		case uimg::TextureInfo::ContainerFormat::DDS:
			return gli::save_dds(gliTex,fullFileName);
		case uimg::TextureInfo::ContainerFormat::KTX:
			return gli::save_ktx(gliTex,fullFileName);
		}*/
		return nullptr;
	}

	std::vector<std::vector<size_t>> layerMipmapOffsets {};
	uint32_t sizePerPixel = 0u;
	if(image.GetTiling() != ImageTiling::Linear || umath::is_flag_set(image.GetCreateInfo().memoryFeatures, prosper::MemoryFeatureFlags::HostAccessable) == false || image.GetFormat() != *dstFormat) {
		// Convert the image into the target format
		auto &context = image.GetContext();
		uint32_t famIdx;
		auto setupCmd = context.AllocatePrimaryLevelCommandBuffer(prosper::QueueFamilyType::Universal, famIdx);
		setupCmd->StartRecording();

		auto copyCreateInfo = image.GetCreateInfo();
		copyCreateInfo.format = *dstFormat;
		copyCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		copyCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
		copyCreateInfo.tiling = ImageTiling::Optimal; // Needs to be in optimal tiling because some GPUs do not support linear tiling with mipmaps
		setupCmd->RecordImageBarrier(image, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::TransferSrcOptimal);
		imgRead = image.Copy(*setupCmd, copyCreateInfo);
		setupCmd->RecordImageBarrier(image, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);

		// Copy the image data to a buffer
		uint64_t size = 0;
		uint64_t offset = 0;
		auto numLayers = image.GetLayerCount();
		auto numMipmaps = image.GetMipmapCount();
		sizePerPixel = get_pixel_size(*dstFormat);
		layerMipmapOffsets.resize(numLayers);
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			auto &mipmapOffsets = layerMipmapOffsets.at(iLayer);
			mipmapOffsets.resize(numMipmaps);
			for(auto iMipmap = decltype(numMipmaps) {0u}; iMipmap < numMipmaps; ++iMipmap) {
				mipmapOffsets.at(iMipmap) = size;

				auto extents = image.GetExtents(iMipmap);
				size += extents.width * extents.height * sizePerPixel;
			}
		}
		buf = context.AllocateTemporaryBuffer(size);
		if(buf == nullptr) {
			context.FlushCommandBuffer(*setupCmd);
			return nullptr; // Buffer allocation failed; Requested size too large?
		}
		setupCmd->RecordImageBarrier(*imgRead, ImageLayout::TransferDstOptimal, ImageLayout::TransferSrcOptimal);

		prosper::util::BufferImageCopyInfo copyInfo {};
		copyInfo.dstImageLayout = ImageLayout::TransferSrcOptimal;

		struct ImageMipmapData {
			uint32_t mipmapIndex = 0u;
			uint64_t bufferOffset = 0ull;
			uint64_t bufferSize = 0ull;
			prosper::Extent2D extents = {};
		};
		struct ImageLayerData {
			std::vector<ImageMipmapData> mipmaps = {};
			uint32_t layerIndex = 0u;
		};
		std::vector<ImageLayerData> layers = {};
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			layers.push_back({});
			auto &layerData = layers.back();
			layerData.layerIndex = iLayer;
			for(auto iMipmap = decltype(numMipmaps) {0u}; iMipmap < numMipmaps; ++iMipmap) {
				layerData.mipmaps.push_back({});
				auto &mipmapData = layerData.mipmaps.back();
				mipmapData.mipmapIndex = iMipmap;
				mipmapData.bufferOffset = layerMipmapOffsets.at(iLayer).at(iMipmap);

				auto extents = image.GetExtents(iMipmap);
				mipmapData.extents = extents;
				mipmapData.bufferSize = extents.width * extents.height * sizePerPixel;
			}
		}
		for(auto &layerData : layers) {
			for(auto &mipmapData : layerData.mipmaps) {
				copyInfo.mipLevel = mipmapData.mipmapIndex;
				copyInfo.baseArrayLayer = layerData.layerIndex;
				copyInfo.bufferOffset = mipmapData.bufferOffset;
				setupCmd->RecordCopyImageToBuffer(copyInfo, *imgRead, ImageLayout::TransferSrcOptimal, *buf);
			}
		}
		context.FlushCommandBuffer(*setupCmd);

		/*// The dds library expects the image data in RGB form, so we have to do some conversions in some cases.
		std::optional<util::Format> imgBufFormat = {};
		switch(srcFormat)
		{
		case Anvil::Format::B8G8R8_UNORM:
		case Anvil::Format::B8G8R8A8_UNORM:
		imgBufFormat = util::Format::RGBA32; // TODO: dst format
		break;
		}
		if(imgBufFormat.has_value())
		{
		for(auto &layerData : layers)
		{
		for(auto &mipmapData : layerData.mipmaps)
		{
		std::vector<uint8_t> imgData {};
		imgData.resize(mipmapData.bufferSize);
		if(buf->Read(mipmapData.bufferOffset,mipmapData.bufferSize,imgData.data()))
		{
		// Swap red and blue channels, then write the new image data back to the buffer
		auto imgBuf = util::ImageBuffer::Create(imgData.data(),mipmapData.extents.width,mipmapData.extents.height,*imgBufFormat,true);
		imgBuf->SwapChannels(util::ImageBuffer::Channel::Red,util::ImageBuffer::Channel::Blue);
		buf->Write(mipmapData.bufferOffset,mipmapData.bufferSize,imgData.data());
		}
		}
		}
		}*/
	}
	if(buf != nullptr) {
		return [imgRead, buf, layerMipmapOffsets, sizePerPixel](uint32_t iLayer, uint32_t iMipmap, std::function<void(void)> &outDeleter) -> const uint8_t * {
			auto offset = layerMipmapOffsets.at(iLayer).at(iMipmap);
			auto extents = imgRead->GetExtents(iMipmap);
			auto size = extents.width * extents.height * sizePerPixel;
			void *mappedPtr = nullptr;

			if(buf->Map(offset, sizePerPixel, prosper::IBuffer::MapFlags::None, &mappedPtr) == false)
				return nullptr;
			outDeleter = [buf]() {
				buf->Unmap(); // Note: setMipmapData copies the data, so we don't need to keep it mapped
			};
			return static_cast<uint8_t *>(mappedPtr);
		};
	}
	return [imgRead](uint32_t iLayer, uint32_t iMipmap, std::function<void(void)> &outDeleter) -> const uint8_t * {
		auto subresourceLayout = imgRead->GetSubresourceLayout(iLayer, iMipmap);
		if(subresourceLayout.has_value() == false)
			return nullptr;
		void *data;
		if(imgRead->Map(subresourceLayout->offset, subresourceLayout->size, &data) == false)
			return nullptr;
		outDeleter = [imgRead]() {
			imgRead->Unmap(); // Note: setMipmapData copies the data, so we don't need to keep it mapped
		};
		return static_cast<uint8_t *>(data);
	};
}
bool prosper::util::compress_image(prosper::IImage &image, const uimg::TextureInfo &texInfo, const uimg::TextureOutputHandler &outputHandler, const std::function<void(const std::string &)> &errorHandler)
{
	auto extents = image.GetExtents();
	auto dstFormat = image.GetFormat();

	uimg::TextureSaveInfo saveInfo {};
	saveInfo.numLayers = image.GetLayerCount();
	saveInfo.numMipmaps = image.GetMipmapCount();
	saveInfo.cubemap = image.IsCubemap();
	saveInfo.width = extents.width;
	saveInfo.height = extents.height;
	saveInfo.szPerPixel = get_pixel_size(dstFormat);
	saveInfo.texInfo = texInfo;
	return uimg::compress_texture(outputHandler, prosper::util::image_to_data(image, dstFormat), saveInfo, errorHandler);
}
bool prosper::util::compress_image(prosper::IImage &image, const uimg::TextureInfo &texInfo, std::vector<std::vector<std::vector<uint8_t>>> &outputData, const std::function<void(const std::string &)> &errorHandler)
{
	auto extents = image.GetExtents();
	auto dstFormat = image.GetFormat();

	uimg::TextureSaveInfo saveInfo {};
	saveInfo.numLayers = image.GetLayerCount();
	saveInfo.numMipmaps = image.GetMipmapCount();
	saveInfo.cubemap = image.IsCubemap();
	saveInfo.width = extents.width;
	saveInfo.height = extents.height;
	saveInfo.szPerPixel = get_pixel_size(dstFormat);
	saveInfo.texInfo = texInfo;
	return uimg::compress_texture(outputData, prosper::util::image_to_data(image, dstFormat), saveInfo, errorHandler);
}

bool prosper::util::is_packed_format(Format format)
{
	switch(format) {
	case Format::R4G4_UNorm_Pack8:
	case Format::R4G4B4A4_UNorm_Pack16:
	case Format::R5G5B5A1_UNorm_Pack16:
	case Format::R5G6B5_UNorm_Pack16:
	case Format::B5G6R5_UNorm_Pack16:
	case Format::B4G4R4A4_UNorm_Pack16:
	case Format::B5G5R5A1_UNorm_Pack16:
	case Format::B10G11R11_UFloat_Pack32:
	case Format::A1R5G5B5_UNorm_Pack16:
	case Format::A2R10G10B10_UNorm_Pack32:
	case Format::A2R10G10B10_SNorm_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_SScaled_Pack32_PoorCoverage:
	case Format::A2R10G10B10_UInt_Pack32:
	case Format::A2R10G10B10_SInt_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UNorm_Pack32:
	case Format::A8B8G8R8_SNorm_Pack32:
	case Format::A8B8G8R8_UScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_SScaled_Pack32_PoorCoverage:
	case Format::A8B8G8R8_UInt_Pack32:
	case Format::A8B8G8R8_SInt_Pack32:
	case Format::A8B8G8R8_SRGB_Pack32:
	case Format::A2B10G10R10_UNorm_Pack32:
	case Format::A2B10G10R10_SNorm_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_SScaled_Pack32_PoorCoverage:
	case Format::A2B10G10R10_UInt_Pack32:
	case Format::A2B10G10R10_SInt_Pack32_PoorCoverage:
		return true;
	}
	return false;
}

bool prosper::util::is_srgb_format(Format format)
{
	switch(format) {
	case Format::R8_SRGB:
	case Format::R8G8_SRGB_PoorCoverage:
	case Format::R8G8B8_SRGB_PoorCoverage:
	case Format::B8G8R8_SRGB_PoorCoverage:
	case Format::R8G8B8A8_SRGB:
	case Format::B8G8R8A8_SRGB:
	case Format::A8B8G8R8_SRGB_Pack32:
	case Format::BC1_RGB_SRGB_Block:
	case Format::BC1_RGBA_SRGB_Block:
	case Format::BC2_SRGB_Block:
	case Format::BC3_SRGB_Block:
	case Format::BC7_SRGB_Block:
	case Format::ETC2_R8G8B8_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A1_SRGB_Block_PoorCoverage:
	case Format::ETC2_R8G8B8A8_SRGB_Block_PoorCoverage:
	case Format::ASTC_4x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x4_SRGB_Block_PoorCoverage:
	case Format::ASTC_5x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_6x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_8x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x5_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x6_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x8_SRGB_Block_PoorCoverage:
	case Format::ASTC_10x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x10_SRGB_Block_PoorCoverage:
	case Format::ASTC_12x12_SRGB_Block_PoorCoverage:
		return true;
	}
	return false;
}

bool prosper::util::save_texture(const std::string &fileName, prosper::IImage &image, const uimg::TextureInfo &texInfo, const std::function<void(const std::string &)> &errorHandler)
{
	std::shared_ptr<prosper::IImage> imgRead = image.shared_from_this();
	auto srcFormat = image.GetFormat();
	auto dstFormat = srcFormat;
	if(texInfo.inputFormat != uimg::TextureInfo::InputFormat::KeepInputImageFormat)
		dstFormat = ::get_prosper_format(texInfo.inputFormat);

	uimg::ChannelMask channelMask {};
	if(is_packed_format(srcFormat))
		channelMask.Reverse();

	if(texInfo.outputFormat == uimg::TextureInfo::OutputFormat::KeepInputImageFormat || is_compressed_format(imgRead->GetFormat())) {
		std::optional<uimg::TextureInfo> convTexInfo = {};
		if(is_compatible(imgRead->GetFormat(), texInfo.outputFormat) == false) {
			// Convert the image into the target format
			auto &context = image.GetContext();
			auto &setupCmd = context.GetSetupCommandBuffer();

			// Note: We should just be able to convert the image to
			// the destination format directly, but for some reason this code
			// causes issues when dealing with compressed formats. So instead,
			// we convert it to a generic color format, and then redirect the image compression
			// to the util_image library instead of gli.
			// TODO: It would be much more efficient to use gli for this, too, find out why this isn't working?
			auto copyCreateInfo = image.GetCreateInfo();
			//copyCreateInfo.format = Anvil::Format::BC1_RGBA_UNORM_BLOCK;//get_anvil_format(texInfo.outputFormat);
			copyCreateInfo.format = Format::R32G32B32A32_SFloat;
			copyCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
			copyCreateInfo.postCreateLayout = ImageLayout::TransferDstOptimal;
			copyCreateInfo.tiling = ImageTiling::Optimal; // Needs to be in optimal tiling because some GPUs do not support linear tiling with mipmaps
			setupCmd->RecordImageBarrier(image, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::TransferSrcOptimal);
			imgRead = image.Copy(*setupCmd, copyCreateInfo);
			setupCmd->RecordImageBarrier(image, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);
			setupCmd->RecordImageBarrier(*imgRead, ImageLayout::TransferDstOptimal, ImageLayout::ShaderReadOnlyOptimal);
			context.FlushSetupCommandBuffer();

			convTexInfo = texInfo;
			convTexInfo->inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
		}

		// No conversion needed, just copy the image data to a buffer and save it directly
		auto extents = imgRead->GetExtents();
		auto numLayers = imgRead->GetLayerCount();
		auto numLevels = imgRead->GetMipmapCount();
		auto &context = imgRead->GetContext();
		auto &setupCmd = context.GetSetupCommandBuffer();

		gli_wrapper::GliTextureWrapper gliTex {extents.width, extents.height, imgRead->GetFormat(), numLevels};

		prosper::util::BufferCreateInfo bufCreateInfo {};
		bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUToCPU;
		bufCreateInfo.size = gliTex.size();
		bufCreateInfo.usageFlags = prosper::BufferUsageFlags::TransferDstBit;
		bufCreateInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
		auto buf = context.CreateBuffer(bufCreateInfo);
		buf->SetPermanentlyMapped(true, prosper::IBuffer::MapFlags::ReadBit | prosper::IBuffer::MapFlags::WriteBit);

		setupCmd->RecordImageBarrier(*imgRead, ImageLayout::ShaderReadOnlyOptimal, ImageLayout::TransferSrcOptimal);
		// Initialize buffer with image data
		size_t bufferOffset = 0;
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex.size(iMipmap);
				prosper::util::BufferImageCopyInfo copyInfo {};
				copyInfo.baseArrayLayer = iLayer;
				copyInfo.bufferOffset = bufferOffset;
				copyInfo.dstImageLayout = ImageLayout::TransferSrcOptimal;
				copyInfo.imageExtent = {extents.width, extents.height};
				copyInfo.layerCount = 1;
				copyInfo.mipLevel = iMipmap;
				setupCmd->RecordCopyImageToBuffer(copyInfo, *imgRead, ImageLayout::TransferSrcOptimal, *buf);

				bufferOffset += mipmapSize;
			}
		}
		setupCmd->RecordImageBarrier(*imgRead, ImageLayout::TransferSrcOptimal, ImageLayout::ShaderReadOnlyOptimal);
		context.FlushSetupCommandBuffer();

		// Copy the data to a gli texture object
		bufferOffset = 0ull;
		for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
			for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
				auto extents = imgRead->GetExtents(iMipmap);
				auto mipmapSize = gliTex.size(iMipmap);
				auto *dstData = gliTex.data(iLayer, 0u /* face */, iMipmap);
				memset(dstData, 1, mipmapSize);
				buf->Read(bufferOffset, mipmapSize, dstData);
				bufferOffset += mipmapSize;
			}
		}
		if(convTexInfo.has_value()) {
			std::vector<std::vector<const void *>> layerMipmapData {};
			layerMipmapData.reserve(numLayers);
			for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
				layerMipmapData.push_back({});
				auto &mipmapData = layerMipmapData.back();
				mipmapData.reserve(numLevels);
				for(auto iMipmap = decltype(numLevels) {0u}; iMipmap < numLevels; ++iMipmap) {
					auto *dstData = gliTex.data(iLayer, 0u /* face */, iMipmap);
					mipmapData.push_back(dstData);
				}
			}
			uimg::TextureSaveInfo saveInfo {};
			saveInfo.width = extents.width;
			saveInfo.height = extents.height;
			saveInfo.szPerPixel = sizeof(float) * 4;
			saveInfo.texInfo = *convTexInfo;
			saveInfo.channelMask = channelMask;
			return uimg::save_texture(fileName, layerMipmapData, saveInfo);
		}
		auto fullFileName = uimg::get_absolute_path(fileName, texInfo.containerFormat);
		auto success = false;
		switch(texInfo.containerFormat) {
		case uimg::TextureInfo::ContainerFormat::DDS:
			success = gliTex.save_dds(fullFileName);
			break;
		case uimg::TextureInfo::ContainerFormat::KTX:
			success = gliTex.save_ktx(fullFileName);
			break;
		}
		if(success)
			filemanager::update_file_index_cache(fullFileName, true);
		return success;
	}
	auto cubemap = imgRead->IsCubemap();
	auto extents = imgRead->GetExtents();

	uimg::TextureSaveInfo saveInfo {};
	saveInfo.width = extents.width;
	saveInfo.height = extents.height;
	saveInfo.numLayers = imgRead->GetLayerCount();
	saveInfo.numMipmaps = imgRead->GetMipmapCount();
	saveInfo.szPerPixel = get_pixel_size(dstFormat);
	saveInfo.texInfo = texInfo;
	saveInfo.cubemap = cubemap;
	saveInfo.channelMask = channelMask;
	return uimg::save_texture(fileName, prosper::util::image_to_data(*imgRead, dstFormat), saveInfo, errorHandler);
}
