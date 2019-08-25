/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_TEXTURE_HPP__
#define __PROSPER_TEXTURE_HPP__

#include "image/prosper_image.hpp"
#include <mathutil/umath.h>

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Texture;
	class Sampler;
	class ImageView;
	namespace util
	{
		struct ImageViewCreateInfo;
		struct SamplerCreateInfo;
		struct DLLPROSPER TextureCreateInfo
		{
			TextureCreateInfo(const std::shared_ptr<ImageView> &imgView,const std::shared_ptr<Sampler> &smp=nullptr);
			TextureCreateInfo();
			std::shared_ptr<Sampler> sampler = nullptr;
			std::shared_ptr<ImageView> imageView = nullptr;

			enum class Flags : uint32_t
			{
				None = 0u,
				Resolvable = 1u, // If this flag is set, a MSAATexture instead of a regular texture will be created, unless the image has a sample count of 1
				CreateImageViewForEachLayer = Resolvable<<1u
			};
			Flags flags = Flags::None;
		};

		DLLPROSPER std::shared_ptr<Texture> create_texture(
			Anvil::BaseDevice &dev,const TextureCreateInfo &createInfo,const std::shared_ptr<Image> &img,
			const ImageViewCreateInfo *imageViewCreateInfo=nullptr,const SamplerCreateInfo *samplerCreateInfo=nullptr
		);
	};

	class DLLPROSPER Texture
		: public ContextObject,
		public std::enable_shared_from_this<Texture>
	{
	public:
		const std::shared_ptr<Image> &GetImage() const;
		const std::shared_ptr<ImageView> &GetImageView(uint32_t layerId) const;
		const std::shared_ptr<ImageView> &GetImageView() const;
		const std::shared_ptr<Sampler> &GetSampler() const;
		void SetSampler(Sampler &sampler);

		virtual bool IsMSAATexture() const;
		virtual void SetDebugName(const std::string &name) override;
	protected:
		friend std::shared_ptr<Texture> util::create_texture(
			Anvil::BaseDevice &dev,const util::TextureCreateInfo &createInfo,const std::shared_ptr<Image> &img,
			const util::ImageViewCreateInfo *imageViewCreateInfo,const util::SamplerCreateInfo *samplerCreateInfo
		);

		Texture(Context &context,const std::shared_ptr<Image> &img,const std::vector<std::shared_ptr<ImageView>> &imgViews,const std::shared_ptr<Sampler> &sampler);
		std::shared_ptr<Image> m_image = nullptr;
		std::vector<std::shared_ptr<ImageView>> m_imageViews = {};
		std::shared_ptr<Sampler> m_sampler = nullptr;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(prosper::util::TextureCreateInfo::Flags);
#pragma warning(pop)

#endif
