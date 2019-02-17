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
}

DescriptorSetGroup::~DescriptorSetGroup()
{
	auto numSets = m_descriptorSetGroup->get_n_descriptor_sets();
	for(auto i=decltype(numSets){0};i<numSets;++i)
		prosper::debug::deregister_debug_object(m_descriptorSetGroup->get_descriptor_set(i));
}

Anvil::DescriptorSetGroup &DescriptorSetGroup::GetAnvilDescriptorSetGroup() const {return *m_descriptorSetGroup;}
Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() {return *m_descriptorSetGroup;}
const Anvil::DescriptorSetGroup &DescriptorSetGroup::operator*() const {return const_cast<DescriptorSetGroup*>(this)->operator*();}
Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() {return m_descriptorSetGroup.get();}
const Anvil::DescriptorSetGroup *DescriptorSetGroup::operator->() const {return const_cast<DescriptorSetGroup*>(this)->operator->();}
