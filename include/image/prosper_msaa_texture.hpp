// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_MSAA_TEXTURE_HPP__
#define __PROSPER_MSAA_TEXTURE_HPP__

#include "image/prosper_texture.hpp"

namespace prosper {
	class DLLPROSPER MSAATexture : public Texture {
	  public:
		const std::shared_ptr<Texture> &GetResolvedTexture() const;

		virtual bool IsMSAATexture() const override;
		std::shared_ptr<Texture> Resolve(prosper::ICommandBuffer &cmdBuffer, ImageLayout msaaLayoutIn, ImageLayout msaaLayoutOut, ImageLayout resolvedLayoutIn, ImageLayout resolvedLayoutOut);

		// Resets resolved flag
		void Reset();
		virtual void SetDebugName(const std::string &name) override;
	  protected:
		friend std::shared_ptr<Texture> IPrContext::CreateTexture(const util::TextureCreateInfo &createInfo, IImage &img, const std::optional<util::ImageViewCreateInfo> &imageViewCreateInfo, const std::optional<util::SamplerCreateInfo> &samplerCreateInfo);

		MSAATexture(IPrContext &context, IImage &img, const std::vector<std::shared_ptr<IImageView>> &imgViews, ISampler *sampler, const std::shared_ptr<Texture> &resolvedTexture);
		std::shared_ptr<Texture> m_resolvedTexture = nullptr;
		bool m_bResolved = false;
	};
};

#endif
