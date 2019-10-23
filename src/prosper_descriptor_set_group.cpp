/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "prosper_descriptor_set_group.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "debug/prosper_debug_lookup_map.hpp"

using namespace prosper;

std::shared_ptr<DescriptorSetGroup> DescriptorSetGroup::Create(Context &context,Anvil::DescriptorSetGroupUniquePtr imgView,const std::function<void(DescriptorSetGroup&)> &onDestroyedCallback)
{
	if(imgView == nullptr)
		return nullptr;
	if(onDestroyedCallback == nullptr)
		return std::shared_ptr<DescriptorSetGroup>(new DescriptorSetGroup(context,std::move(imgView)));
	return std::shared_ptr<DescriptorSetGroup>(new DescriptorSetGroup(context,std::move(imgView)),[onDestroyedCallback](DescriptorSetGroup *buf) {
		onDestroyedCallback(*buf);
		delete buf;
	});
}

DescriptorSetGroup::DescriptorSetGroup(Context &context,Anvil::DescriptorSetGroupUniquePtr imgView)
	: ContextObject(context),std::enable_shared_from_this<DescriptorSetGroup>(),m_descriptorSetGroup(std::move(imgView))
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::register_debug_object(m_descriptorSetGroup->get_descriptor_set(i),this,prosper::debug::ObjectType::DescriptorSet);
	m_descriptorSets.resize(numSets);

	for(auto i=decltype(m_descriptorSets.size()){0u};i<m_descriptorSets.size();++i)
		m_descriptorSets.at(i) = std::make_shared<DescriptorSet>(*this,*m_descriptorSetGroup->get_descriptor_set(i));
}

DescriptorSetGroup::~DescriptorSetGroup()
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::deregister_debug_object(m_descriptorSetGroup->get_descriptor_set(i));
}

DescriptorSet *DescriptorSetGroup::GetDescriptorSet(uint32_t index) {return (index < m_descriptorSets.size()) ? m_descriptorSets.at(index).get() : nullptr;}
const DescriptorSet *DescriptorSetGroup::GetDescriptorSet(uint32_t index) const {return const_cast<DescriptorSetGroup*>(this)->GetDescriptorSet(index);}

Anvil::DescriptorSetGroup &DescriptorSetGroup::GetAnvilDescriptorSetGroup() const {return *m_descriptorSetGroup;}
Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() {return *m_descriptorSetGroup;}
const Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() const {return const_cast<DescriptorSetGroup*>(this)->operator*();}
Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() {return m_descriptorSetGroup.get();}
const Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() const {return const_cast<DescriptorSetGroup*>(this)->operator->();}

/////////////////

DescriptorSet::DescriptorSet(DescriptorSetGroup &dsg,Anvil::DescriptorSet &ds)
	: m_dsg{dsg},m_descSet{ds}
{
	auto numBindings = dsg->get_descriptor_set_create_info()->at(0)->get_n_bindings();
	m_bindings.resize(numBindings);
}

DescriptorSetBinding *DescriptorSet::GetBinding(uint32_t bindingIndex) {return (bindingIndex < m_bindings.size()) ? m_bindings.at(bindingIndex).get() : nullptr;}
const DescriptorSetBinding *DescriptorSet::GetBinding(uint32_t bindingIndex) const {return const_cast<DescriptorSet*>(this)->GetBinding(bindingIndex);}

std::vector<std::unique_ptr<DescriptorSetBinding>> &DescriptorSet::GetBindings() {return m_bindings;}
const std::vector<std::unique_ptr<DescriptorSetBinding>> &DescriptorSet::GetBindings() const {return const_cast<DescriptorSet*>(this)->GetBindings();}

DescriptorSetBinding &DescriptorSet::SetBinding(uint32_t bindingIndex,std::unique_ptr<DescriptorSetBinding> binding)
{
	if(bindingIndex >= m_bindings.size())
		throw std::invalid_argument{std::to_string(bindingIndex) +" exceeds descriptor set binding count " +std::to_string(m_bindings.size()) +" of descriptor set group " +GetDescriptorSetGroup().GetDebugName()};
	m_bindings.at(bindingIndex) = std::move(binding);
	return *m_bindings.at(bindingIndex);
}

prosper::Buffer *DescriptorSet::GetBoundBuffer(uint32_t bindingIndex,uint64_t *outStartOffset,uint64_t *outSize)
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

prosper::Texture *DescriptorSet::GetBoundTexture(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex)
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

prosper::Image *DescriptorSet::GetBoundImage(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex)
{
	auto *tex = GetBoundTexture(bindingIndex,optOutLayerIndex);
	return tex ? tex->GetImage().get() : nullptr;
}

DescriptorSetGroup &DescriptorSet::GetDescriptorSetGroup() const {return m_dsg;}
Anvil::DescriptorSet &DescriptorSet::GetAnvilDescriptorSet() const {return m_descSet;}
Anvil::DescriptorSet &DescriptorSet::operator*() {return m_descSet;}
const Anvil::DescriptorSet &DescriptorSet::operator*() const {return const_cast<DescriptorSet*>(this)->operator*();}
Anvil::DescriptorSet *DescriptorSet::operator->() {return &m_descSet;}
const Anvil::DescriptorSet *DescriptorSet::operator->() const {return const_cast<DescriptorSet*>(this)->operator->();}

/////////////////

DescriptorSetBinding::DescriptorSetBinding(DescriptorSet &descSet,uint32_t bindingIdx)
	: m_descriptorSet{descSet},m_bindingIndex{bindingIdx}
{}
uint32_t DescriptorSetBinding::GetBindingIndex() const {return m_bindingIndex;}
DescriptorSet &DescriptorSetBinding::GetDescriptorSet() const {return m_descriptorSet;}

///

DescriptorSetBindingStorageImage::DescriptorSetBindingStorageImage(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId)
	: DescriptorSetBinding{descSet,bindingIdx},m_texture{texture.shared_from_this()},m_layerId{layerId}
{}
std::optional<uint32_t> DescriptorSetBindingStorageImage::GetLayerIndex() const {return m_layerId;}
const std::shared_ptr<prosper::Texture> &DescriptorSetBindingStorageImage::GetTexture() const {return m_texture;}

///

DescriptorSetBindingTexture::DescriptorSetBindingTexture(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId)
	: DescriptorSetBinding{descSet,bindingIdx},m_texture{texture.shared_from_this()},m_layerId{layerId}
{}
std::optional<uint32_t> DescriptorSetBindingTexture::GetLayerIndex() const {return m_layerId;}
const std::shared_ptr<prosper::Texture> &DescriptorSetBindingTexture::GetTexture() const {return m_texture;}

///

DescriptorSetBindingArrayTexture::DescriptorSetBindingArrayTexture(DescriptorSet &descSet,uint32_t bindingIdx)
	: DescriptorSetBinding{descSet,bindingIdx}
{}
void DescriptorSetBindingArrayTexture::SetArrayBinding(uint32_t arrayIndex,std::unique_ptr<DescriptorSetBindingTexture> bindingTexture)
{
	if(arrayIndex >= m_arrayItems.size())
		m_arrayItems.resize(arrayIndex +1);
	m_arrayItems.at(arrayIndex) = std::move(bindingTexture);
}

///

DescriptorSetBindingUniformBuffer::DescriptorSetBindingUniformBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingUniformBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingUniformBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::Buffer> &DescriptorSetBindingUniformBuffer::GetBuffer() const {return m_buffer;}

///

DescriptorSetBindingDynamicUniformBuffer::DescriptorSetBindingDynamicUniformBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingDynamicUniformBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingDynamicUniformBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::Buffer> &DescriptorSetBindingDynamicUniformBuffer::GetBuffer() const {return m_buffer;}

///

DescriptorSetBindingStorageBuffer::DescriptorSetBindingStorageBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size)
	: DescriptorSetBinding{descSet,bindingIdx},m_buffer{buffer.shared_from_this()},m_startOffset{startOffset},m_size{size}
{}
uint64_t DescriptorSetBindingStorageBuffer::GetStartOffset() const {return m_startOffset;}
uint64_t DescriptorSetBindingStorageBuffer::GetSize() const {return m_size;}
const std::shared_ptr<prosper::Buffer> &DescriptorSetBindingStorageBuffer::GetBuffer() const {return m_buffer;}
