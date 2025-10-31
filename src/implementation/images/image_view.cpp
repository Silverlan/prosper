// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;



module pragma.prosper;

import :image.image_view;

using namespace prosper;

IImageView::IImageView(IPrContext &context, IImage &img, const prosper::util::ImageViewCreateInfo &createInfo, ImageViewType type, ImageAspectFlags aspectFlags)
    : ContextObject(context), std::enable_shared_from_this<IImageView>(), m_image {img.shared_from_this()}, m_type {type}, m_aspectFlags {aspectFlags}, m_createInfo {createInfo}
{
	if(m_createInfo.mipmapLevels == std::numeric_limits<decltype(m_createInfo.mipmapLevels)>::max())
		m_createInfo.mipmapLevels = m_image->GetMipmapCount();
}
IImageView::~IImageView() {}

const IImage &IImageView::GetImage() const { return const_cast<IImageView *>(this)->GetImage(); }
IImage &IImageView::GetImage() { return *m_image; }

ImageViewType IImageView::GetType() const { return m_type; }
ImageAspectFlags IImageView::GetAspectMask() const { return m_aspectFlags; }
uint32_t IImageView::GetBaseLayer() const { return m_createInfo.baseLayer.has_value() ? *m_createInfo.baseLayer : 0; }
uint32_t IImageView::GetLayerCount() const { return m_createInfo.levelCount; }
uint32_t IImageView::GetBaseMipmapLevel() const { return m_createInfo.baseMipmap; }
uint32_t IImageView::GetMipmapCount() const { return m_createInfo.mipmapLevels; }
Format IImageView::GetFormat() const { return m_createInfo.format; }
std::array<ComponentSwizzle, 4> IImageView::GetSwizzleArray() const { return {m_createInfo.swizzleRed, m_createInfo.swizzleGreen, m_createInfo.swizzleBlue, m_createInfo.swizzleAlpha}; }
