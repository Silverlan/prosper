/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PR_PROSPER_VK_DESCRIPTOR_SET_GROUP_HPP__
#define __PR_PROSPER_VK_DESCRIPTOR_SET_GROUP_HPP__

#include "prosper_descriptor_set_group.hpp"
#include <wrappers/descriptor_set_group.h>

namespace prosper
{
	class DLLPROSPER VlkDescriptorSetGroup
		: public IDescriptorSetGroup
	{
	public:
		static std::shared_ptr<VlkDescriptorSetGroup> Create(IPrContext &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg,const std::function<void(IDescriptorSetGroup&)> &onDestroyedCallback=nullptr);
		virtual ~VlkDescriptorSetGroup() override;

		Anvil::DescriptorSetGroup &GetAnvilDescriptorSetGroup() const;
		Anvil::DescriptorSetGroup &operator*();
		const Anvil::DescriptorSetGroup &operator*() const;
		Anvil::DescriptorSetGroup *operator->();
		const Anvil::DescriptorSetGroup *operator->() const;
	protected:
		VlkDescriptorSetGroup(IPrContext &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg);
		std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> m_descriptorSetGroup = nullptr;
	};

	class DLLPROSPER VlkDescriptorSet
		: public IDescriptorSet
	{
	public:
		VlkDescriptorSet(VlkDescriptorSetGroup &dsg,Anvil::DescriptorSet &ds);

		Anvil::DescriptorSet &GetAnvilDescriptorSet() const;
		Anvil::DescriptorSet &operator*();
		const Anvil::DescriptorSet &operator*() const;
		Anvil::DescriptorSet *operator->();
		const Anvil::DescriptorSet *operator->() const;

		virtual bool Update() override;
		virtual bool SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId) override;
		virtual bool SetBindingStorageImage(prosper::Texture &texture,uint32_t bindingIdx) override;
		virtual bool SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t layerId) override;
		virtual bool SetBindingTexture(prosper::Texture &texture,uint32_t bindingIdx) override;
		virtual bool SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex,uint32_t layerId) override;
		virtual bool SetBindingArrayTexture(prosper::Texture &texture,uint32_t bindingIdx,uint32_t arrayIndex) override;
		virtual bool SetBindingUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max()) override;
		virtual bool SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max()) override;
		virtual bool SetBindingStorageBuffer(prosper::IBuffer &buffer,uint32_t bindingIdx,uint64_t startOffset=0ull,uint64_t size=std::numeric_limits<uint64_t>::max()) override;
	private:
		Anvil::DescriptorSet &m_descSet;
	};
};

#endif
