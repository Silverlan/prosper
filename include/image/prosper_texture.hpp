/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TEXTURE_HPP__
#define __PROSPER_TEXTURE_HPP__

#include "prosper_context.hpp"
#include "image/prosper_image.hpp"
#include <mathutil/umath.h>

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Texture;
	class ISampler;
	class IImageView;
	class DLLPROSPER Texture
		: public ContextObject,
		public std::enable_shared_from_this<Texture>
	{
	public:
		Texture(const Texture&)=delete;
		Texture &operator=(const Texture&)=delete;

		const IImage &GetImage() const;
		IImage &GetImage();
		const IImageView *GetImageView(uint32_t layerId) const;
		IImageView *GetImageView(uint32_t layerId);
		const IImageView *GetImageView() const;
		IImageView *GetImageView();
		const ISampler *GetSampler() const;
		ISampler *GetSampler();
		void SetSampler(ISampler &sampler);
		void SetImageView(IImageView &imgView);

		virtual bool IsMSAATexture() const;
		virtual void SetDebugName(const std::string &name) override;
	protected:
		friend std::shared_ptr<Texture> IPrContext::CreateTexture(
			const util::TextureCreateInfo &createInfo,IImage &img,
			const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo,
			const std::optional<util::SamplerCreateInfo> &samplerCreateInfo
		);

		Texture(IPrContext &context,IImage &img,const std::vector<std::shared_ptr<IImageView>> &imgViews,ISampler *sampler=nullptr);
		std::shared_ptr<IImage> m_image = nullptr;
		std::vector<std::shared_ptr<IImageView>> m_imageViews = {};
		std::shared_ptr<ISampler> m_sampler = nullptr;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::TextureCreateInfo::Flags);
#pragma warning(pop)

#endif
