/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "vk_util.hpp"
#include "shader/prosper_shader.hpp"
#include <misc/descriptor_set_create_info.h>

using namespace prosper;

std::unique_ptr<Anvil::DescriptorSetCreateInfo> prosper::ToAnvilDescriptorSetInfo(const DescriptorSetInfo &descSetInfo)
{
	auto dsInfo = Anvil::DescriptorSetCreateInfo::create();
	for(auto &binding : descSetInfo.bindings)
	{
		dsInfo->add_binding(
			binding.bindingIndex,static_cast<Anvil::DescriptorType>(binding.type),binding.descriptorArraySize,
			static_cast<Anvil::ShaderStageFlagBits>(binding.shaderStages)
		);
	}
	return dsInfo;
}
