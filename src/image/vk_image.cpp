/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_includes.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include "stdafx_prosper.h"
#include "vk_image.hpp"
#include "buffers/vk_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "debug/prosper_debug.hpp"
#include <wrappers/image.h>
#include <wrappers/memory_block.h>
#include <misc/image_create_info.h>

using namespace prosper;

static_assert(sizeof(prosper::Extent2D) == sizeof(vk::Extent2D));

static std::unique_ptr<std::unordered_map<Anvil::Image*,VlkImage*>> s_imageMap = nullptr;
VlkImage *prosper::debug::get_image_from_anvil_image(Anvil::Image &img)
{
	if(s_imageMap == nullptr)
		return nullptr;
	auto it = s_imageMap->find(&img);
	return (it != s_imageMap->end()) ? it->second : nullptr;
}
std::shared_ptr<VlkImage> VlkImage::Create(
	IPrContext &context,std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>> img,const prosper::util::ImageCreateInfo &createInfo,bool isSwapchainImage,
	const std::function<void(VlkImage&)> &onDestroyedCallback
)
{
	if(img == nullptr)
		return nullptr;
	return std::shared_ptr<VlkImage>(new VlkImage{context,std::move(img),createInfo,isSwapchainImage},[onDestroyedCallback](VlkImage *img) {
		img->OnRelease();
		if(onDestroyedCallback != nullptr)
			onDestroyedCallback(*img);
		img->GetContext().ReleaseResource<VlkImage>(img);
		});
}
VlkImage::VlkImage(IPrContext &context,std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>> img,const prosper::util::ImageCreateInfo &createInfo,bool isSwapchainImage)
	: IImage{context,createInfo},m_image{std::move(img)},m_swapchainImage{isSwapchainImage}
{
	if(m_swapchainImage)
		return;
	if(prosper::debug::is_debug_mode_enabled())
	{
		if(s_imageMap == nullptr)
			s_imageMap = std::make_unique<std::unordered_map<Anvil::Image*,VlkImage*>>();
		(*s_imageMap)[m_image.get()] = this;
	}
	prosper::debug::register_debug_object(m_image->get_image(),this,prosper::debug::ObjectType::Image);
}
VlkImage::~VlkImage()
{
	if(m_swapchainImage)
		return;
	if(s_imageMap != nullptr)
	{
		auto it = s_imageMap->find(m_image.get());
		if(it != s_imageMap->end())
			s_imageMap->erase(it);
	}
	prosper::debug::deregister_debug_object(m_image->get_image());
}

DeviceSize VlkImage::GetAlignment() const {return m_image->get_image_alignment(0);}

bool VlkImage::Map(DeviceSize offset,DeviceSize size,void **outPtr)
{
	return m_image->get_memory_block()->map(offset,size,outPtr);
}

std::optional<prosper::util::SubresourceLayout> VlkImage::GetSubresourceLayout(uint32_t layerId,uint32_t mipMapIdx)
{
	static_assert(sizeof(prosper::util::SubresourceLayout) == sizeof(Anvil::SubresourceLayout));
	Anvil::SubresourceLayout subresourceLayout;
	if((*this)->get_aspect_subresource_layout(Anvil::ImageAspectFlagBits::COLOR_BIT,layerId,mipMapIdx,&subresourceLayout) == false)
		return std::optional<prosper::util::SubresourceLayout>{};
	return reinterpret_cast<prosper::util::SubresourceLayout&>(subresourceLayout);
}

bool VlkImage::DoSetMemoryBuffer(prosper::IBuffer &buffer)
{
	return m_image->set_memory(dynamic_cast<VlkBuffer&>(buffer).GetAnvilBuffer().get_memory_block(0));
}

Anvil::Image &VlkImage::GetAnvilImage() const {return *m_image;}
Anvil::Image &VlkImage::operator*() {return *m_image;}
const Anvil::Image &VlkImage::operator*() const {return const_cast<VlkImage*>(this)->operator*();}
Anvil::Image *VlkImage::operator->() {return m_image.get();}
const Anvil::Image *VlkImage::operator->() const {return const_cast<VlkImage*>(this)->operator->();}
