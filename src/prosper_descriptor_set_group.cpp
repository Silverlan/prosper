/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_descriptor_set_group.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_image_view.hpp"
#include "image/prosper_sampler.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include "buffers/vk_buffer.hpp"

using namespace prosper;

#pragma optimize("",off)
IDescriptorSetGroup::IDescriptorSetGroup(IPrContext &context,const DescriptorSetCreateInfo &createInfo)
	: ContextObject(context),std::enable_shared_from_this<IDescriptorSetGroup>(),m_createInfo{createInfo}
{}

IDescriptorSetGroup::~IDescriptorSetGroup() {}

uint32_t IDescriptorSetGroup::GetBindingCount() const {return dynamic_cast<const VlkDescriptorSetGroup&>(*this)->get_descriptor_set_create_info()->at(0)->get_n_bindings();}//m_descriptorSets.front()->GetBindingCount();}

const DescriptorSetCreateInfo &IDescriptorSetGroup::GetDescriptorSetCreateInfo() const {return m_createInfo;}

IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) {return (index < m_descriptorSets.size()) ? m_descriptorSets.at(index).get() : nullptr;}
const IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) const {return const_cast<IDescriptorSetGroup*>(this)->GetDescriptorSet(index);}

ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint& in)
	: name{in.name},shader_module_ptr{in.shader_module_ptr},stage{in.stage}
{}

ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string&    in_name,
	ShaderModule*         in_shader_module_ptr,
	ShaderStage           in_stage)
	: name{in_name},shader_module_ptr{in_shader_module_ptr},stage{in_stage}
{}
ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string&    in_name,
	std::unique_ptr<ShaderModule> in_shader_module_ptr,
	ShaderStage           in_stage)
	: name{in_name},shader_module_ptr{in_shader_module_ptr.get()},stage{in_stage},
	shader_module_owned_ptr{std::move(in_shader_module_ptr)}
{}

ShaderModuleStageEntryPoint& ShaderModuleStageEntryPoint::operator=(const ShaderModuleStageEntryPoint &in)
{
	name              = in.name;
	shader_module_ptr = in.shader_module_ptr;
	stage             = in.stage;

	return *this;
}

DescriptorSetCreateInfo::DescriptorSetCreateInfo(const DescriptorSetCreateInfo &other)
	: m_bindings{other.m_bindings},m_numVariableDescriptorCountBinding{other.m_numVariableDescriptorCountBinding},
	m_variableDescriptorCountBindingSize{other.m_variableDescriptorCountBindingSize}
{}
DescriptorSetCreateInfo::DescriptorSetCreateInfo(DescriptorSetCreateInfo &&other)
	: m_bindings{std::move(other.m_bindings)},m_numVariableDescriptorCountBinding{other.m_numVariableDescriptorCountBinding},
	m_variableDescriptorCountBindingSize{other.m_variableDescriptorCountBindingSize}
{}
DescriptorSetCreateInfo &DescriptorSetCreateInfo::operator=(const DescriptorSetCreateInfo &other)
{
	m_bindings = other.m_bindings;
	m_numVariableDescriptorCountBinding = other.m_numVariableDescriptorCountBinding;
	m_variableDescriptorCountBindingSize = other.m_variableDescriptorCountBindingSize;
	return *this;
}
DescriptorSetCreateInfo &DescriptorSetCreateInfo::operator=(DescriptorSetCreateInfo &&other)
{
	m_bindings = std::move(other.m_bindings);
	m_numVariableDescriptorCountBinding = other.m_numVariableDescriptorCountBinding;
	m_variableDescriptorCountBindingSize = other.m_variableDescriptorCountBindingSize;
	return *this;
}
bool DescriptorSetCreateInfo::GetBindingPropertiesByBindingIndex(
	uint32_t bindingIndex,
	DescriptorType *outOptDescriptorType,
	uint32_t *outOptDescriptorArraySize,
	ShaderStageFlags *outOptStageFlags,
	bool *outOptImmutableSamplersEnabled,
	DescriptorBindingFlags *outOptFlags
) const
{
	auto binding_iterator = m_bindings.find(bindingIndex);
	bool result           = false;

	if (binding_iterator == m_bindings.end() )
	{
		goto end;
	}

	if (outOptDescriptorArraySize != nullptr)
	{
		*outOptDescriptorArraySize = binding_iterator->second.descriptorArraySize;
	}

	if (outOptDescriptorType != nullptr)
	{
		*outOptDescriptorType = binding_iterator->second.descriptorType;
	}

	if (outOptImmutableSamplersEnabled != nullptr)
	{
		*outOptImmutableSamplersEnabled = (binding_iterator->second.immutableSamplers.size() != 0);
	}

	if (outOptStageFlags != nullptr)
	{
		*outOptStageFlags = binding_iterator->second.stageFlags;
	}

	if (outOptFlags != nullptr)
	{
		*outOptFlags = binding_iterator->second.flags;
	}

	result = true;

end:
	return result;
}
bool DescriptorSetCreateInfo::GetBindingPropertiesByIndexNumber(
	uint32_t nBinding,
	uint32_t *out_opt_binding_index_ptr,
	DescriptorType *outOptDescriptorType,
	uint32_t *outOptDescriptorArraySize,
	ShaderStageFlags *outOptStageFlags,
	bool *outOptImmutableSamplersEnabled,
	DescriptorBindingFlags *outOptFlags
)
{
	auto binding_iterator = m_bindings.begin();
	bool result           = false;

	if (m_bindings.size() <= nBinding)
	{
		goto end;
	}

	for (uint32_t n_current_binding = 0;
		n_current_binding < nBinding;
		++n_current_binding)
	{
		binding_iterator ++;
	}

	if (binding_iterator != m_bindings.end() )
	{
		if (out_opt_binding_index_ptr != nullptr)
		{
			*out_opt_binding_index_ptr = binding_iterator->first;
		}

		if (outOptDescriptorArraySize != nullptr)
		{
			*outOptDescriptorArraySize = binding_iterator->second.descriptorArraySize;
		}

		if (outOptDescriptorType != nullptr)
		{
			*outOptDescriptorType = binding_iterator->second.descriptorType;
		}

		if (outOptImmutableSamplersEnabled != nullptr)
		{
			*outOptImmutableSamplersEnabled = (binding_iterator->second.immutableSamplers.size() != 0);
		}

		if (outOptStageFlags != nullptr)
		{
			*outOptStageFlags = binding_iterator->second.stageFlags;
		}

		if (outOptFlags != nullptr)
		{
			*outOptFlags = binding_iterator->second.flags;
		}

		result = true;
	}

end:
	return result;
}
bool DescriptorSetCreateInfo::AddBinding(uint32_t                             in_binding_index,
	DescriptorType                in_descriptor_type,
	uint32_t                             in_descriptor_array_size,
	ShaderStageFlags              in_stage_flags,
	const DescriptorBindingFlags& in_flags,
	const ISampler* const*         in_immutable_sampler_ptrs)
{
	bool result = false;

	/* Make sure the binding is not already defined */
	if (m_bindings.find(in_binding_index) != m_bindings.end() )
	{
		anvil_assert_fail();

		goto end;
	}

	/* Make sure the sampler array can actually be specified for this descriptor type. */
	if (in_immutable_sampler_ptrs != nullptr)
	{
		if (in_descriptor_type != DescriptorType::CombinedImageSampler &&
			in_descriptor_type != DescriptorType::Sampler)
		{
			anvil_assert_fail();

			goto end;
		}
	}

	if (in_descriptor_type == DescriptorType::InlineUniformBlock)
	{
		anvil_assert((in_descriptor_array_size % 4) == 0);
	}

	if ((in_flags & DescriptorBindingFlags::VariableDescriptorCountBit) != DescriptorBindingFlags::None)
	{
		Anvil::DescriptorSetCreateInfo;
		if (m_numVariableDescriptorCountBinding != UINT32_MAX)
		{
			/* If this assertion check fails, you're attempting to add more than 1 variable descriptor count binding
			* which is illegal! */
			anvil_assert(m_numVariableDescriptorCountBinding == UINT32_MAX);

			goto end;
		}

		m_numVariableDescriptorCountBinding = in_binding_index;
	}

	/* Add a new binding entry and mark the layout as dirty, so that it is re-baked next time
	* the user calls the getter func */
	m_bindings[in_binding_index] = Binding(in_descriptor_array_size,
		in_descriptor_type,
		in_stage_flags,
		in_immutable_sampler_ptrs,
		in_flags);

	result  = true;
end:
	return result;
}

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
#pragma optimize("",on)
