// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.prosper:image.texture;

export import :image.image;

export {
#pragma warning(push)
#pragma warning(disable : 4251)
	namespace prosper {
		class IPrContext;
		class Texture;
		class ISampler;
		class IImageView;
		class DLLPROSPER Texture : public ContextObject, public std::enable_shared_from_this<Texture> {
		  public:
			Texture(const Texture &) = delete;
			Texture &operator=(const Texture &) = delete;

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

			void Bake();

			virtual bool IsMSAATexture() const;
			virtual void SetDebugName(const std::string &name) override;
		  protected:
			friend IPrContext;
			Texture(IPrContext &context, IImage &img, const std::vector<std::shared_ptr<IImageView>> &imgViews, ISampler *sampler = nullptr);
			std::shared_ptr<IImage> m_image = nullptr;
			std::vector<std::shared_ptr<IImageView>> m_imageViews = {};
			std::shared_ptr<ISampler> m_sampler = nullptr;
		};
	};
#pragma warning(pop)
}
