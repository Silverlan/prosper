/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_IMAGE_VIEW_HPP__
#define __PROSPER_IMAGE_VIEW_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/image_view.h>

#undef max

namespace prosper
{
	class IImage;
	namespace util {struct ImageViewCreateInfo;};
	class DLLPROSPER IImageView
		: public ContextObject,
		public std::enable_shared_from_this<IImageView>
	{
	public:
		IImageView(const IImageView&)=delete;
		IImageView &operator=(const IImageView&)=delete;
		virtual ~IImageView() override;

		ImageViewType GetType() const;
		ImageAspectFlags GetAspectMask() const;
		uint32_t GetBaseLayer() const;
		uint32_t GetLayerCount() const;
		uint32_t GetBaseMipmapLevel() const;
		uint32_t GetMipmapCount() const;
		Format GetFormat() const;
		std::array<ComponentSwizzle,4> GetSwizzleArray() const;

		const IImage &GetImage() const;
		IImage &GetImage();
	protected:
		IImageView(Context &context,IImage &img,const util::ImageViewCreateInfo &createInfo,ImageViewType type,ImageAspectFlags aspectFlags);
		std::shared_ptr<IImage> m_image = nullptr;
		util::ImageViewCreateInfo m_createInfo {};
		ImageViewType m_type = ImageViewType::e2D;
		ImageAspectFlags m_aspectFlags = ImageAspectFlags::ColorBit;
	};
};

#endif
