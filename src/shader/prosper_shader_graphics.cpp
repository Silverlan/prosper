/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "prosper_util.hpp"
#include "prosper_context.hpp"
#include "image/prosper_render_target.hpp"
#include "image/prosper_texture.hpp"
#include "image/prosper_image_view.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"
#include <wrappers/command_buffer.h>
#include <wrappers/buffer.h>
#include <misc/descriptor_set_create_info.h>
#include <misc/buffer_create_info.h>
#include <misc/image_create_info.h>
#include <misc/image_view_create_info.h>
#include <sharedutils/util.h>
#include <queue>
#include <unordered_map>

#pragma optimize("",off)
prosper::ShaderGraphics::VertexBinding::VertexBinding(Anvil::VertexInputRate inputRate,uint32_t stride)
	: stride(stride),inputRate(inputRate)
{}
prosper::ShaderGraphics::VertexBinding::VertexBinding(const VertexBinding &vbOther,std::optional<Anvil::VertexInputRate> inputRate,std::optional<uint32_t> stride)
{
	auto pinputRate = inputRate ? std::make_shared<Anvil::VertexInputRate>(*inputRate) : nullptr;
	auto pstride = stride ? std::make_shared<uint32_t>(*stride) : nullptr;
	initializer = [this,&vbOther,pinputRate,pstride]() {
		*this = vbOther;
		if(pinputRate != nullptr)
			this->inputRate = *pinputRate;
		if(pstride != nullptr)
			this->stride = *pstride;
		initializer = nullptr;
	};
}
uint32_t prosper::ShaderGraphics::VertexBinding::GetBindingIndex() const {return bindingIndex;}
prosper::ShaderGraphics::VertexAttribute::VertexAttribute(const VertexBinding &binding,Anvil::Format format,size_t startOffset)
	: binding(&binding),format(format),startOffset{(startOffset == std::numeric_limits<size_t>::max()) ? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(startOffset)}
{}
prosper::ShaderGraphics::VertexAttribute::VertexAttribute(const VertexAttribute &vaOther)
{
	initializer = [this,&vaOther]() {
		this->format = vaOther.format;
		this->startOffset = vaOther.startOffset;
		initializer = nullptr;
	};
}
prosper::ShaderGraphics::VertexAttribute::VertexAttribute(
	const VertexAttribute &vaOther,const VertexBinding &binding,
	std::optional<Anvil::Format> format
)
	: binding(&binding)
{
	auto pformat = format ? std::make_shared<Anvil::Format>(*format) : nullptr;
	initializer = [this,&vaOther,pformat]() {
		this->format = vaOther.format;
		this->startOffset = vaOther.startOffset;
		if(pformat != nullptr)
			this->format = *pformat;
		initializer = nullptr;
	};
}
uint32_t prosper::ShaderGraphics::VertexAttribute::GetLocation() const {return location;}

///////////////////////////

struct RenderPassInfo
{
	uint32_t pipelineIdx;
	std::shared_ptr<prosper::RenderPass> renderPass;
	prosper::util::RenderPassCreateInfo renderPassInfo;
};

struct RenderPassManager
{
	std::weak_ptr<prosper::Context> context = {};
	std::unordered_map<size_t,std::vector<RenderPassInfo>> renderPasses;
};

static uint32_t s_shaderCount = 0u;
static std::vector<std::unique_ptr<RenderPassManager>> s_rpManagers = {};
prosper::ShaderGraphics::ShaderGraphics(prosper::Context &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{
	++s_shaderCount;
}
prosper::ShaderGraphics::~ShaderGraphics()
{
	if(--s_shaderCount == 0u)
		s_rpManagers.clear();
}
const std::shared_ptr<prosper::RenderPass> &prosper::ShaderGraphics::GetRenderPass(prosper::Context &context,size_t hashCode,uint32_t pipelineIdx)
{
	auto it = std::find_if(s_rpManagers.begin(),s_rpManagers.end(),[&context](const std::unique_ptr<RenderPassManager> &rpMan) {
		return rpMan->context.lock().get() == &context;
	});
	static std::shared_ptr<RenderPass> nptr = nullptr;
	if(it == s_rpManagers.end())
		return nptr;
	auto &rpManager = *(*it);
	auto itRps = rpManager.renderPasses.find(hashCode);
	if(itRps == rpManager.renderPasses.end())
		return nptr;
	auto &rps = itRps->second;
	auto itRp = std::find_if(rps.begin(),rps.end(),[pipelineIdx](const RenderPassInfo &tp) {
		return tp.pipelineIdx == pipelineIdx;
	});
	if(itRp == rps.end())
		return nptr;
	return itRp->renderPass;
}
void prosper::ShaderGraphics::CreateCachedRenderPass(size_t hashCode,const prosper::util::RenderPassCreateInfo &renderPassInfo,std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx,const std::string &debugName)
{
	auto &context = GetContext();
	auto it = std::find_if(s_rpManagers.begin(),s_rpManagers.end(),[&context](const std::unique_ptr<RenderPassManager> &rpMan) {
		return rpMan->context.lock().get() == &context;
	});
	if(it == s_rpManagers.end())
	{
		s_rpManagers.push_back(std::make_unique<RenderPassManager>());
		it = s_rpManagers.end() -1u;
		(*it)->context = context.shared_from_this();
	}
	auto &rpManager = *(*it);
	auto itRps = rpManager.renderPasses.find(hashCode);
	if(itRps == rpManager.renderPasses.end())
		itRps = rpManager.renderPasses.insert(std::make_pair(hashCode,std::vector<RenderPassInfo>{})).first;
	auto &rps = itRps->second;
	auto itRp = std::find_if(rps.begin(),rps.end(),[pipelineIdx](const RenderPassInfo &tp) {
		return tp.pipelineIdx == pipelineIdx;
	});
	auto bInvalidated = false;
	if(itRp != rps.end())
	{
		// Check for differences between our old render pass and the new one
		auto &rpInfo = *itRp;
		if(rpInfo.renderPassInfo != renderPassInfo)
			bInvalidated = true;
	}
	if(itRp == rps.end() || bInvalidated == true)
	{
		auto rp = prosper::util::create_render_pass(context.GetDevice(),renderPassInfo);
		if(debugName.empty() == false)
			rp->SetDebugName(debugName);
		else
			rp->SetDebugName("shader_" +std::to_string(hashCode) +"_rp");
		if(itRp == rps.end())
		{
			rps.push_back({pipelineIdx,rp,renderPassInfo});
			itRp = rps.end() -1u;
		}
		else
			*itRp = {pipelineIdx,rp,renderPassInfo};
	}
	outRenderPass = itRp->renderPass;
}
void prosper::ShaderGraphics::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	pipelineInfo.toggle_dynamic_states(true,{Anvil::DynamicState::VIEWPORT});
}
void prosper::ShaderGraphics::InitializePipeline()
{
	auto &context = GetContext();
	auto &dev = context.GetDevice();
	auto swapchain = context.GetSwapchain();
	auto *gfxPipelineManager = dev.get_graphics_pipeline_manager();

	/* Configure the graphics pipeline */
	auto *modFs = GetStage(Anvil::ShaderStage::FRAGMENT);
	auto *modVs = GetStage(Anvil::ShaderStage::VERTEX);
	auto *modGs = GetStage(Anvil::ShaderStage::GEOMETRY);
	auto *modTessControl = GetStage(Anvil::ShaderStage::TESSELLATION_CONTROL);
	auto *modTessEval = GetStage(Anvil::ShaderStage::TESSELLATION_EVALUATION);
	auto firstPipelineId = std::numeric_limits<Anvil::PipelineID>::max();
	for(auto pipelineIdx=decltype(m_pipelineInfos.size()){0};pipelineIdx<m_pipelineInfos.size();++pipelineIdx)
	{
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		std::shared_ptr<RenderPass> renderPass = nullptr;
		InitializeRenderPass(renderPass,pipelineIdx);
		if(renderPass == nullptr)
			continue;
		Anvil::SubPassID subPassId {0};

		auto basePipelineId = std::numeric_limits<Anvil::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<Anvil::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false)
			m_basePipeline.lock()->GetPipelineId(basePipelineId);

		Anvil::PipelineCreateFlags createFlags = Anvil::PipelineCreateFlagBits::ALLOW_DERIVATIVES_BIT;
		auto bIsDerivative = basePipelineId != std::numeric_limits<Anvil::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | Anvil::PipelineCreateFlagBits::DERIVATIVE_BIT;
		auto gfxPipelineInfo = Anvil::GraphicsPipelineCreateInfo::create(
			createFlags,
			&renderPass->GetAnvilRenderPass(),
			subPassId,
			(modFs != nullptr) ? *modFs->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			(modGs != nullptr) ? *modGs->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			(modTessControl != nullptr) ? *modTessControl->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			(modTessEval != nullptr) ? *modTessEval->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			(modVs != nullptr) ? *modVs->entryPoint : Anvil::ShaderModuleStageEntryPoint(),
			nullptr,bIsDerivative ? &basePipelineId : nullptr
		);

		if(gfxPipelineInfo == nullptr)
			continue;
		ToggleDynamicViewportState(*gfxPipelineInfo,true);
		gfxPipelineInfo->set_primitive_topology(Anvil::PrimitiveTopology::TRIANGLE_LIST);

		gfxPipelineInfo->set_rasterization_properties(
			Anvil::PolygonMode::FILL,
			Anvil::CullModeFlagBits::BACK_BIT,
			Anvil::FrontFace::COUNTER_CLOCKWISE,
			1.0f /* line_width */
		);

		auto &rpInfo = *(*renderPass)->get_render_pass_create_info();
		auto numAttachments = rpInfo.get_n_attachments();
		auto samples = Anvil::SampleCountFlagBits::_1_BIT;
		for(auto i=decltype(numAttachments){0u};i<numAttachments;++i)
		{
			if(rpInfo.get_color_attachment_properties(i,nullptr,&samples) == true)
				break;
		}
		if(samples != Anvil::SampleCountFlagBits::_1_BIT)
			gfxPipelineInfo->set_multisampling_properties(samples,0.f,std::numeric_limits<VkSampleMask>::max());

		InitializeGfxPipeline(*gfxPipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*gfxPipelineInfo);
		PrepareGfxPipeline(*gfxPipelineInfo);

		if(prosper::util::are_dynamic_states_enabled(*gfxPipelineInfo,prosper::util::DynamicStateFlags::Scissor) == false)
			gfxPipelineInfo->set_scissor_box_properties(0u,0,0,std::numeric_limits<int32_t>::max(),std::numeric_limits<int32_t>::max());
		else
			gfxPipelineInfo->set_n_dynamic_scissor_boxes(1u);
		auto &pipelineInfo = m_pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = std::numeric_limits<decltype(pipelineInfo.id)>::max();
		auto r = gfxPipelineManager->add_pipeline(std::move(gfxPipelineInfo),&pipelineInfo.id);
		if(r == true)
		{
			if(firstPipelineId == std::numeric_limits<Anvil::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			pipelineInfo.renderPass = renderPass;
			OnPipelineInitialized(pipelineIdx);
		}
	}
}
Anvil::BasePipelineManager *prosper::ShaderGraphics::GetPipelineManager() const
{
	auto &context = const_cast<const ShaderGraphics*>(this)->GetContext();
	auto &dev = context.GetDevice();
	return dev.get_graphics_pipeline_manager();
}
void prosper::ShaderGraphics::PrepareGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo)
{
	// Initialize vertex bindings and attributes

	// Calculate strides for vertex bindings from attributes assigned to that respective binding
	std::queue<std::size_t> calcStrideQueue;
	for(auto i=decltype(m_vertexAttributes.size()){0};i<m_vertexAttributes.size();++i)
	{
		auto &refAttr = m_vertexAttributes.at(i);
		auto &attr = refAttr.get();

		if(attr.initializer != nullptr)
			attr.initializer();
		if(attr.binding != nullptr && attr.binding->initializer != nullptr)
			attr.binding->initializer();

		if(attr.binding == nullptr || attr.binding->stride != std::numeric_limits<uint32_t>::max())
			continue;
		calcStrideQueue.push(i);
	}
	while(calcStrideQueue.empty() == false)
	{
		auto i = calcStrideQueue.front();
		calcStrideQueue.pop();

		auto &refAttr = m_vertexAttributes.at(i);
		auto &attr = refAttr.get();
		auto &binding = const_cast<VertexBinding&>(*attr.binding);
		if(binding.stride == std::numeric_limits<uint32_t>::max())
			binding.stride = 0u;
		binding.stride += prosper::util::get_byte_size(attr.format);
	}
	//

	auto nextLocation = 0u;
	std::unordered_map<const VertexBinding*,uint32_t> nextStartOffsets;
	std::unordered_map<const VertexBinding*,uint32_t> vertexBindingIndices;

	struct AnvilVertexBinding
	{
		uint32_t binding;
		Anvil::VertexInputRate stepRate;
		uint32_t strideInBytes;
		std::vector<Anvil::VertexInputAttribute> attributes;
	};
	std::vector<AnvilVertexBinding> anvVertexBindings {};
	anvVertexBindings.reserve(m_vertexAttributes.size());

	auto vertexBindingCount = 0u;
	for(auto &refAttr : m_vertexAttributes)
	{
		auto &attr = refAttr.get();

		//auto &location = attr.location;
		auto location = std::numeric_limits<uint32_t>::max();
		if(location == std::numeric_limits<uint32_t>::max())
			location = nextLocation;
		attr.location = location;
		nextLocation = location +1u;

		auto &startOffset = attr.startOffset;
		if(startOffset == std::numeric_limits<uint32_t>::max())
		{
			auto it = nextStartOffsets.find(attr.binding);
			startOffset = (it != nextStartOffsets.end()) ? it->second : 0u;
		}
		nextStartOffsets[attr.binding] = startOffset +prosper::util::get_byte_size(attr.format);

		auto vertexBindingIdx = 0u;
		auto it = vertexBindingIndices.find(attr.binding);
		if(it == vertexBindingIndices.end())
		{
			it = vertexBindingIndices.insert(std::make_pair(attr.binding,vertexBindingCount++)).first;
			const_cast<prosper::ShaderGraphics::VertexBinding&>(*attr.binding).bindingIndex = it->second;
			anvVertexBindings.push_back({});
		}
		vertexBindingIdx = it->second;

		auto &anvBindingInfo = anvVertexBindings.at(vertexBindingIdx);
		anvBindingInfo.binding = vertexBindingIdx;
		anvBindingInfo.stepRate = attr.binding->inputRate;
		anvBindingInfo.strideInBytes = attr.binding->stride;
		anvBindingInfo.attributes.push_back(
			{attr.location,attr.format,attr.startOffset}
		);
	}
	for(auto &anvVertexBinding : anvVertexBindings)
	{
		if(pipelineInfo.add_vertex_binding(
			anvVertexBinding.binding,
			anvVertexBinding.stepRate,
			anvVertexBinding.strideInBytes,
			anvVertexBinding.attributes.size(),
			anvVertexBinding.attributes.data()
		) == false)
			throw std::runtime_error("Unable to add graphics pipeline vertex attribute for binding " +std::to_string(anvVertexBinding.binding));
	}
	m_vertexAttributes = {};
	//
}
void prosper::ShaderGraphics::InitializeRenderPass(std::shared_ptr<RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<prosper::ShaderGraphics>({{prosper::util::RenderPassCreateInfo::AttachmentInfo{}}},outRenderPass,pipelineIdx);
}
void prosper::ShaderGraphics::SetGenericAlphaColorBlendAttachmentProperties(Anvil::GraphicsPipelineCreateInfo &pipelineInfo)
{
	prosper::util::set_generic_alpha_color_blend_attachment_properties(pipelineInfo);
}
void prosper::ShaderGraphics::ToggleDynamicViewportState(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable)
{
	pipelineInfo.toggle_dynamic_states(bEnable,{Anvil::DynamicState::VIEWPORT});
	pipelineInfo.set_n_dynamic_viewports(bEnable ? 1u : 0u);
}

void prosper::ShaderGraphics::ToggleDynamicScissorState(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable)
{
	pipelineInfo.toggle_dynamic_states(bEnable,{Anvil::DynamicState::SCISSOR});
	pipelineInfo.set_n_dynamic_scissor_boxes(bEnable ? 1u : 0u);
}

const std::shared_ptr<prosper::RenderPass> &prosper::ShaderGraphics::GetRenderPass(uint32_t pipelineIdx) const
{
	auto &pipelineInfo = m_pipelineInfos.at(pipelineIdx);
	return pipelineInfo.renderPass;
}
static std::vector<vk::DeviceSize> s_vOffsets;
bool prosper::ShaderGraphics::RecordBindVertexBuffers(const std::vector<Anvil::Buffer*> &buffers,uint32_t startBinding,const std::vector<vk::DeviceSize> &offsets)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	if(offsets.empty() == false)
	{
		if(offsets.size() > s_vOffsets.size())
			s_vOffsets.resize(offsets.size(),0ull);
		for(auto i=decltype(offsets.size()){0};i<offsets.size();++i)
			s_vOffsets.at(i) = buffers.at(i)->get_create_info_ptr()->get_start_offset() +offsets.at(i);
	}
	else
	{
		if(buffers.size() > s_vOffsets.size())
			s_vOffsets.resize(buffers.size(),0ull);
		for(auto i=decltype(buffers.size()){0};i<buffers.size();++i)
			s_vOffsets.at(i) = buffers.at(i)->get_create_info_ptr()->get_start_offset();
	}
	return cmdBuffer != nullptr && (*cmdBuffer)->record_bind_vertex_buffers(startBinding,buffers.size(),const_cast<Anvil::Buffer**>(buffers.data()),s_vOffsets.data());
}

bool prosper::ShaderGraphics::RecordBindVertexBuffer(Anvil::Buffer &buffer,uint32_t startBinding,vk::DeviceSize offset)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	if(s_vOffsets.empty())
		s_vOffsets.resize(1u);
	s_vOffsets.at(0) = buffer.get_create_info_ptr()->get_start_offset() +offset;
	auto *ptrBuffer = &buffer;
	return cmdBuffer != nullptr && (*cmdBuffer)->record_bind_vertex_buffers(startBinding,1u,&ptrBuffer,s_vOffsets.data());
}
bool prosper::ShaderGraphics::RecordBindIndexBuffer(Anvil::Buffer &indexBuffer,Anvil::IndexType indexType,vk::DeviceSize offset)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && (*cmdBuffer)->record_bind_index_buffer(&indexBuffer,indexBuffer.get_create_info_ptr()->get_start_offset() +offset,indexType);
}

bool prosper::ShaderGraphics::RecordDraw(uint32_t vertCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && (*cmdBuffer)->record_draw(vertCount,instanceCount,firstVertex,firstInstance);
}

bool prosper::ShaderGraphics::RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,int32_t vertexOffset,uint32_t firstInstance)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && (*cmdBuffer)->record_draw_indexed(indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
}
bool prosper::ShaderGraphics::AddSpecializationConstant(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::ShaderStage stage,uint32_t constantId,uint32_t numBytes,const void *data)
{
	return pipelineInfo.add_specialization_constant(stage,constantId,numBytes,data);
}
void prosper::ShaderGraphics::AddVertexAttribute(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,VertexAttribute &attr) {m_vertexAttributes.push_back(attr);}
bool prosper::ShaderGraphics::RecordBindDescriptorSet(Anvil::DescriptorSet &descSet,uint32_t firstSet,const std::vector<uint32_t> &dynamicOffsets)
{
	if(GetContext().IsValidationEnabled())
	{
		// Make sure the sample count of all image bindings of the descriptor set corresponds to the shader's sample count
		auto &layout = *descSet.get_descriptor_set_layout();
		auto &createInfo = *layout.get_create_info();
		auto numBindings = createInfo.get_n_bindings();
		for(auto i=decltype(numBindings){0};i<numBindings;++i)
		{
			Anvil::DescriptorType descType;
			uint32_t arraySize;
			if(createInfo.get_binding_properties_by_index_number(i,nullptr,&descType,&arraySize) == false)
				continue;
			for(auto j=decltype(arraySize){0};j<arraySize;++j)
			{
				Anvil::ImageView *imgView = nullptr;
				switch(descType)
				{
					case Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER:
						descSet.get_combined_image_sampler_binding_properties(i,j,nullptr,&imgView,nullptr);
						break;
					case Anvil::DescriptorType::SAMPLED_IMAGE:
						descSet.get_sampled_image_binding_properties(i,j,nullptr,&imgView);
						break;
					case Anvil::DescriptorType::STORAGE_IMAGE:
						descSet.get_storage_image_binding_properties(i,j,nullptr,&imgView);
						break;
				}
				if(imgView == nullptr)
					continue;
				auto *img = imgView->get_create_info_ptr()->get_parent_image();
				if(img == nullptr)
					continue;
				auto imgSampleCount = static_cast<vk::SampleCountFlagBits>(img->get_create_info_ptr()->get_sample_count());
				if(imgSampleCount != vk::SampleCountFlagBits::e1)
				{
					debug::exec_debug_validation_callback(
						vk::DebugReportObjectTypeEXT::eImage,"Bind descriptor set: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(img->get_image())) +
						" at binding " +std::to_string(i) +", array index " +std::to_string(j) +" is multi-sampled with sample count of " +vk::to_string(imgSampleCount) +
						". Is this intended?"
					);
				}
			}
		}
	}
	return prosper::Shader::RecordBindDescriptorSet(descSet,firstSet,dynamicOffsets);
}
bool prosper::ShaderGraphics::BeginDrawViewport(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t width,uint32_t height,uint32_t pipelineIdx,RecordFlags recordFlags)
{
	auto *info = static_cast<const Anvil::GraphicsPipelineCreateInfo*>(GetPipelineInfo(pipelineIdx));
	if(info == nullptr)
		return false;
	auto b = BindPipeline(cmdBuffer->GetAnvilCommandBuffer(),pipelineIdx);
	if(b == false)
		return false;
	if(GetContext().IsValidationEnabled())
	{
		prosper::RenderPass *rp;
		prosper::Image *img;
		prosper::RenderTarget *rt;
		prosper::Framebuffer *fb;
		if(prosper::util::get_current_render_pass_target(**cmdBuffer,&rp,&img,&fb,&rt) && fb && rp)
		{
			auto &rpCreateInfo = *rp->GetAnvilRenderPass().get_render_pass_create_info();
			Anvil::SampleCountFlagBits sampleCount;
			info->get_multisampling_properties(&sampleCount,nullptr);
			auto numAttachments = umath::min(fb->GetAttachmentCount(),rpCreateInfo.get_n_attachments());
			for(auto i=decltype(numAttachments){0};i<numAttachments;++i)
			{
				auto *imgView = fb->GetAttachment(i);
				if(imgView == nullptr)
					continue;
				auto &img = imgView->GetImage();
				Anvil::AttachmentType attType;
				Anvil::SampleCountFlagBits rpSampleCount;
				auto bRpHasColorAttachment = false;
				if(rpCreateInfo.get_attachment_type(i,&attType) == true)
				{
					switch(attType)
					{
					case Anvil::AttachmentType::COLOR:
					{
						bRpHasColorAttachment = true;
						rpCreateInfo.get_color_attachment_properties(i,nullptr,&rpSampleCount);
						break;
					}
					}
				}

				if((sampleCount &img.GetSampleCount()) == Anvil::SampleCountFlagBits::NONE)
				{
					debug::exec_debug_validation_callback(
						vk::DebugReportObjectTypeEXT::ePipeline,"Begin draw: Incompatible sample count between currently bound image '" +img.GetDebugName() +
						"' (with sample count " +vk::to_string(static_cast<vk::SampleCountFlagBits>(img.GetSampleCount())) +") and shader pipeline '" +
						*GetDebugName(pipelineIdx) +"' (with sample count "+vk::to_string(static_cast<vk::SampleCountFlagBits>(sampleCount)) +")!"
					);
					return false;
				}
				if(bRpHasColorAttachment == true)
				{
					if((sampleCount &rpSampleCount) == Anvil::SampleCountFlagBits::NONE)
					{
						debug::exec_debug_validation_callback(
							vk::DebugReportObjectTypeEXT::ePipeline,"Begin draw: Incompatible sample count between currently bound render pass '" +rp->GetDebugName() +
							"' (with sample count " +vk::to_string(static_cast<vk::SampleCountFlagBits>(rpSampleCount)) +") and shader pipeline '" +
							*GetDebugName(pipelineIdx) +"' (with sample count "+vk::to_string(static_cast<vk::SampleCountFlagBits>(sampleCount)) +")!"
						);
						return false;
					}
					if(rpSampleCount != img.GetSampleCount())
					{
						debug::exec_debug_validation_callback(
							vk::DebugReportObjectTypeEXT::ePipeline,"Begin draw: Incompatible sample count between currently bound render pass '" +rp->GetDebugName() +
							"' (with sample count " +vk::to_string(static_cast<vk::SampleCountFlagBits>(rpSampleCount)) +") and render target attachment " +std::to_string(i) +" ('" +img.GetDebugName() +"')" +
							" for image '" +img.GetDebugName() +"', which has sample count of " +vk::to_string(static_cast<vk::SampleCountFlagBits>(sampleCount)) +")!"
						);
						return false;
					}
				}
			}
		}
	}
	if(recordFlags != RecordFlags::None)
	{
		auto nDynamicScissors = info->get_n_dynamic_scissor_boxes();
		auto nDynamicViewports = info->get_n_dynamic_viewports();
		auto bScissor = (nDynamicScissors > 0u && (recordFlags &RecordFlags::RenderPassTargetAsScissor) != RecordFlags::None) ? true : false;
		auto bViewport = (nDynamicViewports > 0u && (recordFlags &RecordFlags::RenderPassTargetAsViewport) != RecordFlags::None) ? true : false;
		if(bScissor || bViewport)
		{
			vk::Extent2D extents {width,height};
			if(bScissor)
			{
				vk::Rect2D scissor(vk::Offset2D(0,0),extents);
				if((*cmdBuffer)->record_set_scissor(0u,1u,reinterpret_cast<VkRect2D*>(&scissor)) == false)
					return false;
			}
			if(bViewport)
			{
				auto vp = vk::Viewport(0.f,0.f,extents.width,extents.height,0.f,1.f);
				if((*cmdBuffer)->record_set_viewport(0u,1u,reinterpret_cast<VkViewport*>(&vp)) == false)
					return false;
			}
		}
	}
	SetCurrentDrawCommandBuffer(cmdBuffer,pipelineIdx);
	return true;
}
bool prosper::ShaderGraphics::BeginDraw(const std::shared_ptr<prosper::PrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx,RecordFlags recordFlags)
{
	prosper::Image *img;
	if(prosper::util::get_current_render_pass_target(cmdBuffer->GetAnvilCommandBuffer(),nullptr,&img) == false || img == nullptr)
		return false;
	auto extents = img->GetExtents();
	return BeginDrawViewport(cmdBuffer,extents.width,extents.height,pipelineIdx,recordFlags);
}
bool prosper::ShaderGraphics::Draw() {return true;}
void prosper::ShaderGraphics::EndDraw() {UnbindPipeline(); SetCurrentDrawCommandBuffer(nullptr);}
#pragma optimize("",on)
