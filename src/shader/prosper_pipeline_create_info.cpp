/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_pipeline_create_info.hpp"

void prosper::BasePipelineCreateInfo::CopyStateFrom(const BasePipelineCreateInfo* in_src_pipeline_create_info_ptr)
{
	m_basePipelineId = in_src_pipeline_create_info_ptr->m_basePipelineId;

	for (auto& current_ds_create_info_item_ptr : in_src_pipeline_create_info_ptr->m_dsCreateInfoItems)
	{
		m_dsCreateInfoItems.push_back(
			std::unique_ptr<prosper::DescriptorSetCreateInfo>(
				new DescriptorSetCreateInfo(*current_ds_create_info_item_ptr.get() )
			)
		);
	}

	m_pushConstantRanges = in_src_pipeline_create_info_ptr->m_pushConstantRanges;

	m_createFlags  = in_src_pipeline_create_info_ptr->m_createFlags;
	m_shaderStages = in_src_pipeline_create_info_ptr->m_shaderStages;

	m_specializationConstantsDataBuffer = in_src_pipeline_create_info_ptr->m_specializationConstantsDataBuffer;
	m_specializationConstantsMap         = in_src_pipeline_create_info_ptr->m_specializationConstantsMap;
}
bool prosper::BasePipelineCreateInfo::AttachPushConstantRange(uint32_t                in_offset,
	uint32_t                in_size,
	ShaderStageFlags in_stages)
{
	bool result = false;

	/* Retrieve pipeline's descriptor and add the specified push constant range */
	const auto new_descriptor = PushConstantRange(in_offset,
		in_size,
		in_stages);

	if (std::find(m_pushConstantRanges.begin(),
		m_pushConstantRanges.end(),
		new_descriptor) != m_pushConstantRanges.end() )
	{
		goto end;
	}

	m_pushConstantRanges.push_back(new_descriptor);

	/* All done */
	result = true;

end:
	return result;
}

bool prosper::BasePipelineCreateInfo::GetShaderStageProperties(
	ShaderStage                  in_shader_stage,
	const ShaderModuleStageEntryPoint** out_opt_result_ptr_ptr) const
{
	bool result                = false;
	auto shader_stage_iterator = m_shaderStages.find(in_shader_stage);

	if (shader_stage_iterator != m_shaderStages.end() )
	{
		if (out_opt_result_ptr_ptr != nullptr)
		{
			*out_opt_result_ptr_ptr = &shader_stage_iterator->second;
		}

		result = true;
	}

	return result;
}

bool prosper::BasePipelineCreateInfo::GetSpecializationConstants(ShaderStage              in_shader_stage,
	const std::vector<SpecializationConstant> ** out_opt_spec_constants_ptr,
	const unsigned char**           out_opt_spec_constants_data_buffer_ptr) const
{
	bool       result          = false;
	const auto sc_map_iterator = m_specializationConstantsMap.find(in_shader_stage);

	if (sc_map_iterator != m_specializationConstantsMap.end() )
	{
		if (out_opt_spec_constants_ptr != nullptr)
		{
			*out_opt_spec_constants_ptr = &sc_map_iterator->second;
		}

		if (out_opt_spec_constants_data_buffer_ptr != nullptr)
		{
			*out_opt_spec_constants_data_buffer_ptr = (m_specializationConstantsDataBuffer.size() > 0) ? &m_specializationConstantsDataBuffer.at(0)
				: nullptr;
		}

		result = true;
	}

	return result;
}

void prosper::BasePipelineCreateInfo::SetDescriptorSetCreateInfo(const std::vector<const DescriptorSetCreateInfo*>* in_ds_create_info_vec_ptr)
{
	const uint32_t n_descriptor_sets = static_cast<uint32_t>(in_ds_create_info_vec_ptr->size() );

	for (uint32_t n_descriptor_set = 0;
		n_descriptor_set < n_descriptor_sets;
		++n_descriptor_set)
	{
		const auto reference_ds_create_info_ptr = in_ds_create_info_vec_ptr->at(n_descriptor_set);

		if (reference_ds_create_info_ptr == nullptr)
		{
			m_dsCreateInfoItems.push_back(
				std::unique_ptr<DescriptorSetCreateInfo>()
			);

			continue;
		}

		m_dsCreateInfoItems.push_back(
			std::unique_ptr<DescriptorSetCreateInfo>(
				new DescriptorSetCreateInfo(*reference_ds_create_info_ptr)
			)
		);
	}
}

bool prosper::BasePipelineCreateInfo::AddSpecializationConstant(ShaderStage in_shader_stage,
	uint32_t           in_constant_id,
	uint32_t           in_n_data_bytes,
	const void*        in_data_ptr)
{
	uint32_t data_buffer_size = 0;
	bool     result           = false;

	if (in_n_data_bytes == 0)
	{
		anvil_assert(!(in_n_data_bytes == 0) );

		goto end;
	}

	if (in_data_ptr == nullptr)
	{
		anvil_assert(!(in_data_ptr == nullptr) );

		goto end;
	}

	/* Append specialization constant data and add a new descriptor. */
	data_buffer_size = static_cast<uint32_t>(m_specializationConstantsDataBuffer.size() );


	anvil_assert(m_specializationConstantsMap.find(in_shader_stage) != m_specializationConstantsMap.end() );

	m_specializationConstantsMap[in_shader_stage].push_back(
		SpecializationConstant(
			in_constant_id,
			in_n_data_bytes,
			data_buffer_size)
	);

	m_specializationConstantsDataBuffer.resize(data_buffer_size + in_n_data_bytes);

	memcpy(&m_specializationConstantsDataBuffer.at(data_buffer_size),
		in_data_ptr,
		in_n_data_bytes);

	/* All done */
	result = true;
end:
	return result;
}

void prosper::BasePipelineCreateInfo::init(const PipelineCreateFlags&                         in_create_flags,
	uint32_t                                                  in_n_shader_module_stage_entrypoints,
	const ShaderModuleStageEntryPoint*                        in_shader_module_stage_entrypoint_ptrs,
	const PipelineID*                                  in_opt_base_pipeline_id_ptr,
	const std::vector<const DescriptorSetCreateInfo*>* in_opt_ds_create_info_vec_ptr)
{
	m_basePipelineId = (in_opt_base_pipeline_id_ptr != nullptr) ? *in_opt_base_pipeline_id_ptr : UINT32_MAX;
	m_createFlags     = in_create_flags;

	if (in_opt_ds_create_info_vec_ptr != nullptr)
	{
		SetDescriptorSetCreateInfo(in_opt_ds_create_info_vec_ptr);
	}

	InitShaderModules(in_n_shader_module_stage_entrypoints,
		in_shader_module_stage_entrypoint_ptrs);
}

void prosper::BasePipelineCreateInfo::InitShaderModules(uint32_t                                  in_n_shader_module_stage_entrypoints,
	const ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs)
{
	for (uint32_t n_shader_module_stage_entrypoint = 0;
		n_shader_module_stage_entrypoint < in_n_shader_module_stage_entrypoints;
		++n_shader_module_stage_entrypoint)
	{
		const auto& shader_module_stage_entrypoint = in_shader_module_stage_entrypoint_ptrs[n_shader_module_stage_entrypoint];

		if (shader_module_stage_entrypoint.stage == prosper::ShaderStage::Unknown)
		{
			continue;
		}

		m_shaderStages               [shader_module_stage_entrypoint.stage] = shader_module_stage_entrypoint;
		m_specializationConstantsMap[shader_module_stage_entrypoint.stage] = std::vector<SpecializationConstant>();
	}
}

/////

prosper::GraphicsPipelineCreateInfo::GraphicsPipelineCreateInfo(const IRenderPass* in_renderpass_ptr,SubPassID in_subpass_id)
	: BasePipelineCreateInfo{}
{
	m_alphaToCoverageEnabled            = false;
	m_alphaToOneEnabled                 = false;
	m_depthBiasClamp                     = 0.0f;
	m_depthBiasConstantFactor           = 0.0f;
	m_depthBiasEnabled                   = false;
	m_depthBiasSlopeFactor              = 1.0f;
	m_depthBoundsTestEnabled            = false;
	m_depthClampEnabled                  = false;
	m_depthClipEnabled                   = true;
	m_depthTestCompareOp                = CompareOp::Always;
	m_depthTestEnabled                   = false;
	m_depthWritesEnabled                 = false;
	m_frontFace                           = FrontFace::CounterClockwise;
	m_logicOp                             = LogicOp::NoOp;
	m_logicOpEnabled                     = false;
	m_maxDepthBounds                     = 1.0f;
	m_minDepthBounds                     = 0.0f;
	m_dynamicScissorBoxesCount              = 0;
	m_dynamicViewportsCount                  = 0;
	m_primitiveRestartEnabled            = false;
	m_rasterizerDiscardEnabled           = false;
	m_sampleLocationsEnabled             = false;
	m_sampleMaskEnabled                  = false;
	m_sampleShadingEnabled               = false;
	m_stencilTestEnabled                 = false;

	m_renderpassPtr = in_renderpass_ptr;
	m_subpassId     = in_subpass_id;

	m_stencilStateBackFace.compareMask = ~0u;
	m_stencilStateBackFace.compareOp   = CompareOp::Always;
	m_stencilStateBackFace.depthFailOp = StencilOp::Keep;
	m_stencilStateBackFace.failOp      = StencilOp::Keep;
	m_stencilStateBackFace.passOp      = StencilOp::Keep;
	m_stencilStateBackFace.reference   = 0;
	m_stencilStateBackFace.writeMask   = ~0u;
	m_stencilStateFrontFace            = m_stencilStateBackFace;

	memset(m_blendConstant.data(),
		0,
		sizeof(m_blendConstant) );

	m_cullMode                  = CullModeFlags::BackBit;
	m_lineWidth                 = 1.0f;
	m_minSampleShading         = 1.0f;
	m_sampleCount               = SampleCountFlags::e1Bit;
	m_polygonMode               = PolygonMode::Fill;
	m_primitiveTopology         = PrimitiveTopology::TriangleList;
	m_sampleMask                = ~0u;
}
bool prosper::GraphicsPipelineCreateInfo::CopyGFXStateFrom(const GraphicsPipelineCreateInfo* in_src_pipeline_create_info_ptr)
{
	bool result = false;

	if (in_src_pipeline_create_info_ptr == nullptr)
	{
		anvil_assert(in_src_pipeline_create_info_ptr != nullptr);

		goto end;
	}

	/* GFX pipeline info-level data */
	m_depthBoundsTestEnabled = in_src_pipeline_create_info_ptr->m_depthBoundsTestEnabled;
	m_depthClipEnabled        = in_src_pipeline_create_info_ptr->m_depthClipEnabled;
	m_maxDepthBounds          = in_src_pipeline_create_info_ptr->m_maxDepthBounds;
	m_minDepthBounds          = in_src_pipeline_create_info_ptr->m_minDepthBounds;

	m_depthBiasEnabled         = in_src_pipeline_create_info_ptr->m_depthBiasEnabled;
	m_depthBiasClamp           = in_src_pipeline_create_info_ptr->m_depthBiasClamp;
	m_depthBiasConstantFactor = in_src_pipeline_create_info_ptr->m_depthBiasConstantFactor;
	m_depthBiasSlopeFactor    = in_src_pipeline_create_info_ptr->m_depthBiasSlopeFactor;

	m_depthTestEnabled         = in_src_pipeline_create_info_ptr->m_depthTestEnabled;
	m_depthTestCompareOp      = in_src_pipeline_create_info_ptr->m_depthTestCompareOp;

	m_enabledDynamicStates = in_src_pipeline_create_info_ptr->m_enabledDynamicStates;

	m_alphaToCoverageEnabled  = in_src_pipeline_create_info_ptr->m_alphaToCoverageEnabled;
	m_alphaToOneEnabled       = in_src_pipeline_create_info_ptr->m_alphaToOneEnabled;
	m_depthClampEnabled        = in_src_pipeline_create_info_ptr->m_depthClampEnabled;
	m_depthWritesEnabled       = in_src_pipeline_create_info_ptr->m_depthWritesEnabled;
	m_logicOpEnabled           = in_src_pipeline_create_info_ptr->m_logicOpEnabled;
	m_primitiveRestartEnabled  = in_src_pipeline_create_info_ptr->m_primitiveRestartEnabled;
	m_rasterizerDiscardEnabled = in_src_pipeline_create_info_ptr->m_rasterizerDiscardEnabled;
	m_sampleLocationsEnabled   = in_src_pipeline_create_info_ptr->m_sampleLocationsEnabled;
	m_sampleMaskEnabled        = in_src_pipeline_create_info_ptr->m_sampleMaskEnabled;
	m_sampleShadingEnabled     = in_src_pipeline_create_info_ptr->m_sampleShadingEnabled;

	m_stencilTestEnabled     = in_src_pipeline_create_info_ptr->m_stencilTestEnabled;
	m_stencilStateBackFace  = in_src_pipeline_create_info_ptr->m_stencilStateBackFace;
	m_stencilStateFrontFace = in_src_pipeline_create_info_ptr->m_stencilStateFrontFace;

	m_bindings                               = in_src_pipeline_create_info_ptr->m_bindings;
	m_blendConstant[0]                      = in_src_pipeline_create_info_ptr->m_blendConstant[0];
	m_blendConstant[1]                      = in_src_pipeline_create_info_ptr->m_blendConstant[1];
	m_blendConstant[2]                      = in_src_pipeline_create_info_ptr->m_blendConstant[2];
	m_blendConstant[3]                      = in_src_pipeline_create_info_ptr->m_blendConstant[3];
	m_cullMode                              = in_src_pipeline_create_info_ptr->m_cullMode;
	m_polygonMode                           = in_src_pipeline_create_info_ptr->m_polygonMode;
	m_frontFace                             = in_src_pipeline_create_info_ptr->m_frontFace;
	m_lineWidth                             = in_src_pipeline_create_info_ptr->m_lineWidth;
	m_logicOp                               = in_src_pipeline_create_info_ptr->m_logicOp;
	m_minSampleShading                     = in_src_pipeline_create_info_ptr->m_minSampleShading;
	m_dynamicScissorBoxesCount                = in_src_pipeline_create_info_ptr->m_dynamicScissorBoxesCount;
	m_dynamicViewportsCount                    = in_src_pipeline_create_info_ptr->m_dynamicViewportsCount;
	m_primitiveTopology                     = in_src_pipeline_create_info_ptr->m_primitiveTopology;
	m_sampleCount                           = in_src_pipeline_create_info_ptr->m_sampleCount;
	m_sampleMask                            = in_src_pipeline_create_info_ptr->m_sampleMask;
	m_scissorBoxes                          = in_src_pipeline_create_info_ptr->m_scissorBoxes;
	m_subpassAttachmentBlendingProperties = in_src_pipeline_create_info_ptr->m_subpassAttachmentBlendingProperties;
	m_viewports                              = in_src_pipeline_create_info_ptr->m_viewports;


	BasePipelineCreateInfo::CopyStateFrom(in_src_pipeline_create_info_ptr);

	result = true;
end:
	return result;
}
std::unique_ptr<prosper::GraphicsPipelineCreateInfo> prosper::GraphicsPipelineCreateInfo::Create(
	const PipelineCreateFlags&        in_create_flags,
	const IRenderPass*                        in_renderpass_ptr,
	SubPassID                                in_subpass_id,
	const ShaderModuleStageEntryPoint&       in_fragment_shader_stage_entrypoint_info,
	const ShaderModuleStageEntryPoint&       in_geometry_shader_stage_entrypoint_info,
	const ShaderModuleStageEntryPoint&       in_tess_control_shader_stage_entrypoint_info,
	const ShaderModuleStageEntryPoint&       in_tess_evaluation_shader_stage_entrypoint_info,
	const ShaderModuleStageEntryPoint&       in_vertex_shader_shader_stage_entrypoint_info,
	const GraphicsPipelineCreateInfo* in_opt_reference_pipeline_info_ptr,
	const PipelineID*                 in_opt_base_pipeline_id_ptr)
{
	std::unique_ptr<GraphicsPipelineCreateInfo> result_ptr(nullptr,
		std::default_delete<prosper::GraphicsPipelineCreateInfo>() );

	result_ptr.reset(
		new GraphicsPipelineCreateInfo(in_renderpass_ptr,
			in_subpass_id)
	);

	if (result_ptr != nullptr)
	{
		const ShaderModuleStageEntryPoint stages[] =
		{
			in_fragment_shader_stage_entrypoint_info,
			in_geometry_shader_stage_entrypoint_info,
			in_tess_control_shader_stage_entrypoint_info,
			in_tess_evaluation_shader_stage_entrypoint_info,
			in_vertex_shader_shader_stage_entrypoint_info
		};

		if (in_opt_reference_pipeline_info_ptr != nullptr)
		{
			if (!result_ptr->CopyGFXStateFrom(in_opt_reference_pipeline_info_ptr) )
			{
				anvil_assert_fail();

				result_ptr.reset();
			}
		}

		result_ptr->init(in_create_flags,
			sizeof(stages) / sizeof(stages[0]),
			stages,
			in_opt_base_pipeline_id_ptr);
	}

	return result_ptr;
}

bool prosper::GraphicsPipelineCreateInfo::AddVertexBinding(
	uint32_t                           in_binding,
	VertexInputRate             in_step_rate,
	uint32_t                           in_stride_in_bytes,
	const uint32_t&                    in_n_attributes,
	const VertexInputAttribute* in_attribute_ptrs,
	uint32_t                           in_divisor)
{
	bool result = false;

#ifdef _DEBUG
	{
		/* Make sure the binding has not already been specified */
		anvil_assert(m_bindings.find(in_binding) == m_bindings.end() );

		/* Make sure the location is not already referred to by already added attributes */
		for (const auto& current_binding : m_bindings)
		{
			for (const auto& current_attribute : current_binding.second.attributes)
			{
				for (uint32_t n_in_attribute = 0;
					n_in_attribute < in_n_attributes;
					++n_in_attribute)
				{
					anvil_assert(current_attribute.location != in_attribute_ptrs[n_in_attribute].location);
				}
			}
		}
	}
#endif

	/* Add a new vertex attribute descriptor. At this point, we do not differentiate between
	* attributes and bindings. Actual Vulkan attributes and bindings will be created at baking
	* time. */
	InternalVertexBinding new_binding(in_divisor,
		in_step_rate,
		in_stride_in_bytes);

	new_binding.attributes.resize(in_n_attributes);

	memcpy(&new_binding.attributes.at(0),
		in_attribute_ptrs,
		in_n_attributes * sizeof(Anvil::VertexInputAttribute) );

	m_bindings[in_binding] = new_binding;

	/* All done */
	result = true;

	return result;
}

bool prosper::GraphicsPipelineCreateInfo::AreDepthWritesEnabled() const
{
	return m_depthWritesEnabled;
}

void prosper::GraphicsPipelineCreateInfo::GetBlendingProperties(const float** out_opt_blend_constant_vec4_ptr_ptr,
	uint32_t*     out_opt_n_blend_attachments_ptr) const
{
	if (out_opt_blend_constant_vec4_ptr_ptr != nullptr)
	{
		*out_opt_blend_constant_vec4_ptr_ptr = m_blendConstant.data();
	}

	if (out_opt_n_blend_attachments_ptr != nullptr)
	{
		*out_opt_n_blend_attachments_ptr = static_cast<uint32_t>(m_subpassAttachmentBlendingProperties.size() );
	}
}

bool prosper::GraphicsPipelineCreateInfo::GetColorBlendAttachmentProperties(SubPassAttachmentID         in_attachment_id,
	bool*                       out_opt_blending_enabled_ptr,
	BlendOp*             out_opt_blend_op_color_ptr,
	BlendOp*             out_opt_blend_op_alpha_ptr,
	BlendFactor*         out_opt_src_color_blend_factor_ptr,
	BlendFactor*         out_opt_dst_color_blend_factor_ptr,
	BlendFactor*         out_opt_src_alpha_blend_factor_ptr,
	BlendFactor*         out_opt_dst_alpha_blend_factor_ptr,
	ColorComponentFlags* out_opt_channel_write_mask_ptr) const
{
	SubPassAttachmentToBlendingPropertiesMap::const_iterator props_iterator;
	bool                                                     result         = false;

	props_iterator = m_subpassAttachmentBlendingProperties.find(in_attachment_id);

	if (props_iterator == m_subpassAttachmentBlendingProperties.end() )
	{
		goto end;
	}

	if (out_opt_blending_enabled_ptr != nullptr)
	{
		*out_opt_blending_enabled_ptr = props_iterator->second.blendEnabled;
	}

	if (out_opt_blend_op_color_ptr != nullptr)
	{
		*out_opt_blend_op_color_ptr = props_iterator->second.blendOpColor;
	}

	if (out_opt_blend_op_alpha_ptr != nullptr)
	{
		*out_opt_blend_op_alpha_ptr = props_iterator->second.blendOpAlpha;
	}

	if (out_opt_src_color_blend_factor_ptr != nullptr)
	{
		*out_opt_src_color_blend_factor_ptr = props_iterator->second.srcColorBlendFactor;
	}

	if (out_opt_dst_color_blend_factor_ptr != nullptr)
	{
		*out_opt_dst_color_blend_factor_ptr = props_iterator->second.dstColorBlendFactor;
	}

	if (out_opt_src_alpha_blend_factor_ptr != nullptr)
	{
		*out_opt_src_alpha_blend_factor_ptr = props_iterator->second.srcAlphaBlendFactor;
	}

	if (out_opt_dst_alpha_blend_factor_ptr != nullptr)
	{
		*out_opt_dst_alpha_blend_factor_ptr = props_iterator->second.dstAlphaBlendFactor;
	}

	if (out_opt_channel_write_mask_ptr != nullptr)
	{
		*out_opt_channel_write_mask_ptr = props_iterator->second.channelWriteMask;
	}

	result = true;
end:
	return result;
}

void prosper::GraphicsPipelineCreateInfo::GetDepthBiasState(bool*  out_opt_is_enabled_ptr,
	float* out_opt_depth_bias_constant_factor_ptr,
	float* out_opt_depth_bias_clamp_ptr,
	float* out_opt_depth_bias_slope_factor_ptr) const
{

	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_depthBiasEnabled;
	}

	if (out_opt_depth_bias_constant_factor_ptr != nullptr)
	{
		*out_opt_depth_bias_constant_factor_ptr = m_depthBiasConstantFactor;
	}

	if (out_opt_depth_bias_clamp_ptr != nullptr)
	{
		*out_opt_depth_bias_clamp_ptr = m_depthBiasClamp;
	}

	if (out_opt_depth_bias_slope_factor_ptr != nullptr)
	{
		*out_opt_depth_bias_slope_factor_ptr = m_depthBiasSlopeFactor;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetDepthBoundsState(bool*  out_opt_is_enabled_ptr,
	float* out_opt_min_depth_bounds_ptr,
	float* out_opt_max_depth_bounds_ptr) const
{
	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_depthBoundsTestEnabled;
	}

	if (out_opt_min_depth_bounds_ptr != nullptr)
	{
		*out_opt_min_depth_bounds_ptr = m_minDepthBounds;
	}

	if (out_opt_max_depth_bounds_ptr != nullptr)
	{
		*out_opt_max_depth_bounds_ptr = m_maxDepthBounds;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetDepthTestState(bool*             out_opt_is_enabled_ptr,
	CompareOp* out_opt_compare_op_ptr) const
{
	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_depthTestEnabled;
	}

	if (out_opt_compare_op_ptr != nullptr)
	{
		*out_opt_compare_op_ptr = m_depthTestCompareOp;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetEnabledDynamicStates(const DynamicState** out_dynamic_states_ptr_ptr,
	uint32_t*                   out_n_dynamic_states_ptr) const
{
	*out_n_dynamic_states_ptr   = static_cast<uint32_t>(m_enabledDynamicStates.size() );
	*out_dynamic_states_ptr_ptr = ((m_enabledDynamicStates.size() > 0) ? &m_enabledDynamicStates.at(0)
		: nullptr);
}

void prosper::GraphicsPipelineCreateInfo::GetGraphicsPipelineProperties(uint32_t*          out_opt_n_scissors_ptr,
	uint32_t*          out_opt_n_viewports_ptr,
	uint32_t*          out_opt_n_vertex_bindings_ptr,
	const IRenderPass** out_opt_renderpass_ptr_ptr,
	SubPassID*         out_opt_subpass_id_ptr) const
{
	if (out_opt_n_scissors_ptr != nullptr)
	{
		*out_opt_n_scissors_ptr = static_cast<uint32_t>(m_scissorBoxes.size() );
	}

	if (out_opt_n_viewports_ptr != nullptr)
	{
		*out_opt_n_viewports_ptr = static_cast<uint32_t>(m_viewports.size() );
	}

	if (out_opt_n_vertex_bindings_ptr != nullptr)
	{
		*out_opt_n_vertex_bindings_ptr = static_cast<uint32_t>(m_bindings.size() );
	}

	if (out_opt_renderpass_ptr_ptr != nullptr)
	{
		*out_opt_renderpass_ptr_ptr = m_renderpassPtr;
	}

	if (out_opt_subpass_id_ptr != nullptr)
	{
		*out_opt_subpass_id_ptr = m_subpassId;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetLogicOpState(bool*           out_opt_is_enabled_ptr,
	LogicOp* out_opt_logic_op_ptr) const
{
	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_logicOpEnabled;
	}

	if (out_opt_logic_op_ptr != nullptr)
	{
		*out_opt_logic_op_ptr = m_logicOp;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetMultisamplingProperties(SampleCountFlags* out_opt_sample_count_ptr,
	const VkSampleMask** out_opt_sample_mask_ptr_ptr) const
{
	if (out_opt_sample_count_ptr != nullptr)
	{
		*out_opt_sample_count_ptr = m_sampleCount;
	}

	if (out_opt_sample_mask_ptr_ptr != nullptr)
	{
		*out_opt_sample_mask_ptr_ptr = &m_sampleMask;
	}
}

uint32_t prosper::GraphicsPipelineCreateInfo::GetDynamicScissorBoxesCount() const
{
	return m_dynamicScissorBoxesCount;
}

uint32_t prosper::GraphicsPipelineCreateInfo::GetDynamicViewportsCount() const
{
	return m_dynamicViewportsCount;
}

uint32_t prosper::GraphicsPipelineCreateInfo::GetScissorBoxesCount() const
{
	return static_cast<uint32_t>(m_scissorBoxes.size() );
}

uint32_t prosper::GraphicsPipelineCreateInfo::GetViewportCount() const
{
	return static_cast<uint32_t>(m_viewports.size() );
}

prosper::PrimitiveTopology prosper::GraphicsPipelineCreateInfo::GetPrimitiveTopology() const
{
	return m_primitiveTopology;
}

void prosper::GraphicsPipelineCreateInfo::GetRasterizationProperties(PolygonMode*   out_opt_polygon_mode_ptr,
	CullModeFlags* out_opt_cull_mode_ptr,
	FrontFace*     out_opt_front_face_ptr,
	float*                out_opt_line_width_ptr) const
{
	if (out_opt_polygon_mode_ptr != nullptr)
	{
		*out_opt_polygon_mode_ptr = m_polygonMode;
	}

	if (out_opt_cull_mode_ptr != nullptr)
	{
		*out_opt_cull_mode_ptr = m_cullMode;
	}

	if (out_opt_front_face_ptr != nullptr)
	{
		*out_opt_front_face_ptr = m_frontFace;
	}

	if (out_opt_line_width_ptr != nullptr)
	{
		*out_opt_line_width_ptr = m_lineWidth;
	}
}

void prosper::GraphicsPipelineCreateInfo::GetSampleShadingState(bool*  out_opt_is_enabled_ptr,
	float* out_opt_min_sample_shading_ptr) const
{
	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_sampleShadingEnabled;
	}

	if (out_opt_min_sample_shading_ptr != nullptr)
	{
		*out_opt_min_sample_shading_ptr = m_minSampleShading;
	}
}

bool prosper::GraphicsPipelineCreateInfo::GetScissorBoxProperties(uint32_t  in_n_scissor_box,
	int32_t*  out_opt_x_ptr,
	int32_t*  out_opt_y_ptr,
	uint32_t* out_opt_width_ptr,
	uint32_t* out_opt_height_ptr) const
{
	bool                      result                = false;
	const InternalScissorBox* scissor_box_props_ptr = nullptr;

	if (m_scissorBoxes.find(in_n_scissor_box) == m_scissorBoxes.end() )
	{
		anvil_assert(!(m_scissorBoxes.find(in_n_scissor_box) == m_scissorBoxes.end()) );

		goto end;
	}
	else
	{
		scissor_box_props_ptr = &m_scissorBoxes.at(in_n_scissor_box);
	}

	if (out_opt_x_ptr != nullptr)
	{
		*out_opt_x_ptr = scissor_box_props_ptr->x;
	}

	if (out_opt_y_ptr != nullptr)
	{
		*out_opt_y_ptr = scissor_box_props_ptr->y;
	}

	if (out_opt_width_ptr != nullptr)
	{
		*out_opt_width_ptr = scissor_box_props_ptr->width;
	}

	if (out_opt_height_ptr != nullptr)
	{
		*out_opt_height_ptr = scissor_box_props_ptr->height;
	}

	result = true;
end:
	return result;
}

void prosper::GraphicsPipelineCreateInfo::GetStencilTestProperties(bool*             out_opt_is_enabled_ptr,
	StencilOp* out_opt_front_stencil_fail_op_ptr,
	StencilOp* out_opt_front_stencil_pass_op_ptr,
	StencilOp* out_opt_front_stencil_depth_fail_op_ptr,
	CompareOp* out_opt_front_stencil_compare_op_ptr,
	uint32_t*         out_opt_front_stencil_compare_mask_ptr,
	uint32_t*         out_opt_front_stencil_write_mask_ptr,
	uint32_t*         out_opt_front_stencil_reference_ptr,
	StencilOp* out_opt_back_stencil_fail_op_ptr,
	StencilOp* out_opt_back_stencil_pass_op_ptr,
	StencilOp* out_opt_back_stencil_depth_fail_op_ptr,
	CompareOp* out_opt_back_stencil_compare_op_ptr,
	uint32_t*         out_opt_back_stencil_compare_mask_ptr,
	uint32_t*         out_opt_back_stencil_write_mask_ptr,
	uint32_t*         out_opt_back_stencil_reference_ptr) const
{
	if (out_opt_is_enabled_ptr != nullptr)
	{
		*out_opt_is_enabled_ptr = m_stencilTestEnabled;
	}

	if (out_opt_front_stencil_fail_op_ptr != nullptr)
	{
		*out_opt_front_stencil_fail_op_ptr = static_cast<StencilOp>(m_stencilStateFrontFace.failOp);
	}

	if (out_opt_front_stencil_pass_op_ptr != nullptr)
	{
		*out_opt_front_stencil_pass_op_ptr = static_cast<StencilOp>(m_stencilStateFrontFace.passOp);
	}

	if (out_opt_front_stencil_depth_fail_op_ptr != nullptr)
	{
		*out_opt_front_stencil_depth_fail_op_ptr = static_cast<StencilOp>(m_stencilStateFrontFace.depthFailOp);
	}

	if (out_opt_front_stencil_compare_op_ptr != nullptr)
	{
		*out_opt_front_stencil_compare_op_ptr = static_cast<CompareOp>(m_stencilStateFrontFace.compareOp);
	}

	if (out_opt_front_stencil_compare_mask_ptr != nullptr)
	{
		*out_opt_front_stencil_compare_mask_ptr = m_stencilStateFrontFace.compareMask;
	}

	if (out_opt_front_stencil_write_mask_ptr != nullptr)
	{
		*out_opt_front_stencil_write_mask_ptr = m_stencilStateFrontFace.writeMask;
	}

	if (out_opt_front_stencil_reference_ptr != nullptr)
	{
		*out_opt_front_stencil_reference_ptr = m_stencilStateFrontFace.reference;
	}

	if (out_opt_back_stencil_fail_op_ptr != nullptr)
	{
		*out_opt_back_stencil_fail_op_ptr = static_cast<StencilOp>(m_stencilStateBackFace.failOp);
	}

	if (out_opt_back_stencil_pass_op_ptr != nullptr)
	{
		*out_opt_back_stencil_pass_op_ptr = static_cast<StencilOp>(m_stencilStateBackFace.passOp);
	}

	if (out_opt_back_stencil_depth_fail_op_ptr != nullptr)
	{
		*out_opt_back_stencil_depth_fail_op_ptr = static_cast<StencilOp>(m_stencilStateBackFace.depthFailOp);
	}

	if (out_opt_back_stencil_compare_op_ptr != nullptr)
	{
		*out_opt_back_stencil_compare_op_ptr = static_cast<CompareOp>(m_stencilStateBackFace.compareOp);
	}

	if (out_opt_back_stencil_compare_mask_ptr != nullptr)
	{
		*out_opt_back_stencil_compare_mask_ptr = m_stencilStateBackFace.compareMask;
	}

	if (out_opt_back_stencil_write_mask_ptr != nullptr)
	{
		*out_opt_back_stencil_write_mask_ptr = m_stencilStateBackFace.writeMask;
	}

	if (out_opt_back_stencil_reference_ptr != nullptr)
	{
		*out_opt_back_stencil_reference_ptr = m_stencilStateBackFace.reference;
	}
}

bool prosper::GraphicsPipelineCreateInfo::GetVertexBindingProperties(uint32_t                     in_n_vertex_binding,
	uint32_t*                    out_opt_binding_ptr,
	uint32_t*                    out_opt_stride_ptr,
	VertexInputRate*      out_opt_rate_ptr,
	uint32_t*                    out_opt_n_attributes_ptr,
	const VertexInputAttribute** out_opt_attributes_ptr_ptr,
	uint32_t*                    out_opt_divisor_ptr) const
{
	auto                         binding_iterator = m_bindings.begin();
	const InternalVertexBinding* binding_ptr      = nullptr;
	bool                         result           = false;

	if (m_bindings.size() < in_n_vertex_binding)
	{
		goto end;
	}
	else
	{
		uint32_t n_item           = 0;

		while (n_item != in_n_vertex_binding)
		{
			binding_iterator++;
			n_item          ++;

			if (binding_iterator == m_bindings.end() )
			{
				goto end;
			}
		}

		binding_ptr = &binding_iterator->second;
	}

	if (out_opt_binding_ptr != nullptr)
	{
		*out_opt_binding_ptr = binding_iterator->first;
	}

	if (out_opt_stride_ptr != nullptr)
	{
		*out_opt_stride_ptr = binding_ptr->strideInBytes;
	}

	if (out_opt_rate_ptr != nullptr)
	{
		*out_opt_rate_ptr = binding_ptr->rate;
	}

	if (out_opt_n_attributes_ptr != nullptr)
	{
		*out_opt_n_attributes_ptr = static_cast<uint32_t>(binding_ptr->attributes.size() );
	}

	if (out_opt_attributes_ptr_ptr != nullptr)
	{
		*out_opt_attributes_ptr_ptr = (binding_ptr->attributes.size() > 0) ? &binding_ptr->attributes.at(0)
			: nullptr;
	}

	if (out_opt_divisor_ptr != nullptr)
	{
		*out_opt_divisor_ptr = binding_ptr->divisor;
	}

	/* All done */
	result = true;
end:
	return result;
}

bool prosper::GraphicsPipelineCreateInfo::GetViewportProperties(uint32_t in_n_viewport,
	float*   out_opt_origin_x_ptr,
	float*   out_opt_origin_y_ptr,
	float*   out_opt_width_ptr,
	float*   out_opt_height_ptr,
	float*   out_opt_min_depth_ptr,
	float*   out_opt_max_depth_ptr) const
{
	bool                    result              = false;
	const InternalViewport* viewport_props_ptr;

	if (m_viewports.find(in_n_viewport) == m_viewports.end() )
	{
		anvil_assert(!(m_viewports.find(in_n_viewport) == m_viewports.end()) );

		goto end;
	}
	else
	{
		viewport_props_ptr = &m_viewports.at(in_n_viewport);
	}

	if (out_opt_origin_x_ptr != nullptr)
	{
		*out_opt_origin_x_ptr = viewport_props_ptr->originX;
	}

	if (out_opt_origin_y_ptr != nullptr)
	{
		*out_opt_origin_y_ptr = viewport_props_ptr->originY;
	}

	if (out_opt_width_ptr != nullptr)
	{
		*out_opt_width_ptr = viewport_props_ptr->width;
	}

	if (out_opt_height_ptr != nullptr)
	{
		*out_opt_height_ptr = viewport_props_ptr->height;
	}

	if (out_opt_min_depth_ptr != nullptr)
	{
		*out_opt_min_depth_ptr = viewport_props_ptr->minDepth;
	}

	if (out_opt_max_depth_ptr != nullptr)
	{
		*out_opt_max_depth_ptr = viewport_props_ptr->maxDepth;
	}

	result = true;
end:
	return result;
}

bool prosper::GraphicsPipelineCreateInfo::IsAlphaToCoverageEnabled() const
{
	return m_alphaToCoverageEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsAlphaToOneEnabled() const
{
	return m_alphaToOneEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsDepthClampEnabled() const
{
	return m_depthClampEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsDepthClipEnabled() const
{
	return m_depthClipEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsPrimitiveRestartEnabled() const
{
	return m_primitiveRestartEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsRasterizerDiscardEnabled() const
{
	return m_rasterizerDiscardEnabled;
}

bool prosper::GraphicsPipelineCreateInfo::IsSampleMaskEnabled() const
{
	return m_sampleMaskEnabled;
}

void prosper::GraphicsPipelineCreateInfo::SetBlendingProperties(const float* in_blend_constant_vec4)
{
	memcpy(m_blendConstant.data(),
		in_blend_constant_vec4,
		sizeof(m_blendConstant) );
}

void prosper::GraphicsPipelineCreateInfo::SetColorBlendAttachmentProperties(SubPassAttachmentID        in_attachment_id,
	bool                       in_blending_enabled,
	BlendOp             in_blend_op_color,
	BlendOp             in_blend_op_alpha,
	BlendFactor         in_src_color_blend_factor,
	BlendFactor         in_dst_color_blend_factor,
	BlendFactor         in_src_alpha_blend_factor,
	BlendFactor         in_dst_alpha_blend_factor,
	ColorComponentFlags in_channel_write_mask)
{
	BlendingProperties* attachment_blending_props_ptr = &m_subpassAttachmentBlendingProperties[in_attachment_id];

	attachment_blending_props_ptr->blendEnabled          = in_blending_enabled;
	attachment_blending_props_ptr->blendOpAlpha         = in_blend_op_alpha;
	attachment_blending_props_ptr->blendOpColor         = in_blend_op_color;
	attachment_blending_props_ptr->channelWriteMask     = in_channel_write_mask;
	attachment_blending_props_ptr->dstAlphaBlendFactor = in_dst_alpha_blend_factor;
	attachment_blending_props_ptr->dstColorBlendFactor = in_dst_color_blend_factor;
	attachment_blending_props_ptr->srcAlphaBlendFactor = in_src_alpha_blend_factor;
	attachment_blending_props_ptr->srcColorBlendFactor = in_src_color_blend_factor;
}

void prosper::GraphicsPipelineCreateInfo::SetMultisamplingProperties(SampleCountFlags in_sample_count,
	float                      in_min_sample_shading,
	const VkSampleMask         in_sample_mask)
{
	m_minSampleShading = in_min_sample_shading;
	m_sampleCount       = in_sample_count;
	m_sampleMask        = in_sample_mask;
}

void prosper::GraphicsPipelineCreateInfo::SetDynamicScissorBoxesCount(uint32_t in_n_dynamic_scissor_boxes)
{
	m_dynamicScissorBoxesCount = in_n_dynamic_scissor_boxes;
}

void prosper::GraphicsPipelineCreateInfo::SetDynamicViewportCount(uint32_t in_n_dynamic_viewports)
{
	m_dynamicViewportsCount = in_n_dynamic_viewports;
}

void prosper::GraphicsPipelineCreateInfo::SetPrimitiveTopology(PrimitiveTopology in_primitive_topology)
{
	m_primitiveTopology = in_primitive_topology;
}

void prosper::GraphicsPipelineCreateInfo::SetRasterizationProperties(PolygonMode   in_polygon_mode,
	CullModeFlags in_cull_mode,
	FrontFace     in_front_face,
	float                in_line_width)
{
	m_cullMode    = in_cull_mode;
	m_frontFace   = in_front_face;
	m_lineWidth   = in_line_width;
	m_polygonMode = in_polygon_mode;
}

void prosper::GraphicsPipelineCreateInfo::SetScissorBoxProperties(uint32_t in_n_scissor_box,
	int32_t  in_x,
	int32_t  in_y,
	uint32_t in_width,
	uint32_t in_height)
{
	m_scissorBoxes[in_n_scissor_box] = InternalScissorBox(in_x,
		in_y,
		in_width,
		in_height);
}

void prosper::GraphicsPipelineCreateInfo::SetStencilTestProperties(bool             in_update_front_face_state,
	StencilOp in_stencil_fail_op,
	StencilOp in_stencil_pass_op,
	StencilOp in_stencil_depth_fail_op,
	CompareOp in_stencil_compare_op,
	uint32_t         in_stencil_compare_mask,
	uint32_t         in_stencil_write_mask,
	uint32_t         in_stencil_reference)
{
	StencilOpState* const stencil_op_state_ptr = (in_update_front_face_state) ? &m_stencilStateFrontFace
		: &m_stencilStateBackFace;

	stencil_op_state_ptr->compareMask = in_stencil_compare_mask;
	stencil_op_state_ptr->compareOp   = in_stencil_compare_op;
	stencil_op_state_ptr->depthFailOp = in_stencil_depth_fail_op;
	stencil_op_state_ptr->failOp      = in_stencil_fail_op;
	stencil_op_state_ptr->passOp      = in_stencil_pass_op;
	stencil_op_state_ptr->reference   = in_stencil_reference;
	stencil_op_state_ptr->writeMask   = in_stencil_write_mask;
}

void prosper::GraphicsPipelineCreateInfo::SetViewportProperties(uint32_t in_n_viewport,
	float    in_origin_x,
	float    in_origin_y,
	float    in_width,
	float    in_height,
	float    in_min_depth,
	float    in_max_depth)
{
	m_viewports[in_n_viewport] = InternalViewport(in_origin_x,
		in_origin_y,
		in_width,
		in_height,
		in_min_depth,
		in_max_depth);
}

void prosper::GraphicsPipelineCreateInfo::ToggleAlphaToCoverage(bool in_should_enable)
{
	m_alphaToCoverageEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleAlphaToOne(bool in_should_enable)
{
	m_alphaToOneEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthBias(bool  in_should_enable,
	float in_depth_bias_constant_factor,
	float in_depth_bias_clamp,
	float in_depth_bias_slope_factor)
{
	m_depthBiasConstantFactor = in_depth_bias_constant_factor;
	m_depthBiasClamp           = in_depth_bias_clamp;
	m_depthBiasEnabled         = in_should_enable;
	m_depthBiasSlopeFactor    = in_depth_bias_slope_factor;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthBoundsTest(bool  in_should_enable,
	float in_min_depth_bounds,
	float in_max_depth_bounds)
{
	m_depthBoundsTestEnabled = in_should_enable;
	m_maxDepthBounds          = in_max_depth_bounds;
	m_minDepthBounds          = in_min_depth_bounds;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthClamp(bool in_should_enable)
{
	m_depthClampEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthClip(bool in_should_enable)
{
	m_depthClipEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthTest(bool             in_should_enable,
	CompareOp in_compare_op)
{
	m_depthTestEnabled    = in_should_enable;
	m_depthTestCompareOp = in_compare_op;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDepthWrites(bool in_should_enable)
{
	m_depthWritesEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleDynamicState(bool                       in_should_enable,
	const DynamicState& in_dynamic_state)
{
	if (in_should_enable)
	{
		if (std::find(m_enabledDynamicStates.begin(),
			m_enabledDynamicStates.end  (),
			in_dynamic_state) == m_enabledDynamicStates.end() )
		{
			m_enabledDynamicStates.push_back(in_dynamic_state);
		}
	}
	else
	{
		auto iterator = std::find(m_enabledDynamicStates.begin(),
			m_enabledDynamicStates.end  (),
			in_dynamic_state);

		if (iterator != m_enabledDynamicStates.end() )
		{
			m_enabledDynamicStates.erase(iterator);
		}
	}
}

void prosper::GraphicsPipelineCreateInfo::ToggleDynamicStates(bool                       in_should_enable,
	const DynamicState* in_dynamic_states_ptr,
	const uint32_t&            in_n_dynamic_states)
{
	for (uint32_t n_dynamic_state = 0;
		n_dynamic_state < in_n_dynamic_states;
		++n_dynamic_state)
	{
		const auto current_state = in_dynamic_states_ptr[n_dynamic_state];

		ToggleDynamicState(in_should_enable,
			current_state);
	}
}

void prosper::GraphicsPipelineCreateInfo::ToggleDynamicStates(bool                                    in_should_enable,
	const std::vector<DynamicState>& in_dynamic_states)
{
	for (const auto& current_state : in_dynamic_states)
	{
		ToggleDynamicState(in_should_enable,
			current_state);
	}
}

void prosper::GraphicsPipelineCreateInfo::ToggleLogicOp(bool           in_should_enable,
	LogicOp in_logic_op)
{
	m_logicOp         = in_logic_op;
	m_logicOpEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::TogglePrimitiveRestart(bool in_should_enable)
{
	m_primitiveRestartEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleRasterizerDiscard(bool in_should_enable)
{
	m_rasterizerDiscardEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleSampleMask(bool in_should_enable)
{
	m_sampleMaskEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleSampleShading(bool in_should_enable)
{
	m_sampleShadingEnabled = in_should_enable;
}

void prosper::GraphicsPipelineCreateInfo::ToggleStencilTest(bool in_should_enable)
{
	m_stencilTestEnabled = in_should_enable;
}
