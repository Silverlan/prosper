/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_descriptor_set_group.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_image_view.hpp"
#include "image/prosper_sampler.hpp"
#include "image/vk_image_view.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "buffers/vk_buffer.hpp"

using namespace prosper;

#pragma optimize("",off)
std::shared_ptr<DescriptorSetGroup> DescriptorSetGroup::Create(Context &context,Anvil::DescriptorSetGroupUniquePtr imgView,const std::function<void(DescriptorSetGroup&)> &onDestroyedCallback)
{
	if(imgView == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<DescriptorSetGroup>(new DescriptorSetGroup(context,std::move(imgView)));
	return std::shared_ptr<DescriptorSetGroup>(new DescriptorSetGroup(context,std::move(imgView)),[onDestroyedCallback](DescriptorSetGroup *buf) {
		buf->OnRelease();
		onDestroyedCallback(*buf);
		delete buf;
	});
}

IDescriptorSetGroup::IDescriptorSetGroup(Context &context)
	: ContextObject(context),std::enable_shared_from_this<IDescriptorSetGroup>()
{}

IDescriptorSetGroup::~IDescriptorSetGroup() {}

uint32_t IDescriptorSetGroup::GetBindingCount() const {return dynamic_cast<const DescriptorSetGroup&>(*this)->get_descriptor_set_create_info()->at(0)->get_n_bindings();}//m_descriptorSets.front()->GetBindingCount();}

IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) {return (index < m_descriptorSets.size()) ? m_descriptorSets.at(index).get() : nullptr;}
const IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) const {return const_cast<IDescriptorSetGroup*>(this)->GetDescriptorSet(index);}

/////////////////

IDescriptorSet::IDescriptorSet(IDescriptorSetGroup &dsg)
	: m_dsg{dsg}
{
	auto numBindings = dsg.GetBindingCount();
	m_bindings.resize(numBindings);
}

uint32_t IDescriptorSet::GetBindingCount() const {return m_bindings.size();}

DescriptorSetBinding *IDescriptorSet::GetBinding(uint32_t bindingIndex) {return (bindingIndex < m_bindings.size()) ? m_bindings.at(bindingIndex).get() : nullptr;}
const DescriptorSetBinding *IDescriptorSet::GetBinding(uint32_t bindingIndex) const {return const_cast<IDescriptorSet*>(this)->GetBinding(bindingIndex);}

std::vector<std::unique_ptr<DescriptorSetBinding>> &IDescriptorSet::GetBindings() {return m_bindings;}
const std::vector<std::unique_ptr<DescriptorSetBinding>> &IDescriptorSet::GetBindings() const {return const_cast<IDescriptorSet*>(this)->GetBindings();}

DescriptorSetBinding &IDescriptorSet::SetBinding(uint32_t bindingIndex,std::unique_ptr<DescriptorSetBinding> binding)
{
	if(bindingIndex >= m_bindings.size())
		throw std::invalid_argument{std::to_string(bindingIndex) +" exceeds descriptor set binding count " +std::to_string(m_bindings.size()) +" of descriptor set group " +GetDescriptorSetGroup().GetDebugName()};
	m_bindings.at(bindingIndex) = std::move(binding);
	return *m_bindings.at(bindingIndex);
}

prosper::IBuffer *IDescriptorSet::GetBoundBuffer(uint32_t bindingIndex,uint64_t *outStartOffset,uint64_t *outSize)
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return nullptr;
	auto &binding = m_bindings.at(bindingIndex);
	switch(binding->GetType())
	{
	case DescriptorSetBinding::Type::UniformBuffer:
	{
		auto &bindingImg = *static_cast<DescriptorSetBindingUniformBuffer*>(binding.get());
		if(outStartOffset)
			*outStartOffset = bindingImg.GetStartOffset();
		if(outSize)
			*outSize = bindingImg.GetSize();
		return bindingImg.GetBuffer().get();
	}
	case DescriptorSetBinding::Type::DynamicUniformBuffer:
	{
		auto &bindingTex = *static_cast<DescriptorSetBindingDynamicUniformBuffer*>(binding.get());
		if(outStartOffset)
			*outStartOffset = bindingTex.GetStartOffset();
		if(outSize)
			*outSize = bindingTex.GetSize();
		return bindingTex.GetBuffer().get();
	}
	case DescriptorSetBinding::Type::StorageBuffer:
	{
		auto &bindingImg = *static_cast<DescriptorSetBindingStorageBuffer*>(binding.get());
		if(outStartOffset)
			*outStartOffset = bindingImg.GetStartOffset();
		if(outSize)
			*outSize = bindingImg.GetSize();
		return bindingImg.GetBuffer().get();
	}
	}
	return nullptr;
}

prosper::Texture *IDescriptorSet::GetBoundTexture(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex)
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return nullptr;
	auto &binding = m_bindings.at(bindingIndex);
	switch(binding->GetType())
	{
	case DescriptorSetBinding::Type::StorageImage:
	{
		auto &bindingImg = *static_cast<DescriptorSetBindingStorageImage*>(binding.get());
		if(optOutLayerIndex)
			*optOutLayerIndex = bindingImg.GetLayerIndex();
		return bindingImg.GetTexture().get();
	}
	case DescriptorSetBinding::Type::Texture:
	{
		auto &bindingTex = *static_cast<DescriptorSetBindingTexture*>(binding.get());
		if(optOutLayerIndex)
			*optOutLayerIndex = bindingTex.GetLayerIndex();
		return bindingTex.GetTexture().get();
	}
	}
	return nullptr;
}

prosper::IImage *IDescriptorSet::GetBoundImage(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex)
{
	auto *tex = GetBoundTexture(bindingIndex,optOutLayerIndex);
	return tex ? &tex->GetImage() : nullptr;
}

IDescriptorSetGroup &IDescriptorSet::GetDescriptorSetGroup() const {return m_dsg;}

/////////////////

DescriptorSetBinding::DescriptorSetBinding(IDescriptorSet &descSet,uint32_t bindingIdx)
	: m_descriptorSet{descSet},m_bindingIndex{bindingIdx}
{}
uint32_t DescriptorSetBinding::GetBindingIndex() const {return m_bindingIndex;}
IDescriptorSet &DescriptorSetBinding::GetDescriptorSet() const {return m_descriptorSet;}

///

DescriptorSetBindingStorageImage::DescriptorSetBindingStorageImage(IDescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId)
	: DescriptorSetBinding{descSet,bindingIdx},m_texture{texture.shared_from_this()},m_layerId{layerId}
{}
std::optional<uint32_t> DescriptorSetBindingStorageImage::GetLayerIndex() const {return m_layerId;}
const std::shared_ptr<prosper::Texture> &DescriptorSetBindingStorageImage::GetTexture() const {return m_texture;}

///

DescriptorSetBindingTexture::DescriptorSetBindingTexture(IDescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId)
	: DescriptorSetBinding{descSet,bindingIdx},m_texture{texture.shared_from_this()},m_layerId{layerId}
{}
std::optional<uint32_t> DescriptorSetBindingTexture::GetLayerIndex() const {return m_layerId;}
const std::shared_ptr<prosper::Texture> &DescriptorSetBindingTexture::GetTexture() const {return m_texture;}

///

DescriptorSetBindingArrayTexture::DescriptorSetBindingArrayTexture(IDescriptorSet &descSet,uint32_t bindingIdx)
	: DescriptorSetBinding{descSet,bindingIdx}
{}
void DescriptorSetBindingArrayTexture::SetArrayBinding(uint32_t arrayIndex,std::unique_ptr<DescriptorSetBindingTexture> bindingTexture)
{
	if(arrayIndex >= m_arrayItems.size())
		m_arrayItems.resize(arrayIndex +1);
	m_arrayItems.at(arrayIndex) = std::move(bindingTexture);
}

///

DescriptorSetBindingUniformBuffer::DescriptorSetBindingUniformBuffer(IDescriptorSet &descSet,uint32_t bindingIdx,prosper::IBuffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingUniformBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingUniformBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::IBuffer> &DescriptorSetBindingUniformBuffer::GetBuffer() const {return m_buffer;}

///

DescriptorSetBindingDynamicUniformBuffer::DescriptorSetBindingDynamicUniformBuffer(IDescriptorSet &descSet,uint32_t bindingIdx,prosper::IBuffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingDynamicUniformBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingDynamicUniformBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::IBuffer> &DescriptorSetBindingDynamicUniformBuffer::GetBuffer() const {return m_buffer;}

///

DescriptorSetBindingStorageBuffer::DescriptorSetBindingStorageBuffer(IDescriptorSet &descSet,uint32_t bindingIdx,prosper::IBuffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingStorageBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingStorageBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::IBuffer> &DescriptorSetBindingStorageBuffer::GetBuffer() const {return m_buffer;}

//////////////

DescriptorSetGroup::DescriptorSetGroup(Context &context,Anvil::DescriptorSetGroupUniquePtr imgView)
	: IDescriptorSetGroup{context},m_descriptorSetGroup(std::move(imgView))
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::register_debug_object(m_descriptorSetGroup->get_descriptor_set(i),this,prosper::debug::ObjectType::DescriptorSet);
	m_descriptorSets.resize(numSets);

	for(auto i=decltype(m_descriptorSets.size()){0u};i<m_descriptorSets.size();++i)
	{
		m_descriptorSets.at(i) = std::shared_ptr<DescriptorSet>{new DescriptorSet{*this,*m_descriptorSetGroup->get_descriptor_set(i)},[](DescriptorSet *ds) {
			// ds->OnRelease();
			delete ds;
		}};
	}
}
DescriptorSetGroup::~DescriptorSetGroup()
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::deregister_debug_object(m_descriptorSetGroup->get_descriptor_set(i));
}

DescriptorSet::DescriptorSet(DescriptorSetGroup &dsg,Anvil::DescriptorSet &ds)
	: IDescriptorSet{dsg},m_descSet{ds}
{}

Anvil::DescriptorSet &DescriptorSet::GetAnvilDescriptorSet() const {return m_descSet;}
Anvil::DescriptorSet &DescriptorSet::operator*() {return m_descSet;}
const Anvil::DescriptorSet &DescriptorSet::operator*() const {return const_cast<DescriptorSet*>(this)->operator*();}
Anvil::DescriptorSet *DescriptorSet::operator->() {return &m_descSet;}
const Anvil::DescriptorSet *DescriptorSet::operator->() const {return const_cast<DescriptorSet*>(this)->operator->();}

bool DescriptorSet::Update() {return m_descSet.update();}
bool DescriptorSet::SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId)
{
	auto *imgView = texture.GetImageView(layerId);
	if(imgView == nullptr)
		return false;
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingStorageImage>(*this,bindingIdx,texture,layerId));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::StorageImageBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView()
	});
}
bool DescriptorSet::SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx)
{
	auto *imgView = texture.GetImageView();
	if(imgView == nullptr)
		return false;
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingStorageImage>(*this,bindingIdx,texture));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::StorageImageBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView()
	});
}
bool DescriptorSet::SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId)
{
	auto *imgView = texture.GetImageView(layerId);
	if(imgView == nullptr && texture.GetImage().GetLayerCount() == 1)
		imgView = texture.GetImageView();
	auto *sampler = texture.GetSampler();
	if(imgView == nullptr || sampler == nullptr)
		return false;
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture,layerId));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView(),&static_cast<Sampler*>(sampler)->GetAnvilSampler()
	});
}
bool DescriptorSet::SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx)
{
	auto *imgView = texture.GetImageView();
	auto *sampler = texture.GetSampler();
	if(imgView == nullptr || sampler == nullptr)
		return false;
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView(),&static_cast<Sampler*>(sampler)->GetAnvilSampler()
	});
}
bool DescriptorSet::SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex,uint32_t layerId)
{
	std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(1u,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(texture.GetImageView(layerId))->GetAnvilImageView(),&static_cast<Sampler*>(texture.GetSampler())->GetAnvilSampler()
	});
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingArrayTexture>(*this,bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture*>(binding)->SetArrayBinding(arrayIndex,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture,layerId));
	return GetAnvilDescriptorSet().set_binding_array_items(bindingIdx,{arrayIndex,1u},bindingElements.data());
}
bool DescriptorSet::SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex)
{
	std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(1u,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(texture.GetImageView())->GetAnvilImageView(),&static_cast<Sampler*>(texture.GetSampler())->GetAnvilSampler()
	});
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingArrayTexture>(*this,bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture*>(binding)->SetArrayBinding(arrayIndex,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture));
	return GetAnvilDescriptorSet().set_binding_array_items(bindingIdx,{arrayIndex,1u},bindingElements.data());
}
bool DescriptorSet::SetBindingUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingUniformBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::UniformBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
	});
}
bool DescriptorSet::SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingDynamicUniformBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::DynamicUniformBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
	});
}
bool DescriptorSet::SetBindingStorageBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingStorageBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::StorageBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
		});
}

Anvil::DescriptorSetGroup &DescriptorSetGroup::GetAnvilDescriptorSetGroup() const {return *m_descriptorSetGroup;}
Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() {return *m_descriptorSetGroup;}
const Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() const {return const_cast<DescriptorSetGroup*>(this)->operator*();}
Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() {return m_descriptorSetGroup.get();}
const Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() const {return const_cast<DescriptorSetGroup*>(this)->operator->();}
#pragma optimize("",on)
