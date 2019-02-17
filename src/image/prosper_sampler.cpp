/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_sampler.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <wrappers/sampler.h>
#include <misc/sampler_create_info.h>

using namespace prosper;

std::shared_ptr<Sampler> prosper::util::create_sampler(Anvil::BaseDevice &dev,const SamplerCreateInfo &createInfo)
{
	return Sampler::Create(Context::GetContext(dev),createInfo);
}

std::shared_ptr<Sampler> Sampler::Create(Context &context,const prosper::util::SamplerCreateInfo &createInfo)
{
	auto &dev = context.GetDevice();
	auto sampler = std::shared_ptr<Sampler>(new Sampler(context,createInfo));
	if(sampler->Update() == false)
		return nullptr;
	return sampler;
}

bool Sampler::Update()
{
	auto &createInfo = m_createInfo;
	auto &dev = GetDevice();
	auto maxDeviceAnisotropy = dev.get_physical_device_properties().core_vk1_0_properties_ptr->limits.max_sampler_anisotropy;
	auto anisotropy = createInfo.maxAnisotropy;
	if(anisotropy == std::numeric_limits<decltype(anisotropy)>::max() || anisotropy > maxDeviceAnisotropy)
		anisotropy = maxDeviceAnisotropy;
	auto newSampler = Anvil::Sampler::create(
		Anvil::SamplerCreateInfo::create(
			&dev,createInfo.magFilter,createInfo.minFilter,
			createInfo.mipmapMode,createInfo.addressModeU,
			createInfo.addressModeV,createInfo.addressModeW,
			createInfo.mipLodBias,anisotropy,createInfo.compareEnable,createInfo.compareOp,
			createInfo.minLod,createInfo.maxLod,createInfo.borderColor,createInfo.useUnnormalizedCoordinates
		)
	);
	if(newSampler != nullptr)
	{
		if(m_sampler != nullptr)
			prosper::debug::deregister_debug_object(m_sampler->get_sampler());
		m_sampler = std::move(newSampler);
		prosper::debug::register_debug_object(m_sampler->get_sampler(),this,prosper::debug::ObjectType::Sampler);
		return true;
	}
	return false;
}

Sampler::Sampler(Context &context,const prosper::util::SamplerCreateInfo &samplerCreateInfo)
	: ContextObject(context),std::enable_shared_from_this<Sampler>(),
	m_createInfo(samplerCreateInfo)
{}

Sampler::~Sampler()
{
	if(m_sampler != nullptr)
		prosper::debug::deregister_debug_object(m_sampler->get_sampler());
}

void Sampler::SetMinFilter(Anvil::Filter filter) {m_createInfo.minFilter = filter;}
void Sampler::SetMagFilter(Anvil::Filter filter) {m_createInfo.magFilter = filter;}
void Sampler::SetMipmapMode(Anvil::SamplerMipmapMode mipmapMode) {m_createInfo.mipmapMode = mipmapMode;}
void Sampler::SetAddressModeU(Anvil::SamplerAddressMode addressMode) {m_createInfo.addressModeU = addressMode;}
void Sampler::SetAddressModeV(Anvil::SamplerAddressMode addressMode) {m_createInfo.addressModeV = addressMode;}
void Sampler::SetAddressModeW(Anvil::SamplerAddressMode addressMode) {m_createInfo.addressModeW = addressMode;}
void Sampler::SetLodBias(float bias) {m_createInfo.mipLodBias = bias;}
void Sampler::SetMaxAnisotropy(float anisotropy) {m_createInfo.maxAnisotropy = anisotropy;}
void Sampler::SetCompareEnable(bool bEnable) {m_createInfo.compareEnable = bEnable;}
void Sampler::SetCompareOp(Anvil::CompareOp compareOp) {m_createInfo.compareOp = compareOp;}
void Sampler::SetMinLod(float minLod) {m_createInfo.minLod = minLod;}
void Sampler::SetMaxLod(float maxLod) {m_createInfo.maxLod = maxLod;}
void Sampler::SetBorderColor(Anvil::BorderColor borderColor) {m_createInfo.borderColor = borderColor;}
void Sampler::SetUseUnnormalizedCoordinates(bool bUseUnnormalizedCoordinates) {m_createInfo.useUnnormalizedCoordinates = bUseUnnormalizedCoordinates;}

Anvil::Filter Sampler::GetMinFilter() const {return m_createInfo.minFilter;}
Anvil::Filter Sampler::GetMagFilter() const {return m_createInfo.magFilter;}
Anvil::SamplerMipmapMode Sampler::GetMipmapMode() const {return m_createInfo.mipmapMode;}
Anvil::SamplerAddressMode Sampler::GetAddressModeU() const {return m_createInfo.addressModeU;}
Anvil::SamplerAddressMode Sampler::GetAddressModeV() const {return m_createInfo.addressModeV;}
Anvil::SamplerAddressMode Sampler::GetAddressModeW() const {return m_createInfo.addressModeW;}
float Sampler::GetLodBias() const {return m_createInfo.mipLodBias;}
float Sampler::GetMaxAnisotropy() const {return m_createInfo.maxAnisotropy;}
bool Sampler::GetCompareEnabled() const {return m_createInfo.compareEnable;}
Anvil::CompareOp Sampler::GetCompareOp() const {return m_createInfo.compareOp;}
float Sampler::GetMinLod() const {return m_createInfo.minLod;}
float Sampler::GetMaxLod() const {return m_createInfo.maxLod;}
Anvil::BorderColor Sampler::GetBorderColor() const {return m_createInfo.borderColor;}
bool Sampler::GetUseUnnormalizedCoordinates() const {return m_createInfo.useUnnormalizedCoordinates;}

Anvil::Sampler &Sampler::GetAnvilSampler() const {return *m_sampler;}
Anvil::Sampler &Sampler::operator*() {return *m_sampler;}
const Anvil::Sampler &Sampler::operator*() const {return const_cast<Sampler*>(this)->operator*();}
Anvil::Sampler *Sampler::operator->() {return m_sampler.get();}
const Anvil::Sampler *Sampler::operator->() const {return const_cast<Sampler*>(this)->operator->();}
