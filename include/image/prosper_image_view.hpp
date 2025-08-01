// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_IMAGE_VIEW_HPP__
#define __PROSPER_IMAGE_VIEW_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"

#undef max

namespace prosper {
	class IImage;
	namespace util {
		struct ImageViewCreateInfo;
	};
	class DLLPROSPER IImageView : public ContextObject, public std::enable_shared_from_this<IImageView> {
	  public:
		IImageView(const IImageView &) = delete;
		IImageView &operator=(const IImageView &) = delete;
		virtual ~IImageView() override;
		virtual const void *GetInternalHandle() const { return nullptr; }

		ImageViewType GetType() const;
		ImageAspectFlags GetAspectMask() const;
		uint32_t GetBaseLayer() const;
		uint32_t GetLayerCount() const;
		uint32_t GetBaseMipmapLevel() const;
		uint32_t GetMipmapCount() const;
		Format GetFormat() const;
		std::array<ComponentSwizzle, 4> GetSwizzleArray() const;
		virtual void Bake() {}

		const IImage &GetImage() const;
		IImage &GetImage();
	  protected:
		IImageView(IPrContext &context, IImage &img, const util::ImageViewCreateInfo &createInfo, ImageViewType type, ImageAspectFlags aspectFlags);
		std::shared_ptr<IImage> m_image = nullptr;
		util::ImageViewCreateInfo m_createInfo {};
		ImageViewType m_type = ImageViewType::e2D;
		ImageAspectFlags m_aspectFlags = ImageAspectFlags::ColorBit;
	};
};

#endif
