/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_IMAGE_HPP__
#define __PR_PROSPER_VK_IMAGE_HPP__

#include "image/prosper_image.hpp"

namespace prosper
{
	class DLLPROSPER VlkImage
		: public IImage
	{
	public:
		static std::shared_ptr<VlkImage> Create(
			Context &context,std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>> img,const util::ImageCreateInfo &createInfo,
			bool isSwapchainImage,const std::function<void(VlkImage&)> &onDestroyedCallback=nullptr
		);
		virtual ~VlkImage() override;
		Anvil::Image &GetAnvilImage() const;
		Anvil::Image &operator*();
		const Anvil::Image &operator*() const;
		Anvil::Image *operator->();
		const Anvil::Image *operator->() const;

		virtual bool Map(DeviceSize offset,DeviceSize size,void **outPtr=nullptr) override;
		virtual std::optional<util::SubresourceLayout> GetSubresourceLayout(uint32_t layerId=0,uint32_t mipMapIdx=0) override;
	protected:
		VlkImage(Context &context,std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>> img,const util::ImageCreateInfo &createInfo,bool isSwapchainImage);
		virtual bool DoSetMemoryBuffer(IBuffer &buffer) override;
		std::unique_ptr<Anvil::Image,std::function<void(Anvil::Image*)>> m_image = nullptr;
		bool m_swapchainImage = false;
	};
};

#endif
