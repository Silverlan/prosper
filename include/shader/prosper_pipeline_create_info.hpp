/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PIPELINE_CREATE_INFO_HPP__
#define __PROSPER_PIPELINE_CREATE_INFO_HPP__

#include "prosper_structs.hpp"
#include <memory>
#include <string>
#include <array>
#include <unordered_map>

namespace prosper
{
	class IRenderPass;
	class DLLPROSPER BasePipelineCreateInfo
	{
	public:
		BasePipelineCreateInfo(const BasePipelineCreateInfo&)=delete;
		BasePipelineCreateInfo &operator=(const BasePipelineCreateInfo&)=delete;
		void CopyStateFrom(const BasePipelineCreateInfo* in_src_pipeline_create_info_ptr);
		bool AllowsDerivatives() const
		{
			return (m_createFlags & PipelineCreateFlags::AllowDerivativesBit) != PipelineCreateFlags::None;
		}

		bool AttachPushConstantRange(uint32_t                in_offset,
			uint32_t                in_size,
			ShaderStageFlags in_stages);

		/* Returns != UINT32_MAX if this pipeline is a derivative pipeline, or UINT32_MAX otherwise. */
		PipelineID GetBasePipelineId() const
		{
			return m_basePipelineId;
		}

		const PipelineCreateFlags& GetCreateFlags() const
		{
			return m_createFlags;
		}

		const std::vector<std::unique_ptr<DescriptorSetCreateInfo>>* GetDsCreateInfoItems() const
		{
			return &m_dsCreateInfoItems;
		}

		const char* GetName() const
		{
			return m_name.c_str();
		}

		const std::vector<PushConstantRange>& GetPushConstantRanges() const
		{
			return m_pushConstantRanges;
		}

		/** Returns shader stage information for each stage, as specified at creation time */
		bool GetShaderStageProperties(ShaderStage                  in_shader_stage,
			const ShaderModuleStageEntryPoint** out_opt_result_ptr_ptr) const;

		bool GetSpecializationConstants(ShaderStage              in_shader_stage,
			const std::vector<SpecializationConstant>** out_opt_spec_constants_ptr,
			const unsigned char**           out_opt_spec_constants_data_buffer_ptr) const;

		bool HasOptimizationsDisabled() const
		{
			return (m_createFlags & PipelineCreateFlags::DisableOptimizationBit) != PipelineCreateFlags::None;
		}

		void SetDescriptorSetCreateInfo(const std::vector<const DescriptorSetCreateInfo*>* in_ds_create_info_vec_ptr);

		void SetName(const std::string& in_name)
		{
			m_name = in_name;
		}
	protected:
		BasePipelineCreateInfo()=default;
		virtual bool AddSpecializationConstant(ShaderStage in_shader_stage,uint32_t           in_constant_id,uint32_t           in_n_data_bytes,const void*        in_data_ptr);
		void init      (const PipelineCreateFlags&                         in_create_flags,
			uint32_t                                                  in_n_shader_module_stage_entrypoints,
			const ShaderModuleStageEntryPoint*                        in_shader_module_stage_entrypoint_ptrs,
			const PipelineID*                                  in_opt_base_pipeline_id_ptr   = nullptr,
			const std::vector<const DescriptorSetCreateInfo*>* in_opt_ds_create_info_vec_ptr = nullptr);
	private:
		void InitShaderModules(uint32_t                                  in_n_shader_module_stage_entrypoints,
			const ShaderModuleStageEntryPoint* in_shader_module_stage_entrypoint_ptrs);
		using ShaderStageToSpecializationConstantsMap = std::unordered_map<ShaderStage,std::vector<SpecializationConstant>>;

		/* Protected variables */
		PipelineID m_basePipelineId;

		std::vector<std::unique_ptr<DescriptorSetCreateInfo>> m_dsCreateInfoItems;
		std::string m_name;
		std::vector<PushConstantRange> m_pushConstantRanges;
		std::vector<unsigned char> m_specializationConstantsDataBuffer;
		ShaderStageToSpecializationConstantsMap m_specializationConstantsMap;

		PipelineCreateFlags m_createFlags;
		std::map<ShaderStage, ShaderModuleStageEntryPoint> m_shaderStages;
	};

	class GraphicsPipelineCreateInfo
		: public BasePipelineCreateInfo
	{
	public:
		GraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo&)=delete;
		GraphicsPipelineCreateInfo &operator=(const GraphicsPipelineCreateInfo&)=delete;
		/* Public functions */
		static std::unique_ptr<GraphicsPipelineCreateInfo> Create      (const PipelineCreateFlags&        in_create_flags,
			const IRenderPass*                        in_renderpass_ptr,
			SubPassID                                in_subpass_id,
			const ShaderModuleStageEntryPoint&       in_fragment_shader_stage_entrypoint_info,
			const ShaderModuleStageEntryPoint&       in_geometry_shader_stage_entrypoint_info,
			const ShaderModuleStageEntryPoint&       in_tess_control_shader_stage_entrypoint_info,
			const ShaderModuleStageEntryPoint&       in_tess_evaluation_shader_stage_entrypoint_info,
			const ShaderModuleStageEntryPoint&       in_vertex_shader_shader_stage_entrypoint_info,
			const GraphicsPipelineCreateInfo* in_opt_reference_pipeline_info_ptr = nullptr,
			const PipelineID*                 in_opt_base_pipeline_id_ptr        = nullptr);

		~GraphicsPipelineCreateInfo()=default;

		bool AddSpecializationConstant(ShaderStage in_shader_stage,
			uint32_t           in_constant_id,
			uint32_t           in_n_data_bytes,
			const void*        in_data_ptr) final
		{
			if (in_shader_stage != ShaderStage::Fragment                &&
				in_shader_stage != ShaderStage::Geometry                &&
				in_shader_stage != ShaderStage::TessellationControl    &&
				in_shader_stage != ShaderStage::TessellationEvaluation &&
				in_shader_stage != ShaderStage::Vertex)
			{
				return false;
			}

			return BasePipelineCreateInfo::AddSpecializationConstant(in_shader_stage,
				in_constant_id,
				in_n_data_bytes,
				in_data_ptr);
		}
		bool CopyGFXStateFrom(const GraphicsPipelineCreateInfo* in_src_pipeline_create_info_ptr);

		/** Adds a new vertex binding descriptor to the specified graphics pipeline, along with one or more attributes
		*  that consume the binding. This data will be used at baking time to configure input vertex attribute & bindings
		*  for the Vulkan pipeline object.
		*
		*  @param in_binding                Index to use for the new binding.
		*  @param in_stride_in_bytes        Stride of the vertex attribute data.
		*  @param in_step_rate              Step rate to use for the vertex attribute data.
		*  @param in_n_attributes           Number of items available for reading under @param in_attribute_ptrs. Must be
		*                                   at least 1.
		*  @param in_attribute_ptrs         Vertex attributes to associate with the new bindings.
		*  @param in_divisor                Vertex attribute divisor to use for the binding. Any value different than 1
		*                                   is only supported for devices with VK_EXT_vertex_attribute_divisor extension
		*                                   enabled.
		*
		*  @return true if successful, false otherwise.
		**/
		bool AddVertexBinding(uint32_t                           in_binding,
			VertexInputRate             in_step_rate,
			uint32_t                           in_stride_in_bytes,
			const uint32_t&                    in_n_attributes,
			const VertexInputAttribute* in_attribute_ptrs,
			uint32_t                           in_divisor        = 1);

		/** Tells whether depth writes have been enabled. **/
		bool AreDepthWritesEnabled() const;

		/** Retrieves blending properties defined.
		*
		*  @param out_opt_blend_constant_vec4_ptr If not NULL, deref will be assigned to a ptr holding four float values
		*                                         representing the blend constant.
		*  @param out_opt_n_blend_attachments_ptr If not NULL, deref will be set to the number of blend
		*                                         attachments the graphics pipeline supports.
		**/
		void GetBlendingProperties(const float** out_opt_blend_constant_vec4_ptr_ptr,
			uint32_t*     out_opt_n_blend_attachments_ptr) const;

		/** Retrieves color blend attachment properties specified for a given subpass attachment.
		*
		*  @param in_attachment_id                   ID of the attachment the query is being made for.
		*  @param out_opt_blending_enabled_ptr       If not null, deref will be set to true if blending has been
		*                                            enabled for this pipeline, or to false otherwise.
		*  @param out_opt_blend_op_color_ptr         If not null, deref will be set to the specified attachment's
		*                                            color blend op.
		*  @param out_opt_blend_op_alpha_ptr         If not null, deref will be set to the specified attachment's
		*                                            alpha blend op.
		*  @param out_opt_src_color_blend_factor_ptr If not null, deref will be set to the specified attachment's
		*                                            source color blend factor.
		*  @param out_opt_dst_color_blend_factor_ptr If not null, deref will be set to the specified attachment's
		*                                            destination color blend factor.
		*  @param out_opt_src_alpha_blend_factor_ptr If not null, deref will be set to the specified attachment's
		*                                            source alpha blend factor.
		*  @param out_opt_dst_alpha_blend_factor_ptr If not null, deref will be set to the specified attachment's
		*                                            destination alpha blend factor.
		*  @param out_opt_channel_write_mask_ptr     If not null, deref will be set to the specified attachment's
		*                                            write mask.
		*
		*  @return true if successful, false otherwise.
		*/
		bool GetColorBlendAttachmentProperties(SubPassAttachmentID         in_attachment_id,
			bool*                       out_opt_blending_enabled_ptr,
			BlendOp*             out_opt_blend_op_color_ptr,
			BlendOp*             out_opt_blend_op_alpha_ptr,
			BlendFactor*         out_opt_src_color_blend_factor_ptr,
			BlendFactor*         out_opt_dst_color_blend_factor_ptr,
			BlendFactor*         out_opt_src_alpha_blend_factor_ptr,
			BlendFactor*         out_opt_dst_alpha_blend_factor_ptr,
			ColorComponentFlags* out_opt_channel_write_mask_ptr) const;

		void GetDepthBiasState(bool*  out_opt_is_enabled_ptr,
			float* out_opt_depth_bias_constant_factor_ptr,
			float* out_opt_depth_bias_clamp_ptr,
			float* out_opt_depth_bias_slope_factor_ptr) const;


		void GetDepthBoundsState(bool*  out_opt_is_enabled_ptr,
			float* out_opt_min_depth_bounds_ptr,
			float* out_opt_max_depth_bounds_ptr) const;


		void GetDepthTestState(bool*             out_opt_is_enabled_ptr,
			CompareOp* out_opt_compare_op_ptr) const;

		void GetEnabledDynamicStates(const DynamicState** out_dynamic_states_ptr_ptr,
			uint32_t*                   out_n_dynamic_states_ptr) const;

		void GetGraphicsPipelineProperties(uint32_t*          out_opt_n_scissors_ptr,
			uint32_t*          out_opt_n_viewports_ptr,
			uint32_t*          out_opt_n_vertex_bindings_ptr,
			const IRenderPass** out_opt_renderpass_ptr_ptr,
			SubPassID*         out_opt_subpass_id_ptr) const;

		void GetLogicOpState(bool*           out_opt_is_enabled_ptr,
			LogicOp* out_opt_logic_op_ptr) const;

		void GetMultisamplingProperties(SampleCountFlags* out_opt_sample_count_ptr,
			const SampleMask**        out_opt_sample_mask_ptr_ptr) const;

		/** Tells the number of dynamic scissor boxes. **/
		uint32_t GetDynamicScissorBoxesCount() const;

		/** Tells the number of dynamic viewports. **/
		uint32_t GetDynamicViewportsCount() const;

		uint32_t GetScissorBoxesCount() const;

		uint32_t GetViewportCount() const;

		/** Tells what primitive topology has been specified for this instance. **/
		PrimitiveTopology GetPrimitiveTopology() const;

		void GetRasterizationProperties(PolygonMode*   out_opt_polygon_mode_ptr,
			CullModeFlags* out_opt_cull_mode_ptr,
			FrontFace*     out_opt_front_face_ptr,
			float*                out_opt_line_width_ptr) const;

		const IRenderPass* GetRenderPass() const
		{
			return m_renderpassPtr;
		}

		void GetSampleShadingState(bool*  out_opt_is_enabled_ptr,
			float* out_opt_min_sample_shading_ptr) const;

		bool GetScissorBoxProperties(uint32_t  in_n_scissor_box,
			int32_t*  out_opt_x_ptr,
			int32_t*  out_opt_y_ptr,
			uint32_t* out_opt_width_ptr,
			uint32_t* out_opt_height_ptr) const;

		void GetStencilTestProperties(bool*             out_opt_is_enabled_ptr,
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
			uint32_t*         out_opt_back_stencil_reference_ptr) const;

		const SubPassID& GetSubpassId() const
		{
			return m_subpassId;
		}

		bool GetVertexBindingProperties(uint32_t                     in_n_vertex_binding,
			uint32_t*                    out_opt_binding_ptr        = nullptr,
			uint32_t*                    out_opt_stride_ptr         = nullptr,
			VertexInputRate*      out_opt_rate_ptr           = nullptr,
			uint32_t*                    out_opt_n_attributes_ptr   = nullptr,
			const VertexInputAttribute** out_opt_attributes_ptr_ptr = nullptr,
			uint32_t*                    out_opt_divisor_ptr        = nullptr) const;

		bool GetViewportProperties(uint32_t in_n_viewport,
			float*   out_opt_origin_x_ptr,
			float*   out_opt_origin_y_ptr,
			float*   out_opt_width_ptr,
			float*   out_opt_height_ptr,
			float*   out_opt_min_depth_ptr,
			float*   out_opt_max_depth_ptr) const;

		/** Tells if alpha-to-coverage mode has been enabled. **/
		bool IsAlphaToCoverageEnabled() const;

		/** Tells if alpha-to-one mode has been enabled. **/
		bool IsAlphaToOneEnabled() const;

		/** Tells whether depth clamping has been enabled. **/
		bool IsDepthClampEnabled() const;

		/** Tells whether depth clipping has been enabled. **/
		bool IsDepthClipEnabled() const;

		/** Tells whether primitive restart mode has been enabled. **/
		bool IsPrimitiveRestartEnabled() const;

		/** Tells whether rasterizer discard has been enabled. */
		bool IsRasterizerDiscardEnabled() const;

		/** Tells whether sample mask has been enabled. */
		bool IsSampleMaskEnabled() const;

		/** Sets a new blend constant.
		*
		*  @param in_blend_constant_vec4 4 floats, specifying the constant. Must not be nullptr.
		*
		**/
		void SetBlendingProperties(const float* in_blend_constant_vec4);

		void SetColorBlendAttachmentProperties(SubPassAttachmentID        in_attachment_id,
			bool                       in_blending_enabled,
			BlendOp             in_blend_op_color,
			BlendOp             in_blend_op_alpha,
			BlendFactor         in_src_color_blend_factor,
			BlendFactor         in_dst_color_blend_factor,
			BlendFactor         in_src_alpha_blend_factor,
			BlendFactor         in_dst_alpha_blend_factor,
			ColorComponentFlags in_channel_write_mask);

		void SetMultisamplingProperties(SampleCountFlags in_sample_count,
			float                      in_min_sample_shading,
			const SampleMask         in_sample_mask);

		/** Updates the number of scissor boxes to be used, when dynamic scissor state is enabled.
		*
		*  @param in_n_dynamic_scissor_boxes As per description.
		*/
		void SetDynamicScissorBoxesCount(uint32_t in_n_dynamic_scissor_boxes);

		/** Updates the number of viewports to be used, when dynamic viewport state is enabled.
		*
		*  @param in_n_dynamic_viewports As per description.
		*/
		void SetDynamicViewportCount(uint32_t in_n_dynamic_viewports);

		/** Sets primitive topology. */
		void SetPrimitiveTopology(PrimitiveTopology in_primitive_topology);

		/** Sets a number of rasterization properties to be used for the pipeline.
		*
		*  @param in_polygon_mode Polygon mode to use.
		*  @param in_cull_mode    Cull mode to use.
		*  @param in_front_face   Front face to use.
		*  @param in_line_width   Line width to use.
		**/
		void SetRasterizationProperties(PolygonMode   in_polygon_mode,
			CullModeFlags in_cull_mode,
			FrontFace     in_front_face,
			float                in_line_width);

		/** Sets properties of a scissor box at the specified index.
		*
		*  If @param n_scissor_box is larger than 1, all previous scissor boxes must also be defined
		*  prior to creating a pipeline. Number of scissor boxes must match the number of viewports
		*  defined for the pipeline.
		*
		*  @param in_n_scissor_box Index of the scissor box to be updated.
		*  @param in_x             X offset of the scissor box.
		*  @param in_y             Y offset of the scissor box.
		*  @param in_width         Width of the scissor box.
		*  @param in_height        Height of the scissor box.
		**/
		void SetScissorBoxProperties(uint32_t in_n_scissor_box,
			int32_t  in_x,
			int32_t  in_y,
			uint32_t in_width,
			uint32_t in_height);

		/** Sets a number of stencil test properties.
		*
		*  @param in_update_front_face_state true if the front face stencil states should be updated; false to update the
		*                                    back stencil states instead.
		*  @param in_stencil_fail_op         Stencil fail operation to use.
		*  @param in_stencil_pass_op         Stencil pass operation to use.
		*  @param in_stencil_depth_fail_op   Stencil depth fail operation to use.
		*  @param in_stencil_compare_op      Stencil compare operation to use.
		*  @param in_stencil_compare_mask    Stencil compare mask to use.
		*  @param in_stencil_write_mask      Stencil write mask to use.
		*  @param in_stencil_reference       Stencil reference value to use.
		**/
		void SetStencilTestProperties(bool             in_update_front_face_state,
			StencilOp in_stencil_fail_op,
			StencilOp in_stencil_pass_op,
			StencilOp in_stencil_depth_fail_op,
			CompareOp in_stencil_compare_op,
			uint32_t         in_stencil_compare_mask,
			uint32_t         in_stencil_write_mask,
			uint32_t         in_stencil_reference);

		/** Sets properties of a viewport at the specified index.
		*
		*  If @param in_n_viewport is larger than 1, all previous viewports must also be defined
		*  prior to creating a pipeline. Number of scissor boxes must match the number of viewports
		*  defined for the pipeline.
		*
		*  @param in_n_viewport Index of the viewport, whose properties should be changed.
		*  @param in_origin_x   X offset to use for the viewport's origin.
		*  @param in_origin_y   Y offset to use for the viewport's origin.
		*  @param in_width      Width of the viewport.
		*  @param in_height     Height of the viewport.
		*  @param in_min_depth  Minimum depth value to use for the viewport.
		*  @param in_max_depth  Maximum depth value to use for the viewport.
		**/
		void SetViewportProperties(uint32_t in_n_viewport,
			float    in_origin_x,
			float    in_origin_y,
			float    in_width,
			float    in_height,
			float    in_min_depth,
			float    in_max_depth);

		/** Enables or disables the "alpha to coverage" test.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleAlphaToCoverage(bool in_should_enable);

		/** Enables or disables the "alpha to one" test.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleAlphaToOne(bool in_should_enable);

		/** Enables or disables the "depth bias" mode and updates related state values.
		*
		*  @param in_should_enable              true to enable the mode; false to disable it.
		*  @param in_depth_bias_constant_factor Depth bias constant factor to use for the mode.
		*  @param in_depth_bias_clamp           Depth bias clamp to use for the mode.
		*  @param in_depth_bias_slope_factor    Slope scale for the depth bias to use for the mode.
		*/
		void ToggleDepthBias(bool  in_should_enable,
			float in_depth_bias_constant_factor,
			float in_depth_bias_clamp,
			float in_depth_bias_slope_factor);

		/** Enables or disables the "depth bounds" test and updates related state values.
		*
		*  @param in_should_enable    true to enable the test; false to disable it.
		*  @param in_min_depth_bounds Minimum boundary value to use for the test.
		*  @param in_max_depth_bounds Maximum boundary value to use for the test. 
		*/
		void ToggleDepthBoundsTest(bool  in_should_enable,
			float in_min_depth_bounds,
			float in_max_depth_bounds);

		/** Enables or disables the "depth clamp" test.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleDepthClamp(bool in_should_enable);

		/** Enables or disables the "depth clip" test.
		*
		*  Requires VK_EXT_depth_clip_enable extension support.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleDepthClip(bool in_should_enable);

		/** Enables or disables the depth test and updates related state values.
		*
		*  @param in_should_enable true to enable the mode; false to disable it.
		*  @param in_compare_op    Compare operation to use for the test.
		*/
		void ToggleDepthTest(bool             in_should_enable,
			CompareOp in_compare_op);

		/** Enables or disables depth writes.
		*
		*  @param in_should_enable true to enable the writes; false to disable them.
		*/
		void ToggleDepthWrites(bool in_should_enable);

		/** Enables or disables the specified dynamic states.
		*
		*  @param in_should_enable      true to enable the dynamic state(s) specified by @param in_dynamic_state_bits, false
		*                               disable them if already enabled.
		*  @param in_dynamic_states_ptr Pointer to an array of dynamic states to disable or enable. Must not be nullptr.
		*  @param in_n_dynamic_states   Number of array items available for reading under @param in_dynamic_states_ptr
		**/
		void ToggleDynamicState (bool                                    in_should_enable,
			const DynamicState&              in_dynamic_state);
		void ToggleDynamicStates(bool                                    in_should_enable,
			const DynamicState*              in_dynamic_states_ptr,
			const uint32_t&                         in_n_dynamic_states);
		void ToggleDynamicStates(bool                                    in_should_enable,
			const std::vector<DynamicState>& in_dynamic_states);

		/** Enables or disables logic ops and specifies which logic op should be used.
		*
		*  @param in_should_enable true to enable the mode; false to disable it.
		*  @param in_logic_op      Logic operation type to use.
		*/
		void ToggleLogicOp(bool           in_should_enable,
			LogicOp in_logic_op);

		/** Enables or disables the "primitive restart" mode.
		*
		*  @param in_should_enable true to enable the mode; false to disable it.
		*/
		void TogglePrimitiveRestart(bool in_should_enable);

		/** Enables or disables the "rasterizer discard" mode.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleRasterizerDiscard(bool in_should_enable);

		/** Enables or disables the sample mask.
		*
		*  Note: make sure to configure the sample mask using set_multisampling_properties() if you intend to use it.
		*
		*  Note: disabling sample mask will make the manager set VkPipelineMultisampleStateCreateInfo::pSampleMask to a non-null
		*        value at pipeline creation time.
		*
		*  @param in_should_enable true to enable sample mask; false to disable it.
		*/
		void ToggleSampleMask(bool in_should_enable);

		/** Enables or disables the "per-sample shading" mode.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleSampleShading(bool in_should_enable);

		/** Enables or disables the stencil test.
		*
		*  @param in_should_enable true to enable the test; false to disable it.
		*/
		void ToggleStencilTest(bool in_should_enable);

	private:
		/* Private type definitions */

		/* Defines blending properties for a single subpass attachment. */
		struct BlendingProperties
		{
			ColorComponentFlags channelWriteMask;

			bool               blendEnabled;
			BlendOp     blendOpAlpha;
			BlendOp     blendOpColor;
			BlendFactor dstAlphaBlendFactor;
			BlendFactor dstColorBlendFactor;
			BlendFactor srcAlphaBlendFactor;
			BlendFactor srcColorBlendFactor;

			/** Constructor. */
			BlendingProperties()
			{
				blendEnabled           = false;
				blendOpAlpha          = BlendOp::Add;
				blendOpColor          = BlendOp::Add;
				channelWriteMask      = ColorComponentFlags::ABit |
					ColorComponentFlags::BBit |
					ColorComponentFlags::GBit |
					ColorComponentFlags::RBit;
				dstAlphaBlendFactor  = BlendFactor::One;
				dstColorBlendFactor  = BlendFactor::One;
				srcAlphaBlendFactor  = BlendFactor::One;
				srcColorBlendFactor  = BlendFactor::One;
			}

			/** Comparison operator
			*
			*  @param in Object to compare against.
			*
			*  @return true if all states match, false otherwise.
			**/
			bool operator==(const BlendingProperties& in) const
			{
				return (in.blendEnabled          == blendEnabled          &&
					in.blendOpAlpha         == blendOpAlpha         &&
					in.blendOpColor         == blendOpColor         &&
					in.channelWriteMask     == channelWriteMask     &&
					in.dstAlphaBlendFactor == dstAlphaBlendFactor &&
					in.dstColorBlendFactor == dstColorBlendFactor &&
					in.srcAlphaBlendFactor == srcAlphaBlendFactor &&
					in.srcColorBlendFactor == srcColorBlendFactor);
			}
		};

		typedef std::map<SubPassAttachmentID, BlendingProperties> SubPassAttachmentToBlendingPropertiesMap;

		/** Defines a single scissor box
		*
		* This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
		* descriptors at baking time instead.
		*/
		struct InternalScissorBox
		{
			int32_t  x;
			int32_t  y;
			uint32_t width;
			uint32_t height;

			/* Constructor. Should only be used by STL. */
			InternalScissorBox()
			{
				x      = 0;
				y      = 0;
				width  = 32u;
				height = 32u;
			}

			/* Constructor
			*
			* @param in_x      X offset of the scissor box
			* @param in_y      Y offset of the scissor box
			* @param in_width  Width of the scissor box
			* @param in_height Height of the scissor box
			**/
			InternalScissorBox(int32_t  in_x,
				int32_t  in_y,
				uint32_t in_width,
				uint32_t in_height)
			{
				height = in_height;
				width  = in_width;
				x      = in_x;
				y      = in_y;
			}
		};

		/** Defines a single viewport
		*
		* This descriptor is not exposed to the Vulkan implementation. It is used to form Vulkan-specific
		* descriptors at baking time instead.
		*/
		typedef struct InternalViewport
		{
			float height;
			float maxDepth;
			float minDepth;
			float originX;
			float originY;
			float width;

			/* Constructor. Should only be used by STL */
			InternalViewport()
			{
				height    =  32.0f;
				maxDepth =  1.0f;
				minDepth =  0.0f;
				originX  =  0.0f;
				originY  =  0.0f;
				width     =  32.0f;
			}

			/* Constructor.
			*
			* @param in_origin_x  Origin X of the viewport.
			* @param in_origin_y  Origin Y of the viewport.
			* @param in_width     Width of the viewport.
			* @param in_height    Height of the viewport.
			* @param in_min_depth Minimum depth value the viewport should use.
			* @param in_max_depth Maximum depth value the viewport should use.
			**/
			InternalViewport(float in_origin_x,
				float in_origin_y,
				float in_width,
				float in_height,
				float in_min_depth,
				float in_max_depth)
			{
				height    = in_height;
				maxDepth = in_max_depth;
				minDepth = in_min_depth;
				originX  = in_origin_x;
				originY  = in_origin_y;
				width     = in_width;
			}
		} InternalViewport;

		/** A vertex attribute descriptor. This descriptor is not exposed to the Vulkan implementation. Instead,
		*  its members are used to create Vulkan input attribute & binding descriptors at baking time.
		*/
		typedef struct InternalVertexBinding
		{
			uint32_t                          divisor;
			VertexInputRate            rate;
			uint32_t                          strideInBytes;
			std::vector<VertexInputAttribute> attributes;

			/** Dummy constructor. Should only be used by STL. */
			InternalVertexBinding()
			{
				divisor         = UINT32_MAX;
				rate            = VertexInputRate::Unknown;
				strideInBytes = UINT32_MAX;
			}

			/** Constructor.
			*
			*  @param in_divisor         Vertexattribute divisor.
			*  @param in_rate            Step rate.
			*  @param in_stride_in_bytes Stride in bytes.
			**/
			InternalVertexBinding(uint32_t               in_divisor,
				VertexInputRate in_rate,
				uint32_t               in_stride_in_bytes)
			{
				divisor         = in_divisor;
				rate            = in_rate;
				strideInBytes = in_stride_in_bytes;
			}
		} InternalVertexBinding;

		typedef std::map<uint32_t, InternalScissorBox>    InternalScissorBoxes;
		typedef std::map<uint32_t, InternalVertexBinding> InternalVertexBindings;
		typedef std::map<uint32_t, InternalViewport>      InternalViewports;

		/* Private functions */
		GraphicsPipelineCreateInfo(const IRenderPass* in_renderpass_ptr,SubPassID in_subpass_id);

		/* Private variables */
		bool m_depthClipEnabled;

		bool m_depthBoundsTestEnabled;
		float m_maxDepthBounds;
		float m_minDepthBounds;

		bool m_depthBiasEnabled;
		float m_depthBiasClamp;
		float m_depthBiasConstantFactor;
		float m_depthBiasSlopeFactor;

		bool m_depthTestEnabled;
		CompareOp m_depthTestCompareOp;

		std::vector<DynamicState> m_enabledDynamicStates;

		bool m_alphaToCoverageEnabled;
		bool m_alphaToOneEnabled;
		bool m_depthClampEnabled;
		bool m_depthWritesEnabled;
		bool m_logicOpEnabled;
		bool m_primitiveRestartEnabled;
		bool m_rasterizerDiscardEnabled;
		bool m_sampleLocationsEnabled;
		bool m_sampleMaskEnabled;
		bool m_sampleShadingEnabled;

		bool m_stencilTestEnabled;
		StencilOpState m_stencilStateBackFace;
		StencilOpState m_stencilStateFrontFace;

		InternalVertexBindings m_bindings;
		std::array<float,4> m_blendConstant;
		CullModeFlags m_cullMode;
		PolygonMode m_polygonMode;
		FrontFace m_frontFace;
		float m_lineWidth;
		LogicOp m_logicOp;
		float m_minSampleShading;
		uint32_t m_dynamicScissorBoxesCount;
		uint32_t m_dynamicViewportsCount;
		PrimitiveTopology m_primitiveTopology;
		SampleCountFlags m_sampleCount;
		SampleMask m_sampleMask;
		InternalScissorBoxes m_scissorBoxes;
		SubPassAttachmentToBlendingPropertiesMap m_subpassAttachmentBlendingProperties;
		InternalViewports m_viewports;

		const IRenderPass *m_renderpassPtr;
		SubPassID m_subpassId;
	};

	class ComputePipelineCreateInfo
		: public BasePipelineCreateInfo
	{
	public:
		/* Public functions */
		static std::unique_ptr<ComputePipelineCreateInfo> Create      (const PipelineCreateFlags&  in_create_flags,
			const ShaderModuleStageEntryPoint& in_compute_shader_stage_entrypoint_info,
			const PipelineID*           in_opt_base_pipeline_id_ptr = nullptr);

		/** Adds a new specialization constant.
		*
		*  @param in_constant_id  ID of the specialization constant to assign data for.
		*  @param in_n_data_bytes Number of bytes under @param in_data_ptr to assign to the specialization constant.
		*  @param in_data_ptr     A buffer holding the data to be assigned to the constant. Must hold at least
		*                         @param in_n_data_bytes bytes that will be read by the function.
		*
		*  @return true if successful, false otherwise.
		**/
		bool add_specialization_constant(uint32_t    in_constant_id,
			uint32_t    in_n_data_bytes,
			const void* in_data_ptr)
		{
			return BasePipelineCreateInfo::AddSpecializationConstant(ShaderStage::Compute,
				in_constant_id,
				in_n_data_bytes,
				in_data_ptr);
		}

		~ComputePipelineCreateInfo();

	private:
		ComputePipelineCreateInfo();
	};
};

#endif
