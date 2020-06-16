/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __VK_UTIL_HPP__
#define __VK_UTIL_HPP__

namespace Anvil
{
	class DescriptorSetCreateInfo;
};
namespace prosper
{
	struct DescriptorSetInfo;
	std::unique_ptr<Anvil::DescriptorSetCreateInfo> ToAnvilDescriptorSetInfo(const DescriptorSetInfo &descSetInfo);
};

#endif
