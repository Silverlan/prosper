/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "image/prosper_sampler.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

bool ISampler::Update() {return DoUpdate();}

ISampler::ISampler(IPrContext &context,const prosper::util::SamplerCreateInfo &samplerCreateInfo)
	: ContextObject(context),std::enable_shared_from_this<ISampler>(),
	m_createInfo(samplerCreateInfo)
{}

ISampler::~ISampler() {}

void ISampler::SetMinFilter(Filter filter) {m_createInfo.minFilter = filter;}
void ISampler::SetMagFilter(Filter filter) {m_createInfo.magFilter = filter;}
void ISampler::SetMipmapMode(SamplerMipmapMode mipmapMode) {m_createInfo.mipmapMode = mipmapMode;}
void ISampler::SetAddressModeU(SamplerAddressMode addressMode) {m_createInfo.addressModeU = addressMode;}
void ISampler::SetAddressModeV(SamplerAddressMode addressMode) {m_createInfo.addressModeV = addressMode;}
void ISampler::SetAddressModeW(SamplerAddressMode addressMode) {m_createInfo.addressModeW = addressMode;}
void ISampler::SetLodBias(float bias) {m_createInfo.mipLodBias = bias;}
void ISampler::SetMaxAnisotropy(float anisotropy) {m_createInfo.maxAnisotropy = anisotropy;}
void ISampler::SetCompareEnable(bool bEnable) {m_createInfo.compareEnable = bEnable;}
void ISampler::SetCompareOp(CompareOp compareOp) {m_createInfo.compareOp = compareOp;}
void ISampler::SetMinLod(float minLod) {m_createInfo.minLod = minLod;}
void ISampler::SetMaxLod(float maxLod) {m_createInfo.maxLod = maxLod;}
void ISampler::SetBorderColor(BorderColor borderColor) {m_createInfo.borderColor = borderColor;}
void ISampler::SetUseUnnormalizedCoordinates(bool bUseUnnormalizedCoordinates) {m_createInfo.useUnnormalizedCoordinates = bUseUnnormalizedCoordinates;}

Filter ISampler::GetMinFilter() const {return m_createInfo.minFilter;}
Filter ISampler::GetMagFilter() const {return m_createInfo.magFilter;}
SamplerMipmapMode ISampler::GetMipmapMode() const {return m_createInfo.mipmapMode;}
SamplerAddressMode ISampler::GetAddressModeU() const {return m_createInfo.addressModeU;}
SamplerAddressMode ISampler::GetAddressModeV() const {return m_createInfo.addressModeV;}
SamplerAddressMode ISampler::GetAddressModeW() const {return m_createInfo.addressModeW;}
float ISampler::GetLodBias() const {return m_createInfo.mipLodBias;}
float ISampler::GetMaxAnisotropy() const {return m_createInfo.maxAnisotropy;}
bool ISampler::GetCompareEnabled() const {return m_createInfo.compareEnable;}
CompareOp ISampler::GetCompareOp() const {return m_createInfo.compareOp;}
float ISampler::GetMinLod() const {return m_createInfo.minLod;}
float ISampler::GetMaxLod() const {return m_createInfo.maxLod;}
BorderColor ISampler::GetBorderColor() const {return m_createInfo.borderColor;}
bool ISampler::GetUseUnnormalizedCoordinates() const {return m_createInfo.useUnnormalizedCoordinates;}
