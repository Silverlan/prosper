// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"


export module pragma.prosper:image.msaa_texture;

export import :image.texture;

export namespace prosper {
	class IPrContext;
	class DLLPROSPER MSAATexture : public Texture {
	  public:
		const std::shared_ptr<Texture> &GetResolvedTexture() const;

		virtual bool IsMSAATexture() const override;
		std::shared_ptr<Texture> Resolve(prosper::ICommandBuffer &cmdBuffer, ImageLayout msaaLayoutIn, ImageLayout msaaLayoutOut, ImageLayout resolvedLayoutIn, ImageLayout resolvedLayoutOut);

		// Resets resolved flag
		void Reset();
		virtual void SetDebugName(const std::string &name) override;
	  protected:
		friend IPrContext;
		MSAATexture(IPrContext &context, IImage &img, const std::vector<std::shared_ptr<IImageView>> &imgViews, ISampler *sampler, const std::shared_ptr<Texture> &resolvedTexture);
		std::shared_ptr<Texture> m_resolvedTexture = nullptr;
		bool m_bResolved = false;
	};
};
