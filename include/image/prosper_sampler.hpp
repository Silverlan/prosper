/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_SAMPLER_HPP__
#define __PROSPER_SAMPLER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <functional>

namespace Anvil
{
	class Sampler;
};

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Sampler;
	namespace util
	{
		struct DLLPROSPER SamplerCreateInfo
		{
			Anvil::Filter minFilter = Anvil::Filter::LINEAR;
			Anvil::Filter magFilter = Anvil::Filter::LINEAR;
			Anvil::SamplerMipmapMode mipmapMode = Anvil::SamplerMipmapMode::LINEAR;
			Anvil::SamplerAddressMode addressModeU = Anvil::SamplerAddressMode::REPEAT;
			Anvil::SamplerAddressMode addressModeV = Anvil::SamplerAddressMode::REPEAT;
			Anvil::SamplerAddressMode addressModeW = Anvil::SamplerAddressMode::REPEAT;
			float mipLodBias = 0.f;
			float maxAnisotropy = std::numeric_limits<float>::max();
			bool compareEnable = false;
			Anvil::CompareOp compareOp = Anvil::CompareOp::LESS;
			float minLod = 0.f;
			float maxLod = std::numeric_limits<float>::max();
			Anvil::BorderColor borderColor = Anvil::BorderColor::FLOAT_TRANSPARENT_BLACK;
			bool useUnnormalizedCoordinates = false;
		};
		DLLPROSPER std::shared_ptr<Sampler> create_sampler(Anvil::BaseDevice &dev,const SamplerCreateInfo &createInfo);
	};
	class DLLPROSPER Sampler
		: public ContextObject,
		public std::enable_shared_from_this<Sampler>
	{
	public:
		static std::shared_ptr<Sampler> Create(Context &context,const util::SamplerCreateInfo &createInfo);
		virtual ~Sampler() override;
		Anvil::Sampler &GetAnvilSampler() const;
		Anvil::Sampler &operator*();
		const Anvil::Sampler &operator*() const;
		Anvil::Sampler *operator->();
		const Anvil::Sampler *operator->() const;

		void SetMinFilter(Anvil::Filter filter);
		void SetMagFilter(Anvil::Filter filter);
		void SetMipmapMode(Anvil::SamplerMipmapMode mipmapMode);
		void SetAddressModeU(Anvil::SamplerAddressMode addressMode);
		void SetAddressModeV(Anvil::SamplerAddressMode addressMode);
		void SetAddressModeW(Anvil::SamplerAddressMode addressMode);
		void SetLodBias(float bias);
		void SetMaxAnisotropy(float anisotropy);
		void SetCompareEnable(bool bEnable);
		void SetCompareOp(Anvil::CompareOp compareOp);
		void SetMinLod(float minLod);
		void SetMaxLod(float maxLod);
		void SetBorderColor(Anvil::BorderColor borderColor);
		void SetUseUnnormalizedCoordinates(bool bUseUnnormalizedCoordinates);

		Anvil::Filter GetMinFilter() const;
		Anvil::Filter GetMagFilter() const;
		Anvil::SamplerMipmapMode GetMipmapMode() const;
		Anvil::SamplerAddressMode GetAddressModeU() const;
		Anvil::SamplerAddressMode GetAddressModeV() const;
		Anvil::SamplerAddressMode GetAddressModeW() const;
		float GetLodBias() const;
		float GetMaxAnisotropy() const;
		bool GetCompareEnabled() const;
		Anvil::CompareOp GetCompareOp() const;
		float GetMinLod() const;
		float GetMaxLod() const;
		Anvil::BorderColor GetBorderColor() const;
		bool GetUseUnnormalizedCoordinates() const;

		bool Update();
	protected:
		Sampler(Context &context,const util::SamplerCreateInfo &samplerCreateInfo);
		std::unique_ptr<Anvil::Sampler,std::function<void(Anvil::Sampler*)>> m_sampler = nullptr;
		util::SamplerCreateInfo m_createInfo = {};
	};
};
#pragma warning(pop)

#endif
