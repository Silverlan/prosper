/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_IMAGE_HPP__
#define __PROSPER_IMAGE_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"
#include "prosper_enums.hpp"
#include <wrappers/image.h>
#include <wrappers/image_view.h>
#include <wrappers/sampler.h>
#include <optional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class ICommandBuffer;
	class IBuffer;
	namespace util {struct ImageCreateInfo;};
	class DLLPROSPER IImage
		: public ContextObject,
		public std::enable_shared_from_this<IImage>
	{
	public:
		IImage(const IImage&)=delete;
		IImage &operator=(const IImage&)=delete;
		virtual ~IImage() override;

		ImageType GetType() const;
		bool IsCubemap() const;
		ImageUsageFlags GetUsageFlags() const;
		uint32_t GetLayerCount() const;
		ImageTiling GetTiling() const;
		Format GetFormat() const;
		SampleCountFlags GetSampleCount() const;
		Extent2D GetExtents(uint32_t mipLevel=0u) const;
		uint32_t GetWidth(uint32_t mipLevel=0u) const;
		uint32_t GetHeight(uint32_t mipLevel=0u) const;
		uint32_t GetMipmapCount() const;
		SharingMode GetSharingMode() const;
		ImageAspectFlags GetAspectFlags() const;
		virtual std::optional<util::SubresourceLayout> GetSubresourceLayout(uint32_t layerId=0,uint32_t mipMapIdx=0)=0;
		const util::ImageCreateInfo &GetCreateInfo() const;
		const prosper::IBuffer *GetMemoryBuffer() const;
		prosper::IBuffer *GetMemoryBuffer();
		bool SetMemoryBuffer(IBuffer &buffer);
		DeviceSize GetAlignment() const;

		virtual bool Map(DeviceSize offset,DeviceSize size,void **outPtr=nullptr)=0;
		std::shared_ptr<IImage> Copy(prosper::ICommandBuffer &cmd,const util::ImageCreateInfo &copyCreateInfo);
	protected:
		IImage(Context &context,const util::ImageCreateInfo &createInfo);
		virtual bool DoSetMemoryBuffer(IBuffer &buffer)=0;
		std::shared_ptr<prosper::IBuffer> m_buffer = nullptr; // Optional buffer
		util::ImageCreateInfo m_createInfo {};
	};
};
#pragma warning(pop)

#endif
