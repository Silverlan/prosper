/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "prosper_window.hpp"
#include "prosper_descriptor_set_group.hpp"
#include "buffers/prosper_swap_buffer.hpp"
#include "image/prosper_image_view.hpp"
#include "image/prosper_sampler.hpp"
#include "image/prosper_texture.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <cassert>

using namespace prosper;

IDescriptorSetGroup::IDescriptorSetGroup(IPrContext &context, const DescriptorSetCreateInfo &createInfo) : ContextObject(context), std::enable_shared_from_this<IDescriptorSetGroup>(), m_createInfo {createInfo} {}

IDescriptorSetGroup::~IDescriptorSetGroup() {}

uint32_t IDescriptorSetGroup::GetBindingCount() const { return GetDescriptorSetCreateInfo().GetBindingCount(); }

const DescriptorSetCreateInfo &IDescriptorSetGroup::GetDescriptorSetCreateInfo() const { return m_createInfo; }

IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) { return m_descriptorSets.at(index).get(); }
const IDescriptorSet *IDescriptorSetGroup::GetDescriptorSet(uint32_t index) const { return const_cast<IDescriptorSetGroup *>(this)->GetDescriptorSet(index); }

ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const ShaderModuleStageEntryPoint &in) : name {in.name}, shader_module_ptr {in.shader_module_ptr}, stage {in.stage} {}

ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string &in_name, ShaderModule *in_shader_module_ptr, ShaderStage in_stage) : name {in_name}, shader_module_ptr {in_shader_module_ptr}, stage {in_stage} {}
ShaderModuleStageEntryPoint::ShaderModuleStageEntryPoint(const std::string &in_name, std::unique_ptr<ShaderModule> in_shader_module_ptr, ShaderStage in_stage)
    : name {in_name}, shader_module_ptr {in_shader_module_ptr.get()}, stage {in_stage}, shader_module_owned_ptr {std::move(in_shader_module_ptr)}
{
}

ShaderModuleStageEntryPoint &ShaderModuleStageEntryPoint::operator=(const ShaderModuleStageEntryPoint &in)
{
	name = in.name;
	shader_module_ptr = in.shader_module_ptr;
	stage = in.stage;

	return *this;
}

DescriptorSetCreateInfo::DescriptorSetCreateInfo(const DescriptorSetCreateInfo &other) : m_bindings {other.m_bindings}, m_numVariableDescriptorCountBinding {other.m_numVariableDescriptorCountBinding}, m_variableDescriptorCountBindingSize {other.m_variableDescriptorCountBindingSize} {}
DescriptorSetCreateInfo::DescriptorSetCreateInfo(DescriptorSetCreateInfo &&other) : m_bindings {std::move(other.m_bindings)}, m_numVariableDescriptorCountBinding {other.m_numVariableDescriptorCountBinding}, m_variableDescriptorCountBindingSize {other.m_variableDescriptorCountBindingSize}
{
}
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
bool DescriptorSetCreateInfo::GetBindingPropertiesByBindingIndex(uint32_t bindingIndex, DescriptorType *outOptDescriptorType, uint32_t *outOptDescriptorArraySize, ShaderStageFlags *outOptStageFlags, bool *outOptImmutableSamplersEnabled, DescriptorBindingFlags *outOptFlags,
  PrDescriptorSetBindingFlags *outOptPrFlags) const
{
	auto binding_iterator = m_bindings.find(bindingIndex);
	bool result = false;

	if(binding_iterator == m_bindings.end()) {
		goto end;
	}

	if(outOptDescriptorArraySize != nullptr) {
		*outOptDescriptorArraySize = binding_iterator->second.descriptorArraySize;
	}

	if(outOptDescriptorType != nullptr) {
		*outOptDescriptorType = binding_iterator->second.descriptorType;
	}

	if(outOptImmutableSamplersEnabled != nullptr) {
		*outOptImmutableSamplersEnabled = (binding_iterator->second.immutableSamplers.size() != 0);
	}

	if(outOptStageFlags != nullptr) {
		*outOptStageFlags = binding_iterator->second.stageFlags;
	}

	if(outOptFlags != nullptr) {
		*outOptFlags = binding_iterator->second.flags;
	}

	if(outOptPrFlags != nullptr) {
		*outOptPrFlags = binding_iterator->second.prFlags;
	}

	result = true;

end:
	return result;
}
bool DescriptorSetCreateInfo::GetBindingPropertiesByIndexNumber(uint32_t nBinding, uint32_t *out_opt_binding_index_ptr, DescriptorType *outOptDescriptorType, uint32_t *outOptDescriptorArraySize, ShaderStageFlags *outOptStageFlags, bool *outOptImmutableSamplersEnabled,
  DescriptorBindingFlags *outOptFlags, PrDescriptorSetBindingFlags *outOptPrFlags)
{
	auto binding_iterator = m_bindings.begin();
	bool result = false;

	if(m_bindings.size() <= nBinding) {
		goto end;
	}

	for(uint32_t n_current_binding = 0; n_current_binding < nBinding; ++n_current_binding) {
		binding_iterator++;
	}

	if(binding_iterator != m_bindings.end()) {
		if(out_opt_binding_index_ptr != nullptr) {
			*out_opt_binding_index_ptr = binding_iterator->first;
		}

		if(outOptDescriptorArraySize != nullptr) {
			*outOptDescriptorArraySize = binding_iterator->second.descriptorArraySize;
		}

		if(outOptDescriptorType != nullptr) {
			*outOptDescriptorType = binding_iterator->second.descriptorType;
		}

		if(outOptImmutableSamplersEnabled != nullptr) {
			*outOptImmutableSamplersEnabled = (binding_iterator->second.immutableSamplers.size() != 0);
		}

		if(outOptStageFlags != nullptr) {
			*outOptStageFlags = binding_iterator->second.stageFlags;
		}

		if(outOptFlags != nullptr) {
			*outOptFlags = binding_iterator->second.flags;
		}

		if(outOptPrFlags != nullptr) {
			*outOptPrFlags = binding_iterator->second.prFlags;
		}

		result = true;
	}

end:
	return result;
}
bool DescriptorSetCreateInfo::AddBinding(uint32_t in_binding_index, DescriptorType in_descriptor_type, uint32_t in_descriptor_array_size, ShaderStageFlags in_stage_flags, const DescriptorBindingFlags &in_flags, const PrDescriptorSetBindingFlags &in_pr_flags,
  const ISampler *const *in_immutable_sampler_ptrs)
{
	bool result = false;

	/* Make sure the binding is not already defined */
	if(m_bindings.find(in_binding_index) != m_bindings.end()) {
		assert(false);

		goto end;
	}

	/* Make sure the sampler array can actually be specified for this descriptor type. */
	if(in_immutable_sampler_ptrs != nullptr) {
		if(in_descriptor_type != DescriptorType::CombinedImageSampler && in_descriptor_type != DescriptorType::Sampler) {
			assert(false);

			goto end;
		}
	}

	if(in_descriptor_type == DescriptorType::InlineUniformBlock) {
		assert((in_descriptor_array_size % 4) == 0);
	}

	if((in_flags & DescriptorBindingFlags::VariableDescriptorCountBit) != DescriptorBindingFlags::None) {
		if(m_numVariableDescriptorCountBinding != UINT32_MAX) {
			/* If this assertion check fails, you're attempting to add more than 1 variable descriptor count binding
			* which is illegal! */
			assert(m_numVariableDescriptorCountBinding == UINT32_MAX);

			goto end;
		}

		m_numVariableDescriptorCountBinding = in_binding_index;
	}

	/* Add a new binding entry and mark the layout as dirty, so that it is re-baked next time
	* the user calls the getter func */
	m_bindings[in_binding_index] = Binding(in_descriptor_array_size, in_descriptor_type, in_stage_flags, in_immutable_sampler_ptrs, in_flags, in_pr_flags);

	result = true;
end:
	return result;
}

/////////////////

IDescriptorSet::IDescriptorSet(IDescriptorSetGroup &dsg) : m_dsg {dsg}
{
	auto numBindings = dsg.GetBindingCount();
	m_bindings.resize(numBindings);
}

uint32_t IDescriptorSet::GetBindingCount() const { return m_bindings.size(); }

DescriptorSetBinding *IDescriptorSet::GetBinding(uint32_t bindingIndex) { return (bindingIndex < m_bindings.size()) ? m_bindings.at(bindingIndex).get() : nullptr; }
const DescriptorSetBinding *IDescriptorSet::GetBinding(uint32_t bindingIndex) const { return const_cast<IDescriptorSet *>(this)->GetBinding(bindingIndex); }

std::vector<std::unique_ptr<DescriptorSetBinding>> &IDescriptorSet::GetBindings() { return m_bindings; }
const std::vector<std::unique_ptr<DescriptorSetBinding>> &IDescriptorSet::GetBindings() const { return const_cast<IDescriptorSet *>(this)->GetBindings(); }

DescriptorSetBinding &IDescriptorSet::SetBinding(uint32_t bindingIndex, std::unique_ptr<DescriptorSetBinding> binding)
{
	if(bindingIndex >= m_bindings.size())
		throw std::invalid_argument {std::to_string(bindingIndex) + " exceeds descriptor set binding count " + std::to_string(m_bindings.size()) + " of descriptor set group " + GetDescriptorSetGroup().GetDebugName()};
	m_bindings.at(bindingIndex) = std::move(binding);
	return *m_bindings.at(bindingIndex);
}

bool IDescriptorSet::SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId)
{
	auto *imgView = texture.GetImageView(layerId);
	if(imgView == nullptr)
		return false;
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingStorageImage>(*this, bindingIdx, texture, layerId));
	return DoSetBindingStorageImage(texture, bindingIdx, layerId);
}
bool IDescriptorSet::SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx)
{
	auto *imgView = texture.GetImageView();
	if(imgView == nullptr)
		return false;
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingStorageImage>(*this, bindingIdx, texture));
	return DoSetBindingStorageImage(texture, bindingIdx, {});
}
bool IDescriptorSet::SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId)
{
	auto *imgView = texture.GetImageView(layerId);
	if(imgView == nullptr && texture.GetImage().GetLayerCount() == 1)
		imgView = texture.GetImageView();
	auto *sampler = texture.GetSampler();
	if(imgView == nullptr || sampler == nullptr)
		return false;
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingTexture>(*this, bindingIdx, texture, layerId));
	return DoSetBindingTexture(texture, bindingIdx, layerId);
}
bool IDescriptorSet::SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx)
{
	auto *imgView = texture.GetImageView();
	auto *sampler = texture.GetSampler();
	if(imgView == nullptr || sampler == nullptr)
		return false;
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingTexture>(*this, bindingIdx, texture));
	return DoSetBindingTexture(texture, bindingIdx, {});
}
bool IDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex, uint32_t layerId)
{
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingArrayTexture>(*this, bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture *>(binding)->SetArrayBinding(arrayIndex, std::make_unique<DescriptorSetBindingTexture>(*this, bindingIdx, texture, layerId));
	return DoSetBindingArrayTexture(texture, bindingIdx, arrayIndex, layerId);
}
bool IDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex)
{
	auto *binding = GetBinding(bindingIdx);
	if(binding == nullptr)
		binding = &SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingArrayTexture>(*this, bindingIdx));
	if(binding && binding->GetType() == prosper::DescriptorSetBinding::Type::ArrayTexture)
		static_cast<prosper::DescriptorSetBindingArrayTexture *>(binding)->SetArrayBinding(arrayIndex, std::make_unique<DescriptorSetBindingTexture>(*this, bindingIdx, texture));
	return DoSetBindingArrayTexture(texture, bindingIdx, arrayIndex, {});
}
bool IDescriptorSet::SetBindingUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingUniformBuffer>(*this, bindingIdx, buffer, startOffset, size));
	return DoSetBindingUniformBuffer(buffer, bindingIdx, startOffset, size);
}
bool IDescriptorSet::SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingDynamicUniformBuffer>(*this, bindingIdx, buffer, startOffset, size));
	return DoSetBindingDynamicUniformBuffer(buffer, bindingIdx, startOffset, size);
}
bool IDescriptorSet::SetBindingStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingStorageBuffer>(*this, bindingIdx, buffer, startOffset, size));
	return DoSetBindingStorageBuffer(buffer, bindingIdx, startOffset, size);
}
bool IDescriptorSet::SetBindingDynamicStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	size = (size != std::numeric_limits<decltype(size)>::max()) ? size : buffer.GetSize();
	SetBinding(bindingIdx, std::make_unique<DescriptorSetBindingDynamicStorageBuffer>(*this, bindingIdx, buffer, startOffset, size));
	return DoSetBindingDynamicStorageBuffer(buffer, bindingIdx, startOffset, size);
}

void IDescriptorSet::ReloadBinding(uint32_t bindingIdx)
{
	if(bindingIdx >= m_bindings.size() || m_bindings[bindingIdx] == nullptr)
		return;
	auto &pbinding = m_bindings[bindingIdx];
	switch(pbinding->GetType()) {
	case DescriptorSetBinding::Type::StorageImage:
		{
			auto &binding = static_cast<DescriptorSetBindingStorageImage &>(*pbinding);
			auto &tex = binding.GetTexture();
			if(tex.expired())
				return;
			DoSetBindingStorageImage(*tex.lock(), binding.GetBindingIndex(), binding.GetLayerIndex());
			break;
		}
	case DescriptorSetBinding::Type::Texture:
		{
			auto &binding = static_cast<DescriptorSetBindingTexture &>(*pbinding);
			auto &tex = binding.GetTexture();
			if(tex.expired())
				return;
			DoSetBindingTexture(*tex.lock(), binding.GetBindingIndex(), binding.GetLayerIndex());
			break;
		}
	case DescriptorSetBinding::Type::ArrayTexture:
		{
			auto &binding = static_cast<DescriptorSetBindingArrayTexture &>(*pbinding);
			auto n = binding.GetArrayCount();
			for(auto i = decltype(n) {0u}; i < n; ++i) {
				auto *texBinding = binding.GetArrayItem(i);
				if(texBinding == nullptr)
					continue;
				auto &tex = texBinding->GetTexture();
				if(tex.expired())
					continue;
				DoSetBindingArrayTexture(*tex.lock(), texBinding->GetBindingIndex(), i, texBinding->GetLayerIndex());
			}
			break;
		}
	case DescriptorSetBinding::Type::UniformBuffer:
		{
			auto &binding = static_cast<DescriptorSetBindingUniformBuffer &>(*pbinding);
			auto &buf = binding.GetBuffer();
			if(buf.expired())
				return;
			DoSetBindingUniformBuffer(*buf.lock(), binding.GetBindingIndex(), binding.GetStartOffset(), binding.GetSize());
			break;
		}
	case DescriptorSetBinding::Type::DynamicUniformBuffer:
		{
			auto &binding = static_cast<DescriptorSetBindingDynamicUniformBuffer &>(*pbinding);
			auto &buf = binding.GetBuffer();
			if(buf.expired())
				return;
			DoSetBindingDynamicUniformBuffer(*buf.lock(), binding.GetBindingIndex(), binding.GetStartOffset(), binding.GetSize());
			break;
		}
	case DescriptorSetBinding::Type::StorageBuffer:
		{
			auto &binding = static_cast<DescriptorSetBindingStorageBuffer &>(*pbinding);
			auto &buf = binding.GetBuffer();
			if(buf.expired())
				return;
			DoSetBindingStorageBuffer(*buf.lock(), binding.GetBindingIndex(), binding.GetStartOffset(), binding.GetSize());
			break;
		}
	case DescriptorSetBinding::Type::DynamicStorageBuffer:
		{
			auto &binding = static_cast<DescriptorSetBindingDynamicStorageBuffer &>(*pbinding);
			auto &buf = binding.GetBuffer();
			if(buf.expired())
				return;
			DoSetBindingDynamicStorageBuffer(*buf.lock(), binding.GetBindingIndex(), binding.GetStartOffset(), binding.GetSize());
			break;
		}
	}
	static_assert(umath::to_integral(DescriptorSetBinding::Type::Count) == 7u, "Update this implementation when new types are added!");
}

prosper::IBuffer *IDescriptorSet::GetBoundBuffer(uint32_t bindingIndex, uint64_t *outStartOffset, uint64_t *outSize)
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return nullptr;
	auto &binding = m_bindings.at(bindingIndex);
	switch(binding->GetType()) {
	case DescriptorSetBinding::Type::UniformBuffer:
		{
			auto &bindingImg = *static_cast<DescriptorSetBindingUniformBuffer *>(binding.get());
			if(outStartOffset)
				*outStartOffset = bindingImg.GetStartOffset();
			if(outSize)
				*outSize = bindingImg.GetSize();
			return bindingImg.GetBuffer().lock().get();
		}
	case DescriptorSetBinding::Type::DynamicUniformBuffer:
		{
			auto &bindingTex = *static_cast<DescriptorSetBindingDynamicUniformBuffer *>(binding.get());
			if(outStartOffset)
				*outStartOffset = bindingTex.GetStartOffset();
			if(outSize)
				*outSize = bindingTex.GetSize();
			return bindingTex.GetBuffer().lock().get();
		}
	case DescriptorSetBinding::Type::StorageBuffer:
		{
			auto &bindingImg = *static_cast<DescriptorSetBindingStorageBuffer *>(binding.get());
			if(outStartOffset)
				*outStartOffset = bindingImg.GetStartOffset();
			if(outSize)
				*outSize = bindingImg.GetSize();
			return bindingImg.GetBuffer().lock().get();
		}
	}
	return nullptr;
}

prosper::Texture *IDescriptorSet::GetBoundArrayTexture(uint32_t bindingIndex, uint32_t index)
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return 0;
	auto &binding = m_bindings.at(bindingIndex);
	if(binding->GetType() != DescriptorSetBinding::Type::ArrayTexture)
		return 0;
	auto &arrayTextureBinding = static_cast<prosper::DescriptorSetBindingArrayTexture &>(*binding);
	auto *item = arrayTextureBinding.GetArrayItem(index);
	return item ? item->GetTexture().lock().get() : nullptr;
}
uint32_t IDescriptorSet::GetBoundArrayTextureCount(uint32_t bindingIndex) const
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return 0;
	auto &binding = m_bindings.at(bindingIndex);
	if(binding->GetType() != DescriptorSetBinding::Type::ArrayTexture)
		return 0;
	return static_cast<prosper::DescriptorSetBindingArrayTexture &>(*binding).GetArrayCount();
}

prosper::Texture *IDescriptorSet::GetBoundTexture(uint32_t bindingIndex, std::optional<uint32_t> *optOutLayerIndex)
{
	if(bindingIndex >= m_bindings.size() || m_bindings.at(bindingIndex) == nullptr)
		return nullptr;
	auto &binding = m_bindings.at(bindingIndex);
	switch(binding->GetType()) {
	case DescriptorSetBinding::Type::StorageImage:
		{
			auto &bindingImg = *static_cast<DescriptorSetBindingStorageImage *>(binding.get());
			if(optOutLayerIndex)
				*optOutLayerIndex = bindingImg.GetLayerIndex();
			return bindingImg.GetTexture().lock().get();
		}
	case DescriptorSetBinding::Type::Texture:
		{
			auto &bindingTex = *static_cast<DescriptorSetBindingTexture *>(binding.get());
			if(optOutLayerIndex)
				*optOutLayerIndex = bindingTex.GetLayerIndex();
			return bindingTex.GetTexture().lock().get();
		}
	}
	return nullptr;
}

prosper::IImage *IDescriptorSet::GetBoundImage(uint32_t bindingIndex, std::optional<uint32_t> *optOutLayerIndex)
{
	auto *tex = GetBoundTexture(bindingIndex, optOutLayerIndex);
	return tex ? &tex->GetImage() : nullptr;
}

IDescriptorSetGroup &IDescriptorSet::GetDescriptorSetGroup() const { return m_dsg; }

/////////////////

DescriptorSetBinding::DescriptorSetBinding(IDescriptorSet &descSet, uint32_t bindingIdx) : m_descriptorSet {descSet}, m_bindingIndex {bindingIdx} {}
uint32_t DescriptorSetBinding::GetBindingIndex() const { return m_bindingIndex; }
IDescriptorSet &DescriptorSetBinding::GetDescriptorSet() const { return m_descriptorSet; }

///

DescriptorSetBindingStorageImage::DescriptorSetBindingStorageImage(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::Texture &texture, std::optional<uint32_t> layerId) : DescriptorSetBinding {descSet, bindingIdx}, m_texture {texture.shared_from_this()}, m_layerId {layerId} {}
std::optional<uint32_t> DescriptorSetBindingStorageImage::GetLayerIndex() const { return m_layerId; }
const std::weak_ptr<prosper::Texture> &DescriptorSetBindingStorageImage::GetTexture() const { return m_texture; }

///

DescriptorSetBindingTexture::DescriptorSetBindingTexture(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::Texture &texture, std::optional<uint32_t> layerId) : DescriptorSetBinding {descSet, bindingIdx}, m_texture {texture.shared_from_this()}, m_layerId {layerId} {}
std::optional<uint32_t> DescriptorSetBindingTexture::GetLayerIndex() const { return m_layerId; }
const std::weak_ptr<prosper::Texture> &DescriptorSetBindingTexture::GetTexture() const { return m_texture; }

///

DescriptorSetBindingArrayTexture::DescriptorSetBindingArrayTexture(IDescriptorSet &descSet, uint32_t bindingIdx) : DescriptorSetBinding {descSet, bindingIdx} {}
void DescriptorSetBindingArrayTexture::SetArrayBinding(uint32_t arrayIndex, std::unique_ptr<DescriptorSetBindingTexture> bindingTexture)
{
	if(arrayIndex >= m_arrayItems.size())
		m_arrayItems.resize(arrayIndex + 1);
	m_arrayItems.at(arrayIndex) = std::move(bindingTexture);
}
uint32_t DescriptorSetBindingArrayTexture::GetArrayCount() const { return m_arrayItems.size(); }
DescriptorSetBindingTexture *DescriptorSetBindingArrayTexture::GetArrayItem(uint32_t idx) { return (idx < m_arrayItems.size()) ? m_arrayItems.at(idx).get() : nullptr; }

///

DescriptorSetBindingBaseBuffer::DescriptorSetBindingBaseBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size)
    : DescriptorSetBinding {descSet, bindingIdx}, m_buffer {buffer.shared_from_this()}, m_startOffset {startOffset}, m_size {size}
{
	m_cbBufferReallocation = buffer.AddReallocationCallback([&descSet, bindingIdx]() { descSet.ReloadBinding(bindingIdx); });
}
DescriptorSetBindingBaseBuffer::~DescriptorSetBindingBaseBuffer()
{
	if(m_cbBufferReallocation.IsValid())
		m_cbBufferReallocation.Remove();
}
uint64_t DescriptorSetBindingBaseBuffer::GetStartOffset() const { return m_startOffset; }
uint64_t DescriptorSetBindingBaseBuffer::GetSize() const { return m_size; }
const std::weak_ptr<prosper::IBuffer> &DescriptorSetBindingBaseBuffer::GetBuffer() const { return m_buffer; }

///

DescriptorSetBindingUniformBuffer::DescriptorSetBindingUniformBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size) : DescriptorSetBindingBaseBuffer {descSet, bindingIdx, buffer, startOffset, size} {}

///

DescriptorSetBindingDynamicUniformBuffer::DescriptorSetBindingDynamicUniformBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size) : DescriptorSetBindingBaseBuffer {descSet, bindingIdx, buffer, startOffset, size} {}

///

DescriptorSetBindingStorageBuffer::DescriptorSetBindingStorageBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size) : DescriptorSetBindingBaseBuffer {descSet, bindingIdx, buffer, startOffset, size} {}

///

DescriptorSetBindingDynamicStorageBuffer::DescriptorSetBindingDynamicStorageBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size) : DescriptorSetBindingBaseBuffer {descSet, bindingIdx, buffer, startOffset, size} {}

///////////////////

std::shared_ptr<SwapDescriptorSet> prosper::SwapDescriptorSet::Create(Window &window, const DescriptorSetInfo &descSetInfo)
{
	auto n = window.GetSwapchainImageCount();
	std::vector<std::shared_ptr<IDescriptorSetGroup>> dsgs;
	dsgs.reserve(n);
	for(auto i = decltype(n) {0u}; i < n; ++i) {
		auto dsg = window.GetContext().CreateDescriptorSetGroup(descSetInfo);
		dsgs.push_back(dsg);
	}
	return std::shared_ptr<SwapDescriptorSet> {new SwapDescriptorSet {window, std::move(dsgs)}};
}
std::shared_ptr<prosper::SwapDescriptorSet> prosper::SwapDescriptorSet::Create(Window &window, std::vector<std::shared_ptr<IDescriptorSetGroup>> &&dsgs)
{
	assert(!dsgs.empty());
    assert(dsgs.size() == window.GetSwapchainImageCount());
	if(dsgs.size() != window.GetSwapchainImageCount())
		throw std::logic_error {"Swap descriptor set dsg count does not match number of swapchain images!"};
	return std::shared_ptr<SwapDescriptorSet> {new SwapDescriptorSet {window, std::move(dsgs)}};
}

prosper::SwapDescriptorSet::SwapDescriptorSet(Window &window, std::vector<std::shared_ptr<IDescriptorSetGroup>> &&dsgs) : m_dsgs {std::move(dsgs)}, m_window {window.shared_from_this()}, m_windowPtr {&window} {}
prosper::IDescriptorSet *prosper::SwapDescriptorSet::operator->()
{
	if(m_window.expired())
		return nullptr;
	auto idx = m_windowPtr->GetLastAcquiredSwapchainImageIndex();
	if(idx >= m_dsgs.size())
		return nullptr;
	return m_dsgs[idx]->GetDescriptorSet();
}
prosper::IDescriptorSet &prosper::SwapDescriptorSet::operator*() { return *operator->(); }
IDescriptorSet &prosper::SwapDescriptorSet::GetDescriptorSet() { return operator*(); }
prosper::IDescriptorSet &prosper::SwapDescriptorSet::GetDescriptorSet(SubDescriptorSetIndex idx)
{
	assert(idx < m_dsgs.size());
	return *m_dsgs[idx]->GetDescriptorSet();
}
void prosper::SwapDescriptorSet::SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingStorageImage(texture, bindingIdx, layerId);
}
void prosper::SwapDescriptorSet::SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingStorageImage(texture, bindingIdx);
}
void prosper::SwapDescriptorSet::SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingTexture(texture, bindingIdx, layerId);
}
void prosper::SwapDescriptorSet::SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingTexture(texture, bindingIdx);
}
void prosper::SwapDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex, uint32_t layerId)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingArrayTexture(texture, bindingIdx, arrayIndex, layerId);
}
void prosper::SwapDescriptorSet::SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingArrayTexture(texture, bindingIdx, arrayIndex);
}
void prosper::SwapDescriptorSet::SetBindingUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingUniformBuffer(buffer, bindingIdx, startOffset, size);
}
void prosper::SwapDescriptorSet::SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingDynamicUniformBuffer(buffer, bindingIdx, startOffset, size);
}
void prosper::SwapDescriptorSet::SetBindingStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->SetBindingStorageBuffer(buffer, bindingIdx, startOffset, size);
}

void prosper::SwapDescriptorSet::SetBindingUniformBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto i = decltype(m_dsgs.size()) {0u}; i < m_dsgs.size(); ++i)
		m_dsgs[i]->GetDescriptorSet()->SetBindingUniformBuffer(buffer.GetBuffer(i), bindingIdx, startOffset, size);
}
void prosper::SwapDescriptorSet::SetBindingDynamicUniformBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto i = decltype(m_dsgs.size()) {0u}; i < m_dsgs.size(); ++i)
		m_dsgs[i]->GetDescriptorSet()->SetBindingDynamicUniformBuffer(buffer.GetBuffer(i), bindingIdx, startOffset, size);
}
void prosper::SwapDescriptorSet::SetBindingStorageBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size)
{
	for(auto i = decltype(m_dsgs.size()) {0u}; i < m_dsgs.size(); ++i)
		m_dsgs[i]->GetDescriptorSet()->SetBindingStorageBuffer(buffer.GetBuffer(i), bindingIdx, startOffset, size);
}
void prosper::SwapDescriptorSet::Update()
{
	for(auto &dsg : m_dsgs)
		dsg->GetDescriptorSet()->Update();
}
