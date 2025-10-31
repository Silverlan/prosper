// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:shader_system.descriptor_array_manager;

export import :context;
export import :descriptor_set_group;

#undef max

export {
	namespace prosper {
		namespace detail {
			struct DLLPROSPER DescriptorSetInfoBinding;
		};
		class IDescriptorSet;
		class IDescriptorSetGroup;
		class IPrContext;
		class DLLPROSPER DescriptorArrayManager : public std::enable_shared_from_this<DescriptorArrayManager> {
		public:
			template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(prosper::IPrContext &context, const char *setName, const char *bindingName, prosper::ShaderStageFlags shaderStages);
			template<class TDescriptorArrayManager>
			static std::shared_ptr<TDescriptorArrayManager> Create(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg, uint32_t bindingIndex);
			using ArrayIndex = uint32_t;
			static const auto INVALID_ARRAY_INDEX = std::numeric_limits<ArrayIndex>::max();

			virtual ~DescriptorArrayManager() = default;
			void RemoveItem(ArrayIndex index);
		protected:
			DescriptorArrayManager(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg, ArrayIndex maxArrayLayers, uint32_t bindingIndex = 0);
			std::optional<ArrayIndex> AddItem(const std::function<bool(prosper::IDescriptorSet &, ArrayIndex, uint32_t)> &fAddBinding);
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

		template<class TDescriptorArrayManager>
		std::shared_ptr<TDescriptorArrayManager> DescriptorArrayManager::Create(prosper::IPrContext &context, const char *setName, const char *bindingName, prosper::ShaderStageFlags shaderStages)
		{
			auto limits = context.GetPhysicalDeviceLimits();
			auto maxArrayLayers = limits.maxImageArrayLayers;
			auto createInfo = prosper::DescriptorSetCreateInfo::Create(setName);
			createInfo->AddBinding(bindingName, 0u, prosper::DescriptorType::CombinedImageSampler, maxArrayLayers, shaderStages);
			auto matArrayDsg = context.CreateDescriptorSetGroup(*createInfo);
			return Create<TDescriptorArrayManager>(matArrayDsg, 0u);
		}

		template<class TDescriptorArrayManager>
		std::shared_ptr<TDescriptorArrayManager> DescriptorArrayManager::Create(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg, uint32_t bindingIndex)
		{
			uint32_t arraySize;
			auto &createInfo = matArrayDsg->GetDescriptorSetCreateInfo();
			if(createInfo.GetBindingPropertiesByBindingIndex(bindingIndex, nullptr, &arraySize) == false)
				return nullptr;
			auto arrayManager = std::shared_ptr<TDescriptorArrayManager> {new TDescriptorArrayManager {matArrayDsg, arraySize}};
			arrayManager->Initialize(matArrayDsg->GetContext());
			return arrayManager;
		}
	};
}
