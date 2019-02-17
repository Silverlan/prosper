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
	class DLLPROSPER ImageView
		: public ContextObject,
		public std::enable_shared_from_this<ImageView>
	{
	public:
		static std::shared_ptr<ImageView> Create(Context &context,std::unique_ptr<Anvil::ImageView,std::function<void(Anvil::ImageView*)>> imgView,const std::function<void(ImageView&)> &onDestroyedCallback=nullptr);
		virtual ~ImageView() override;
		Anvil::ImageView &GetAnvilImageView() const;
		Anvil::ImageView &operator*();
		const Anvil::ImageView &operator*() const;
		Anvil::ImageView *operator->();
		const Anvil::ImageView *operator->() const;

		Anvil::ImageViewType GetType() const;
		Anvil::ImageAspectFlags GetAspectMask() const;
		uint32_t GetBaseLayer() const;
		uint32_t GetLayerCount() const;
		uint32_t GetBaseMipmapLevel() const;
		uint32_t GetMipmapCount() const;
		Anvil::Format GetFormat() const;
		std::array<Anvil::ComponentSwizzle,4> GetSwizzleArray() const;
	protected:
		ImageView(Context &context,std::unique_ptr<Anvil::ImageView,std::function<void(Anvil::ImageView*)>> imgView);
		std::unique_ptr<Anvil::ImageView,std::function<void(Anvil::ImageView*)>> m_imageView = nullptr;
	};
};

#endif
