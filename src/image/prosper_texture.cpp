// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "image/prosper_texture.hpp"
#include "image/prosper_msaa_texture.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_sampler.hpp"
#include "image/prosper_image_view.hpp"

using namespace prosper;

Texture::Texture(IPrContext &context, IImage &img, const std::vector<std::shared_ptr<IImageView>> &imgViews, ISampler *sampler)
    : ContextObject(context), std::enable_shared_from_this<Texture>(), m_image {img.shared_from_this()}, m_imageViews {imgViews}, m_sampler {sampler ? sampler->shared_from_this() : nullptr}
{
}

const IImage &Texture::GetImage() const { return const_cast<Texture *>(this)->GetImage(); }
IImage &Texture::GetImage() { return *m_image; }

const IImageView *Texture::GetImageView(uint32_t layerId) const { return const_cast<Texture *>(this)->GetImageView(layerId); }
IImageView *Texture::GetImageView(uint32_t layerId)
{
	++layerId;
	return (layerId < m_imageViews.size()) ? m_imageViews.at(layerId).get() : nullptr;
}
const IImageView *Texture::GetImageView() const { return const_cast<Texture *>(this)->GetImageView(); }
IImageView *Texture::GetImageView() { return m_imageViews.at(0u).get(); }
const ISampler *Texture::GetSampler() const { return const_cast<Texture *>(this)->GetSampler(); }
ISampler *Texture::GetSampler() { return m_sampler.get(); }
void Texture::SetSampler(ISampler &sampler) { m_sampler = sampler.shared_from_this(); }
void Texture::SetImageView(IImageView &imgView) { m_imageViews.at(0) = imgView.shared_from_this(); }
bool Texture::IsMSAATexture() const { return false; }
void Texture::SetDebugName(const std::string &name)
{
	ContextObject::SetDebugName(name);
	if(m_image != nullptr)
		m_image->SetDebugName(name + "_img");
	auto idx = 0u;
	for(auto &imgView : m_imageViews)
		imgView->SetDebugName(name + "_imgView" + std::to_string(idx++));
	if(m_sampler != nullptr)
		m_sampler->SetDebugName(name + "_smp");
}
void Texture::Bake()
{
	if(m_image)
		m_image->Bake();
	for(auto &imgView : m_imageViews)
		imgView->Bake();
	if(m_sampler)
		m_sampler->Bake();
}

//////////////////////////

prosper::util::TextureCreateInfo::TextureCreateInfo(IImageView &imgView, ISampler *smp) : imageView {imgView.shared_from_this()}, sampler {smp ? smp->shared_from_this() : nullptr} {}
