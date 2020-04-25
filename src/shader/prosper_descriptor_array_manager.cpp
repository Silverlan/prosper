/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_descriptor_array_manager.hpp"
#include <prosper_descriptor_set_group.hpp>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <shader/prosper_shader.hpp>

using namespace prosper;

DescriptorArrayManager::DescriptorArrayManager(const std::shared_ptr<prosper::IDescriptorSetGroup> &matArrayDsg,ArrayIndex maxArrayLayers,uint32_t bindingIndex)
	: m_dsgArray{matArrayDsg},m_maxArrayLayers{maxArrayLayers},m_bindingIndex{bindingIndex}
{}

void DescriptorArrayManager::PushFreeIndex(ArrayIndex index) {m_freeIndices.push(index);}

std::optional<DescriptorArrayManager::ArrayIndex> DescriptorArrayManager::PopFreeIndex()
{
	if(m_freeIndices.empty() == false)
	{
		auto index = m_freeIndices.front();
		m_freeIndices.pop();
		return index;
	}
	if(m_nextIndex >= m_maxArrayLayers)
		return {};
	return m_nextIndex++;
}

std::optional<DescriptorArrayManager::ArrayIndex> DescriptorArrayManager::AddItem(const std::function<bool(prosper::IDescriptorSet&,ArrayIndex,uint32_t)> &fAddBinding)
{
	auto index = PopFreeIndex();
	if(index.has_value() == false)
		return {};
	if(fAddBinding(*m_dsgArray->GetDescriptorSet(),*index,m_bindingIndex) == false)
	{
		PushFreeIndex(*index);
		return {};
	}
	return index;
}
void DescriptorArrayManager::RemoveItem(ArrayIndex index) {PushFreeIndex(index);}
