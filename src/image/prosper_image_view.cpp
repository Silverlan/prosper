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

IImageView::IImageView(Context &context,IImage &img,const prosper::util::ImageViewCreateInfo &createInfo,ImageViewType type,ImageAspectFlags aspectFlags)
	: ContextObject(context),std::enable_shared_from_this<IImageView>(),m_image{img.shared_from_this()},
	m_type{type},m_aspectFlags{aspectFlags},m_createInfo{createInfo}
{}
IImageView::~IImageView() {}

const IImage &IImageView::GetImage() const {return const_cast<IImageView*>(this)->GetImage();}
IImage &IImageView::GetImage() {return *m_image;}

ImageViewType IImageView::GetType() const {return m_type;}
ImageAspectFlags IImageView::GetAspectMask() const {return m_aspectFlags;}
uint32_t IImageView::GetBaseLayer() const {return m_createInfo.baseLayer.has_value() ? *m_createInfo.baseLayer : 0;}
uint32_t IImageView::GetLayerCount() const {return m_createInfo.levelCount;}
uint32_t IImageView::GetBaseMipmapLevel() const {return m_createInfo.baseMipmap;}
uint32_t IImageView::GetMipmapCount() const {return m_createInfo.mipmapLevels;}
Format IImageView::GetFormat() const {return m_createInfo.format;}
std::array<ComponentSwizzle,4> IImageView::GetSwizzleArray() const
{
	return {
		m_createInfo.swizzleRed,
		m_createInfo.swizzleGreen,
		m_createInfo.swizzleBlue,
		m_createInfo.swizzleAlpha
	};
}
