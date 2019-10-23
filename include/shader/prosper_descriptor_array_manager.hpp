/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DESCRIPTOR_ARRAY_MANAGER_HPP__
#define __PROSPER_DESCRIPTOR_ARRAY_MANAGER_HPP__

#include "prosper_definitions.hpp"
#include <memory>
#include <queue>
#include <optional>
#include <functional>
#include <unordered_map>
#include <misc/types.h>
#include <sharedutils/functioncallback.h>

#undef max

namespace Anvil {class DescriptorSet;};
namespace prosper
{
	class DescriptorSet;
	class DescriptorSetGroup;
	class Context;
	class DLLPROSPER DescriptorArrayManager
		: public std::enable_shared_from_this<DescriptorArrayManager>
	{
	public:
		template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(prosper::Context &context,Anvil::ShaderStageFlags shaderStages);
		template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(const std::shared_ptr<prosper::DescriptorSetGroup> &matArrayDsg,uint32_t bindingIndex);
		using ArrayIndex = uint32_t;
		static const auto INVALID_ARRAY_INDEX = std::numeric_limits<ArrayIndex>::max();

		virtual ~DescriptorArrayManager()=default;
		void RemoveItem(ArrayIndex index);
	protected:
		DescriptorArrayManager(
			const std::shared_ptr<prosper::DescriptorSetGroup> &matArrayDsg,ArrayIndex maxArrayLayers,uint32_t bindingIndex=0
		);
		std::optional<ArrayIndex> AddItem(const std::function<bool(prosper::DescriptorSet&,ArrayIndex,uint32_t)> &fAddBinding);
		virtual void Initialize(prosper::Context &context) {}
	private:
		std::optional<ArrayIndex> PopFreeIndex();
		void PushFreeIndex(ArrayIndex index);
		std::shared_ptr<prosper::DescriptorSetGroup> m_dsgArray = nullptr;
		uint32_t m_bindingIndex = 0;
		std::queue<ArrayIndex> m_freeIndices = {};
		ArrayIndex m_nextIndex = 0;
		ArrayIndex m_maxArrayLayers = 0;
	};
};

template<class TDescriptorArrayManager>
	std::shared_ptr<TDescriptorArrayManager> prosper::DescriptorArrayManager::Create(prosper::Context &context,Anvil::ShaderStageFlags shaderStages)
{
	auto &dev = context.GetDevice();
	auto maxArrayLayers = dev.get_physical_device_properties().core_vk1_0_properties_ptr->limits.max_image_array_layers;
	auto matArrayDsg = prosper::util::create_descriptor_set_group(dev,{
		{
			prosper::Shader::DescriptorSetInfo::Binding {
				Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
				shaderStages,
				maxArrayLayers,
				0
			}
		}
	});
	return Create(matArrayDsg,0);
}

template<class TDescriptorArrayManager>
	std::shared_ptr<TDescriptorArrayManager> prosper::DescriptorArrayManager::Create(const std::shared_ptr<prosper::DescriptorSetGroup> &matArrayDsg,uint32_t bindingIndex)
{
	uint32_t arraySize;
	auto *dsInfo = matArrayDsg->GetAnvilDescriptorSetGroup().get_descriptor_set_create_info(0);
	if(dsInfo == nullptr || dsInfo->get_binding_properties_by_binding_index(bindingIndex,nullptr,&arraySize) == false)
		return nullptr;
	auto arrayManager =  std::shared_ptr<TDescriptorArrayManager>{new TDescriptorArrayManager{matArrayDsg,arraySize}};
	arrayManager->Initialize(matArrayDsg->GetContext());
	return arrayManager;
}

#endif
