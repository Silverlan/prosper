/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_image_view.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/image_view_create_info.h>

using namespace prosper;

std::shared_ptr<ImageView> ImageView::Create(Context &context,Anvil::ImageViewUniquePtr imgView,const std::function<void(ImageView&)> &onDestroyedCallback)
{
	if(imgView == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<ImageView>(new ImageView(context,std::move(imgView)));
	return std::shared_ptr<ImageView>(new ImageView(context,std::move(imgView)),[onDestroyedCallback](ImageView *buf) {
		onDestroyedCallback(*buf);
		delete buf;
	});
}

ImageView::ImageView(Context &context,Anvil::ImageViewUniquePtr imgView)
	: ContextObject(context),std::enable_shared_from_this<ImageView>(),m_imageView(std::move(imgView))
{
	prosper::debug::register_debug_object(m_imageView->get_image_view(),this,prosper::debug::ObjectType::ImageView);
}

ImageView::~ImageView()
{
	prosper::debug::deregister_debug_object(m_imageView->get_image_view());
}

Anvil::ImageView &ImageView::GetAnvilImageView() const {return *m_imageView;}
Anvil::ImageView &ImageView::operator*() {return *m_imageView;}
const Anvil::ImageView &ImageView::operator*() const {return const_cast<ImageView*>(this)->operator*();}
Anvil::ImageView *ImageView::operator->() {return m_imageView.get();}
const Anvil::ImageView *ImageView::operator->() const {return const_cast<ImageView*>(this)->operator->();}

Anvil::ImageViewType ImageView::GetType() const {return m_imageView->get_create_info_ptr()->get_type();}
Anvil::ImageAspectFlags ImageView::GetAspectMask() const {return m_imageView->get_create_info_ptr()->get_aspect();}
uint32_t ImageView::GetBaseLayer() const {return m_imageView->get_create_info_ptr()->get_base_layer();}
uint32_t ImageView::GetLayerCount() const {return m_imageView->get_create_info_ptr()->get_n_layers();}
uint32_t ImageView::GetBaseMipmapLevel() const {return m_imageView->get_create_info_ptr()->get_base_mipmap_level();}
uint32_t ImageView::GetMipmapCount() const {return m_imageView->get_create_info_ptr()->get_n_mipmaps();}
Anvil::Format ImageView::GetFormat() const {return m_imageView->get_create_info_ptr()->get_format();}
std::array<Anvil::ComponentSwizzle,4> ImageView::GetSwizzleArray() const
{
	auto &swizzle = m_imageView->get_create_info_ptr()->get_swizzle_array();
	return reinterpret_cast<const std::array<Anvil::ComponentSwizzle,4>&>(swizzle);
}
