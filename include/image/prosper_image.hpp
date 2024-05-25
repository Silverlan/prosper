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
#include <optional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)

namespace uimg {
	enum class Format : uint8_t;
	class ImageBuffer;
};

namespace prosper {
	class ICommandBuffer;
	class IBuffer;
	namespace util {
		struct ImageCreateInfo;
	};
	class DLLPROSPER IImage : public ContextObject, public std::enable_shared_from_this<IImage> {
	  public:
		IImage(const IImage &) = delete;
		IImage &operator=(const IImage &) = delete;
		virtual ~IImage() override;

		ImageType GetType() const;
		bool IsCubemap() const;
		ImageUsageFlags GetUsageFlags() const;
		uint32_t GetLayerCount() const;
		ImageTiling GetTiling() const;
		Format GetFormat() const;
		SampleCountFlags GetSampleCount() const;
		Extent2D GetExtents(uint32_t mipLevel = 0u) const;
		uint32_t GetWidth(uint32_t mipLevel = 0u) const;
		uint32_t GetHeight(uint32_t mipLevel = 0u) const;
		uint32_t GetMipmapCount() const;
		SharingMode GetSharingMode() const;
		ImageAspectFlags GetAspectFlags() const;
		virtual std::optional<util::SubresourceLayout> GetSubresourceLayout(uint32_t layerId = 0, uint32_t mipMapIdx = 0) = 0;
		const util::ImageCreateInfo &GetCreateInfo() const;
		util::ImageCreateInfo &GetCreateInfo();
		const prosper::IBuffer *GetMemoryBuffer() const;
		prosper::IBuffer *GetMemoryBuffer();
		bool SetMemoryBuffer(IBuffer &buffer);
		virtual DeviceSize GetAlignment() const = 0;
		virtual const void *GetInternalHandle() const = 0;
		prosper::FeatureSupport AreFormatFeaturesSupported(FormatFeatureFlags featureFlags) const;

		bool IsSrgb() const;
		bool IsNormalMap() const;
		void SetSrgb(bool srgb);
		void SetNormalMap(bool normalMap);
		virtual std::optional<size_t> GetStorageSize() const = 0;

		uint32_t GetPixelSize() const;
		uint32_t GetSize() const;
		uint32_t GetSize(uint32_t mipmap) const;

		virtual void Bake() {};

		virtual bool WriteImageData(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t layerIndex, uint32_t mipLevel, uint64_t size, const uint8_t *data) = 0;
		virtual bool Map(DeviceSize offset, DeviceSize size, void **outPtr = nullptr) = 0;
		virtual bool Unmap() = 0;
		std::shared_ptr<uimg::ImageBuffer> ToHostImageBuffer(uimg::Format format, prosper::ImageLayout curImgLayout) const;
		std::shared_ptr<IImage> Copy(prosper::ICommandBuffer &cmd, const util::ImageCreateInfo &copyCreateInfo);
		bool Copy(prosper::ICommandBuffer &cmd, IImage &imgDst);
		std::shared_ptr<IImage> Convert(prosper::ICommandBuffer &cmd, Format newFormat);
	  protected:
		IImage(IPrContext &context, const util::ImageCreateInfo &createInfo);
		virtual bool DoSetMemoryBuffer(IBuffer &buffer) = 0;
		std::shared_ptr<prosper::IBuffer> m_buffer = nullptr; // Optional buffer
		util::ImageCreateInfo m_createInfo {};
	};
};
#pragma warning(pop)

#endif
