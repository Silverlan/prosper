// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_SAMPLER_HPP__
#define __PROSPER_SAMPLER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_enums.hpp"
#include "prosper_structs.hpp"
#include <functional>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	class DLLPROSPER ISampler : public ContextObject, public std::enable_shared_from_this<ISampler> {
	  public:
		ISampler(const ISampler &) = delete;
		ISampler &operator=(const ISampler &) = delete;
		virtual ~ISampler() override;
		virtual const void *GetInternalHandle() const { return nullptr; }

		void SetMinFilter(Filter filter);
		void SetMagFilter(Filter filter);
		void SetMipmapMode(SamplerMipmapMode mipmapMode);
		void SetAddressModeU(SamplerAddressMode addressMode);
		void SetAddressModeV(SamplerAddressMode addressMode);
		void SetAddressModeW(SamplerAddressMode addressMode);
		void SetLodBias(float bias);
		void SetMaxAnisotropy(float anisotropy);
		void SetCompareEnable(bool bEnable);
		void SetCompareOp(CompareOp compareOp);
		void SetMinLod(float minLod);
		void SetMaxLod(float maxLod);
		void SetBorderColor(BorderColor borderColor);
		virtual void Bake() {}
		// void SetUseUnnormalizedCoordinates(bool bUseUnnormalizedCoordinates);

		Filter GetMinFilter() const;
		Filter GetMagFilter() const;
		SamplerMipmapMode GetMipmapMode() const;
		SamplerAddressMode GetAddressModeU() const;
		SamplerAddressMode GetAddressModeV() const;
		SamplerAddressMode GetAddressModeW() const;
		float GetLodBias() const;
		float GetMaxAnisotropy() const;
		bool GetCompareEnabled() const;
		CompareOp GetCompareOp() const;
		float GetMinLod() const;
		float GetMaxLod() const;
		BorderColor GetBorderColor() const;
		// bool GetUseUnnormalizedCoordinates() const;

		bool Update();
	  protected:
		ISampler(IPrContext &context, const util::SamplerCreateInfo &samplerCreateInfo);
		virtual bool DoUpdate() = 0;
		util::SamplerCreateInfo m_createInfo = {};
	};
};
#pragma warning(pop)

#endif
