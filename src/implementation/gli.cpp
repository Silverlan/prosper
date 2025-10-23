// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>
#include <string>

module pragma.prosper;

import :gli;
import gli;

namespace gli_wrapper {
	struct GliTexture {
		gli::extent2d extents;
		gli::texture2d gliTex;
	};
};

gli_wrapper::GliTextureWrapper::GliTextureWrapper(uint32_t width, uint32_t height, prosper::Format format, uint32_t numLevels)
{
	texture = std::make_unique<GliTexture>();
	texture->extents = gli::extent2d {width, height};
	texture->gliTex = gli::texture2d {static_cast<gli::texture::format_type>(format), texture->extents, numLevels};
}

gli_wrapper::GliTextureWrapper::~GliTextureWrapper() {}

size_t gli_wrapper::GliTextureWrapper::size() const { return texture->gliTex.size(); }
size_t gli_wrapper::GliTextureWrapper::size(uint32_t imimap) const { return texture->gliTex.size(imimap); }
void *gli_wrapper::GliTextureWrapper::data(uint32_t layer, uint32_t face, uint32_t mipmap) { return texture->gliTex.data(layer, face, mipmap); }
const void *gli_wrapper::GliTextureWrapper::data(uint32_t layer, uint32_t face, uint32_t mipmap) const { return const_cast<GliTextureWrapper *>(this)->data(layer, face, mipmap); }

bool gli_wrapper::GliTextureWrapper::save_dds(const std::string &fileName) const { return gli::save_dds(texture->gliTex, fileName); }
bool gli_wrapper::GliTextureWrapper::save_ktx(const std::string &fileName) const { return gli::save_ktx(texture->gliTex, fileName); }

uint32_t gli_wrapper::get_block_size(prosper::Format format) { return gli::block_size(static_cast<gli::texture::format_type>(format)); }
uint32_t gli_wrapper::get_component_count(prosper::Format format) { return gli::component_count(static_cast<gli::texture::format_type>(format)); }
