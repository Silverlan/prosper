/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DEBUG_LOOKUP_MAP_HPP__
#define __PROSPER_DEBUG_LOOKUP_MAP_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include <memory>
#include <unordered_map>

namespace prosper
{
	class VlkImage;
	class VlkImageView;
	class VlkSampler;
	class VkBuffer;
	class VlkCommandBuffer;
	class VlkRenderPass;
	class VlkFramebuffer;
	class VlkDescriptorSetGroup;
	class Shader;
	namespace debug
	{
		enum class ObjectType : uint32_t
		{
			Image = 0u,
			ImageView,
			Sampler,
			Buffer,
			CommandBuffer,
			RenderPass,
			Framebuffer,
			DescriptorSet,
			Pipeline
		};
		struct ShaderPipelineInfo
		{
			Shader *shader = nullptr;
			uint32_t pipelineIdx = std::numeric_limits<uint32_t>::max();
		};
		DLLPROSPER void set_debug_mode_enabled(bool b);
		DLLPROSPER bool is_debug_mode_enabled();
		DLLPROSPER void register_debug_object(void *vkPtr,void *objPtr,ObjectType type);
		DLLPROSPER void register_debug_shader_pipeline(void *vkPtr,const ShaderPipelineInfo &pipelineInfo);
		DLLPROSPER void deregister_debug_object(void *vkPtr);

		DLLPROSPER void *get_object(void *vkObj,ObjectType &type);
		DLLPROSPER VlkImage *get_image(vk::Image vkImage);
		DLLPROSPER VlkImageView *get_image_view(vk::ImageView vkImageView);
		DLLPROSPER VlkSampler *get_sampler(vk::Sampler vkSampler);
		DLLPROSPER VkBuffer *get_buffer(vk::Buffer vkBuffer);
		DLLPROSPER VlkCommandBuffer *get_command_buffer(vk::CommandBuffer vkBuffer);
		DLLPROSPER VlkRenderPass *get_render_pass(vk::RenderPass vkBuffer);
		DLLPROSPER VlkFramebuffer *get_framebuffer(vk::Framebuffer vkBuffer);
		DLLPROSPER VlkDescriptorSetGroup *get_descriptor_set_group(vk::DescriptorSet vkBuffer);
		DLLPROSPER ShaderPipelineInfo *get_shader_pipeline(vk::Pipeline vkPipeline);
	};
};

#endif
