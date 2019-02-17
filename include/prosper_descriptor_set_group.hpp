/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DESCRIPTOR_SET_GROUP_HPP__
#define __PROSPER_DESCRIPTOR_SET_GROUP_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/descriptor_set_group.h>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class DLLPROSPER DescriptorSetGroup
		: public ContextObject,
		public std::enable_shared_from_this<DescriptorSetGroup>
	{
	public:
		static std::shared_ptr<DescriptorSetGroup> Create(Context &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg,const std::function<void(DescriptorSetGroup&)> &onDestroyedCallback=nullptr);
		virtual ~DescriptorSetGroup() override;
		Anvil::DescriptorSetGroup &GetAnvilDescriptorSetGroup() const;
		Anvil::DescriptorSetGroup &operator*();
		const Anvil::DescriptorSetGroup &operator*() const;
		Anvil::DescriptorSetGroup *operator->();
		const Anvil::DescriptorSetGroup *operator->() const;
	protected:
		DescriptorSetGroup(Context &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg);
		std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> m_descriptorSetGroup = nullptr;
	};
};
#pragma warning(pop)

#endif
