/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_descriptor_set_group.hpp"
#include "buffers/vk_buffer.hpp"
#include "image/vk_image_view.hpp"
#include "image/vk_sampler.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

std::shared_ptr<VlkDescriptorSetGroup> VlkDescriptorSetGroup::Create(IPrContext &context,Anvil::DescriptorSetGroupUniquePtr imgView,const std::function<void(IDescriptorSetGroup&)> &onDestroyedCallback)
{
	if(imgView == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<VlkDescriptorSetGroup>(new VlkDescriptorSetGroup(context,std::move(imgView)));
	return std::shared_ptr<VlkDescriptorSetGroup>(new VlkDescriptorSetGroup(context,std::move(imgView)),[onDestroyedCallback](VlkDescriptorSetGroup *buf) {
		buf->OnRelease();
		onDestroyedCallback(*buf);
		delete buf;
		});
}

VlkDescriptorSetGroup::VlkDescriptorSetGroup(IPrContext &context,Anvil::DescriptorSetGroupUniquePtr imgView)
	: IDescriptorSetGroup{context},m_descriptorSetGroup(std::move(imgView))
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::register_debug_object(m_descriptorSetGroup->get_descriptor_set(i),this,prosper::debug::ObjectType::DescriptorSet);
	m_descriptorSets.resize(numSets);

	for(auto i=decltype(m_descriptorSets.size()){0u};i<m_descriptorSets.size();++i)
	{
		m_descriptorSets.at(i) = std::shared_ptr<VlkDescriptorSet>{new VlkDescriptorSet{*this,*m_descriptorSetGroup->get_descriptor_set(i)},[](VlkDescriptorSet *ds) {
			// ds->OnRelease();
			delete ds;
		}};
	}
}
VlkDescriptorSetGroup::~VlkDescriptorSetGroup()
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::deregister_debug_object(m_descriptorSetGroup->get_descriptor_set(i));
}

VlkDescriptorSet::VlkDescriptorSet(VlkDescriptorSetGroup &dsg,Anvil::DescriptorSet &ds)
	: IDescriptorSet{dsg},m_descSet{ds}
{}

Anvil::DescriptorSet &VlkDescriptorSet::GetAnvilDescriptorSet() const {return m_descSet;}
Anvil::DescriptorSet &VlkDescriptorSet::operator*() {return m_descSet;}
const Anvil::DescriptorSet &VlkDescriptorSet::operator*() const {return const_cast<VlkDescriptorSet*>(this)->operator*();}
Anvil::DescriptorSet *VlkDescriptorSet::operator->() {return &m_descSet;}
const Anvil::DescriptorSet *VlkDescriptorSet::operator->() const {return const_cast<VlkDescriptorSet*>(this)->operator->();}

bool VlkDescriptorSet::Update() {return m_descSet.update();}
bool VlkDescriptorSet::SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId)
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
bool VlkDescriptorSet::SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx)
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
bool VlkDescriptorSet::SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId)
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
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView(),&static_cast<VlkSampler*>(sampler)->GetAnvilSampler()
		});
}
bool VlkDescriptorSet::SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx)
{
	auto *imgView = texture.GetImageView();
	auto *sampler = texture.GetSampler();
	if(imgView == nullptr || sampler == nullptr)
		return false;
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(imgView)->GetAnvilImageView(),&static_cast<VlkSampler*>(sampler)->GetAnvilSampler()
		});
}
bool VlkDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex,uint32_t layerId)
{
	std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(1u,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(texture.GetImageView(layerId))->GetAnvilImageView(),&static_cast<VlkSampler*>(texture.GetSampler())->GetAnvilSampler()
		});
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingArrayTexture>(*this,bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture*>(binding)->SetArrayBinding(arrayIndex,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture,layerId));
	return GetAnvilDescriptorSet().set_binding_array_items(bindingIdx,{arrayIndex,1u},bindingElements.data());
}
bool VlkDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex)
{
	std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> bindingElements(1u,Anvil::DescriptorSet::CombinedImageSamplerBindingElement{
		Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		&static_cast<VlkImageView*>(texture.GetImageView())->GetAnvilImageView(),&static_cast<VlkSampler*>(texture.GetSampler())->GetAnvilSampler()
		});
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingArrayTexture>(*this,bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture*>(binding)->SetArrayBinding(arrayIndex,std::make_unique<DescriptorSetBindingTexture>(*this,bindingIdx,texture));
	return GetAnvilDescriptorSet().set_binding_array_items(bindingIdx,{arrayIndex,1u},bindingElements.data());
}
bool VlkDescriptorSet::SetBindingUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingUniformBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::UniformBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
		});
}
bool VlkDescriptorSet::SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingDynamicUniformBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::DynamicUniformBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
		});
}
bool VlkDescriptorSet::SetBindingStorageBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset,uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx,std::make_unique<DescriptorSetBindingStorageBuffer>(*this,bindingIdx,buffer,startOffset,size));
	return GetAnvilDescriptorSet().set_binding_item(bindingIdx,Anvil::DescriptorSet::StorageBufferBindingElement{
		&dynamic_cast<VlkBuffer&>(buffer).GetBaseAnvilBuffer(),buffer.GetStartOffset() +startOffset,size
		});
}

Anvil::DescriptorSetGroup &VlkDescriptorSetGroup::GetAnvilDescriptorSetGroup() const {return *m_descriptorSetGroup;}
Anvil::DescriptorSetGroup &VlkDescriptorSetGroup::operator*() {return *m_descriptorSetGroup;}
const Anvil::DescriptorSetGroup &VlkDescriptorSetGroup::operator*() const {return const_cast<VlkDescriptorSetGroup*>(this)->operator*();}
Anvil::DescriptorSetGroup *VlkDescriptorSetGroup::operator->() {return m_descriptorSetGroup.get();}
const Anvil::DescriptorSetGroup *VlkDescriptorSetGroup::operator->() const {return const_cast<VlkDescriptorSetGroup*>(this)->operator->();}
