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
#include <sharedutils/functioncallback.h>
#include <prosper_descriptor_set_group.hpp>

#undef max

namespace prosper
{
	class IDescriptorSet;
	class IDescriptorSetGroup;
	class IPrContext;
	class DLLPROSPER DescriptorArrayManager
		: public std::enable_shared_from_this<DescriptorArrayManager>
	{
	public:
		template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(prosper::IPrContext &context,prosper::ShaderStageFlags shaderStages);
		template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg,uint32_t bindingIndex);
		using ArrayIndex = uint32_t;
		static const auto INVALID_ARRAY_INDEX = std::numeric_limits<ArrayIndex>::max();

		virtual ~DescriptorArrayManager()=default;
		void RemoveItem(ArrayIndex index);
	protected:
		DescriptorArrayManager(
			const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg,ArrayIndex maxArrayLayers,uint32_t bindingIndex=0
		);
		std::optional<ArrayIndex> AddItem(const std::function<bool(prosper::IDescriptorSet&,ArrayIndex,uint32_t)> &fAddBinding);
		virtual void Initialize(prosper::IPrContext &context) {}
	private:
		std::optional<ArrayIndex> PopFreeIndex();
		void PushFreeIndex(ArrayIndex index);
		std::shared_ptr<prosper::IDescriptorSetGroup> m_dsgArray = nullptr;
		uint32_t m_bindingIndex = 0;
		std::queue<ArrayIndex> m_freeIndices = {};
		ArrayIndex m_nextIndex = 0;
		ArrayIndex m_maxArrayLayers = 0;
	};
};

template<class TDescriptorArrayManager>
	std::shared_ptr<TDescriptorArrayManager> prosper::DescriptorArrayManager::Create(prosper::IPrContext &context,prosper::ShaderStageFlags shaderStages)
{
	auto limits = prosper::util::get_physical_device_limits(context);
	auto maxArrayLayers = limits.maxImageArrayLayers;
	auto matArrayDsg = context.CreateDescriptorSetGroup({
		{
			prosper::DescriptorSetInfo::Binding {
				prosper::DescriptorType::CombinedImageSampler,
				shaderStages,
				maxArrayLayers,
				0
			}
		}
	});
	return Create(matArrayDsg,0u);
}

template<class TDescriptorArrayManager>
	std::shared_ptr<TDescriptorArrayManager> prosper::DescriptorArrayManager::Create(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg,uint32_t bindingIndex)
{
	uint32_t arraySize;
	auto &createInfo = matArrayDsg->GetDescriptorSetCreateInfo();
	if(createInfo.GetBindingPropertiesByBindingIndex(bindingIndex,nullptr,&arraySize) == false)
		return nullptr;
	auto arrayManager =  std::shared_ptr<TDescriptorArrayManager>{new TDescriptorArrayManager{matArrayDsg,arraySize}};
	arrayManager->Initialize(matArrayDsg->GetContext());
	return arrayManager;
}

#endif
