/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_pipeline_loader.hpp"
#include "prosper_util.hpp"
#include "image/prosper_render_target.hpp"
#include "image/prosper_texture.hpp"
#include "image/prosper_image_view.hpp"
#include "debug/prosper_debug.hpp"
#include "prosper_framebuffer.hpp"
#include "prosper_render_pass.hpp"
#include "prosper_command_buffer.hpp"
#include <sharedutils/util.h>
#include <queue>
#include <unordered_map>

prosper::ShaderGraphics::VertexBinding::VertexBinding(prosper::VertexInputRate inputRate,uint32_t stride)
	: stride(stride),inputRate(inputRate)
{}
prosper::ShaderGraphics::VertexBinding::VertexBinding(const VertexBinding &vbOther,std::optional<prosper::VertexInputRate> inputRate,std::optional<uint32_t> stride)
{
	auto pinputRate = inputRate ? std::make_shared<prosper::VertexInputRate>(*inputRate) : nullptr;
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
prosper::ShaderGraphics::VertexAttribute::VertexAttribute(const VertexBinding &binding,prosper::Format format,size_t startOffset)
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
	std::optional<prosper::Format> format
)
	: binding(&binding)
{
	auto pformat = format ? std::make_shared<prosper::Format>(*format) : nullptr;
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
	std::shared_ptr<prosper::IRenderPass> renderPass;
	prosper::util::RenderPassCreateInfo renderPassInfo;
};

struct RenderPassManager
{
	std::weak_ptr<prosper::IPrContext> context = {};
	std::unordered_map<size_t,std::vector<RenderPassInfo>> renderPasses;
};

static uint32_t s_shaderCount = 0u;
static std::vector<std::unique_ptr<RenderPassManager>> s_rpManagers = {};
prosper::ShaderGraphics::ShaderGraphics(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{
	++s_shaderCount;
}
prosper::ShaderGraphics::~ShaderGraphics()
{
	if(--s_shaderCount == 0u)
		s_rpManagers.clear();
}
const std::shared_ptr<prosper::IRenderPass> &prosper::ShaderGraphics::GetRenderPass(prosper::IPrContext &context,size_t hashCode,uint32_t pipelineIdx)
{
	auto it = std::find_if(s_rpManagers.begin(),s_rpManagers.end(),[&context](const std::unique_ptr<RenderPassManager> &rpMan) {
		return rpMan->context.lock().get() == &context;
	});
	static std::shared_ptr<IRenderPass> nptr = nullptr;
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
void prosper::ShaderGraphics::CreateCachedRenderPass(size_t hashCode,const prosper::util::RenderPassCreateInfo &renderPassInfo,std::shared_ptr<IRenderPass> &outRenderPass,uint32_t pipelineIdx,const std::string &debugName)
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
		auto rp = context.CreateRenderPass(renderPassInfo);
		if(rp)
		{
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
	}
	outRenderPass = itRp->renderPass;
}
void prosper::ShaderGraphics::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	pipelineInfo.ToggleDynamicStates(true,{prosper::DynamicState::Viewport});
}

void prosper::ShaderGraphics::InitializePipeline()
{
	// Reset pipeline infos (in case the shader is being reloaded)
	auto &pipelineInfos = GetPipelineInfos();
	for(auto &pipelineInfo : pipelineInfos)
		pipelineInfo = {};

	/* Configure the graphics pipeline */
	auto *modFs = GetStage(ShaderStage::Fragment);
	auto *modVs = GetStage(ShaderStage::Vertex);
	auto *modGs = GetStage(ShaderStage::Geometry);
	auto *modTessControl = GetStage(ShaderStage::TessellationControl);
	auto *modTessEval = GetStage(ShaderStage::TessellationEvaluation);
	auto firstPipelineId = std::numeric_limits<prosper::PipelineID>::max();

	static std::chrono::high_resolution_clock::duration accTime {0};
	auto t = std::chrono::high_resolution_clock::now();
	for(auto pipelineIdx=decltype(pipelineInfos.size()){0};pipelineIdx<pipelineInfos.size();++pipelineIdx)
	{
		if(ShouldInitializePipeline(pipelineIdx) == false)
			continue;
		std::shared_ptr<IRenderPass> renderPass = nullptr;
		InitializeRenderPass(renderPass,pipelineIdx);
		if(renderPass == nullptr)
			continue;
		prosper::SubPassID subPassId {0};

		auto basePipelineId = std::numeric_limits<prosper::PipelineID>::max();
		if(firstPipelineId != std::numeric_limits<prosper::PipelineID>::max())
			basePipelineId = firstPipelineId;
		else if(m_basePipeline.expired() == false)
		{
			assert(!GetContext().GetPipelineLoader().IsShaderQueued(m_basePipeline.lock()->GetIndex()));
			// Base shader pipeline must have already been loaded (but not necessarily baked) at this point!
			m_basePipeline.lock()->GetPipelineId(basePipelineId,0u,false);
		}

		prosper::PipelineCreateFlags createFlags = prosper::PipelineCreateFlags::AllowDerivativesBit;
		auto bIsDerivative = basePipelineId != std::numeric_limits<prosper::PipelineID>::max();
		if(bIsDerivative)
			createFlags = createFlags | prosper::PipelineCreateFlags::DerivativeBit;
		auto gfxPipelineInfo = prosper::GraphicsPipelineCreateInfo::Create(
			createFlags,
			renderPass.get(),
			subPassId,
			(modFs != nullptr) ? *modFs->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			(modGs != nullptr) ? *modGs->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			(modTessControl != nullptr) ? *modTessControl->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			(modTessEval != nullptr) ? *modTessEval->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			(modVs != nullptr) ? *modVs->entryPoint : prosper::ShaderModuleStageEntryPoint(),
			nullptr,bIsDerivative ? &basePipelineId : nullptr
		);

		if(gfxPipelineInfo == nullptr)
			continue;
		ToggleDynamicViewportState(*gfxPipelineInfo,true);
		gfxPipelineInfo->SetPrimitiveTopology(prosper::PrimitiveTopology::TriangleList);

		gfxPipelineInfo->SetRasterizationProperties(
			prosper::PolygonMode::Fill,
			prosper::CullModeFlags::BackBit,
			prosper::FrontFace::CounterClockwise,
			1.0f /* line_width */
		);

		auto &rpInfo = renderPass->GetCreateInfo();
		auto numAttachments = rpInfo.attachments.size();
		auto samples = (numAttachments > 0) ? rpInfo.attachments.front().sampleCount : prosper::SampleCountFlags::e1Bit;
		if(samples != prosper::SampleCountFlags::e1Bit)
			gfxPipelineInfo->SetMultisamplingProperties(samples,0.f,std::numeric_limits<SampleMask>::max());

		m_currentPipelineIdx = pipelineIdx;
		InitializeGfxPipeline(*gfxPipelineInfo,pipelineIdx);
		InitializeDescriptorSetGroup(*gfxPipelineInfo);

		auto &pipelineInitInfo = pipelineInfos.at(pipelineIdx);
		for(auto &range : pipelineInitInfo.pushConstantRanges)
			gfxPipelineInfo->AttachPushConstantRange(range.offset,range.size,range.stages);

		PrepareGfxPipeline(*gfxPipelineInfo);
		m_currentPipelineIdx = std::numeric_limits<decltype(m_currentPipelineIdx)>::max();

		if(prosper::util::are_dynamic_states_enabled(*gfxPipelineInfo,prosper::util::DynamicStateFlags::Scissor) == false)
			gfxPipelineInfo->SetScissorBoxProperties(0u,0,0,std::numeric_limits<int32_t>::max(),std::numeric_limits<int32_t>::max());
		else
			gfxPipelineInfo->SetDynamicScissorBoxesCount(1u);
		auto &pipelineInfo = pipelineInfos.at(pipelineIdx);
		pipelineInfo.id = InitPipelineId(pipelineIdx);
		auto &context = GetContext();
		auto result = context.AddPipeline(*this,pipelineIdx,*gfxPipelineInfo,*renderPass,modFs,modVs,modGs,modTessControl,modTessEval,subPassId,basePipelineId);
		pipelineInfo.createInfo = std::move(gfxPipelineInfo);
		if(result.has_value())
		{
			pipelineInfo.id = *result;
			if(firstPipelineId == std::numeric_limits<prosper::PipelineID>::max())
				firstPipelineId = pipelineInfo.id;
			pipelineInfo.renderPass = renderPass;
			OnPipelineInitialized(pipelineIdx);

			GetContext().GetPipelineLoader().Bake(GetIndex(),pipelineInfo.id,GetPipelineBindPoint());
		}
		else
			pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}
void prosper::ShaderGraphics::PrepareGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo)
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
		binding.stride += prosper::util::get_byte_size(static_cast<Format>(attr.format));
	}
	//

	auto nextLocation = 0u;
	std::unordered_map<const VertexBinding*,uint32_t> nextStartOffsets;
	std::unordered_map<const VertexBinding*,uint32_t> vertexBindingIndices;

	struct VertexBindingInfo
	{
		uint32_t binding;
		prosper::VertexInputRate stepRate;
		uint32_t strideInBytes;
		std::vector<prosper::VertexInputAttribute> attributes;
	};
	std::vector<VertexBindingInfo> vertexBindingsInfo {};
	vertexBindingsInfo.reserve(m_vertexAttributes.size());

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
		nextStartOffsets[attr.binding] = startOffset +prosper::util::get_byte_size(static_cast<Format>(attr.format));

		auto vertexBindingIdx = 0u;
		auto it = vertexBindingIndices.find(attr.binding);
		if(it == vertexBindingIndices.end())
		{
			it = vertexBindingIndices.insert(std::make_pair(attr.binding,vertexBindingCount++)).first;
			const_cast<prosper::ShaderGraphics::VertexBinding&>(*attr.binding).bindingIndex = it->second;
			vertexBindingsInfo.push_back({});
		}
		vertexBindingIdx = it->second;

		auto &anvBindingInfo = vertexBindingsInfo.at(vertexBindingIdx);
		anvBindingInfo.binding = vertexBindingIdx;
		anvBindingInfo.stepRate = attr.binding->inputRate;
		anvBindingInfo.strideInBytes = attr.binding->stride;
		anvBindingInfo.attributes.push_back(
			{attr.location,attr.format,attr.startOffset}
		);
	}
	for(auto &vertexBindingInfo : vertexBindingsInfo)
	{
		if(pipelineInfo.AddVertexBinding(
			vertexBindingInfo.binding,
			vertexBindingInfo.stepRate,
			vertexBindingInfo.strideInBytes,
			vertexBindingInfo.attributes.size(),
			vertexBindingInfo.attributes.data()
		) == false)
			throw std::runtime_error("Unable to add graphics pipeline vertex attribute for binding " +std::to_string(vertexBindingInfo.binding));
	}
	m_vertexAttributes = {};
	//
}
void prosper::ShaderGraphics::InitializeRenderPass(std::shared_ptr<IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<prosper::ShaderGraphics>({{prosper::util::RenderPassCreateInfo::AttachmentInfo{}}},outRenderPass,pipelineIdx);
}
void prosper::ShaderGraphics::SetGenericAlphaColorBlendAttachmentProperties(prosper::GraphicsPipelineCreateInfo &pipelineInfo)
{
	prosper::util::set_generic_alpha_color_blend_attachment_properties(pipelineInfo);
}
void prosper::ShaderGraphics::ToggleDynamicViewportState(prosper::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable)
{
	pipelineInfo.ToggleDynamicStates(bEnable,{prosper::DynamicState::Viewport});
	pipelineInfo.SetDynamicViewportCount(bEnable ? 1u : 0u);
}

void prosper::ShaderGraphics::ToggleDynamicScissorState(prosper::GraphicsPipelineCreateInfo &pipelineInfo,bool bEnable)
{
	pipelineInfo.ToggleDynamicStates(bEnable,{prosper::DynamicState::Scissor});
	pipelineInfo.SetDynamicScissorBoxesCount(bEnable ? 1u : 0u);
}

const std::shared_ptr<prosper::IRenderPass> &prosper::ShaderGraphics::GetRenderPass(uint32_t pipelineIdx) const
{
	auto *pipelineInfo = GetPipelineInfo(pipelineIdx);
	assert(pipelineInfo);
	return pipelineInfo->renderPass;
}
bool prosper::ShaderGraphics::RecordBindRenderBuffer(const IRenderBuffer &renderBuffer)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer ? cmdBuffer->RecordBindRenderBuffer(renderBuffer) : false;
}
bool prosper::ShaderGraphics::RecordBindVertexBuffers(const std::vector<IBuffer*> &buffers,uint32_t startBinding,const std::vector<prosper::DeviceSize> &offsets)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer ? cmdBuffer->RecordBindVertexBuffers(*this,buffers,startBinding,offsets) : false;
}

bool prosper::ShaderGraphics::RecordBindVertexBuffer(prosper::IBuffer &buffer,uint32_t startBinding,DeviceSize offset)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer ? cmdBuffer->RecordBindVertexBuffers(*this,{&buffer},startBinding,{offset}) : false;
}
bool prosper::ShaderGraphics::RecordBindIndexBuffer(prosper::IBuffer &indexBuffer,prosper::IndexType indexType,DeviceSize offset)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && cmdBuffer->RecordBindIndexBuffer(indexBuffer,indexType,offset);
}

bool prosper::ShaderGraphics::RecordDraw(uint32_t vertCount,uint32_t instanceCount,uint32_t firstVertex,uint32_t firstInstance)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && cmdBuffer->RecordDraw(vertCount,instanceCount,firstVertex,firstInstance);
}

bool prosper::ShaderGraphics::RecordDrawIndexed(uint32_t indexCount,uint32_t instanceCount,uint32_t firstIndex,uint32_t firstInstance)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && cmdBuffer->RecordDrawIndexed(indexCount,instanceCount,firstIndex,firstInstance);
}
bool prosper::ShaderGraphics::AddSpecializationConstant(prosper::GraphicsPipelineCreateInfo &pipelineInfo,prosper::ShaderStageFlags stageFlags,uint32_t constantId,uint32_t numBytes,const void *data)
{
	for(auto v : util::shader_stage_flags_to_shader_stages(stageFlags))
	{
		if(pipelineInfo.AddSpecializationConstant(v,constantId,numBytes,data) == false)
			return false;
	}
	return true;
}
void prosper::ShaderGraphics::AddVertexAttribute(prosper::GraphicsPipelineCreateInfo &pipelineInfo,VertexAttribute &attr) {m_vertexAttributes.push_back(attr);}
bool prosper::ShaderGraphics::RecordBindDescriptorSet(prosper::IDescriptorSet &descSet,uint32_t firstSet,const std::vector<uint32_t> &dynamicOffsets)
{
#if 0
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
#endif
	return prosper::Shader::RecordBindDescriptorSet(descSet,firstSet,dynamicOffsets);
}
bool prosper::ShaderGraphics::BeginDrawViewport(const std::shared_ptr<prosper::ICommandBuffer> &cmdBuffer,uint32_t width,uint32_t height,uint32_t pipelineIdx,RecordFlags recordFlags)
{
	auto *info = static_cast<const prosper::GraphicsPipelineCreateInfo*>(GetPipelineCreateInfo(pipelineIdx));
	if(info == nullptr)
		return false;
	auto b = BindPipeline(*cmdBuffer,pipelineIdx);
	if(b == false)
		return false;
#if 0
	if(GetContext().IsValidationEnabled())
	{
		prosper::IRenderPass *rp;
		prosper::IImage *img;
		prosper::RenderTarget *rt;
		prosper::IFramebuffer *fb;
		if(cmdBuffer->GetActiveRenderPassTarget(&rp,&img,&fb,&rt) && fb && rp)
		{
			auto &rpCreateInfo = rp->GetCreateInfo();
			prosper::SampleCountFlags sampleCount;
			info->GetMultisamplingProperties(&sampleCount,nullptr);
			auto numAttachments = umath::min(fb->GetAttachmentCount(),static_cast<uint32_t>(rpCreateInfo.attachments.size()));
			for(auto i=decltype(numAttachments){0};i<numAttachments;++i)
			{
				auto *imgView = fb->GetAttachment(i);
				if(imgView == nullptr)
					continue;
				auto &img = imgView->GetImage();
				auto &attInfo = rpCreateInfo.attachments.at(i);
				auto rpSampleCount = attInfo.sampleCount;
				auto bRpHasColorAttachment = false;
				if(util::is_depth_format(attInfo.format) == false)
					bRpHasColorAttachment = true;

				if((sampleCount &img.GetSampleCount()) == prosper::SampleCountFlags::None)
				{
					debug::exec_debug_validation_callback(
						GetContext(),DebugReportObjectTypeEXT::Pipeline,"Begin draw: Incompatible sample count between currently bound image '" +img.GetDebugName() +
						"' (with sample count " +util::to_string(img.GetSampleCount()) +") and shader pipeline '" +
						*GetDebugName(pipelineIdx) +"' (with sample count "+util::to_string(sampleCount) +")!"
					);
					return false;
				}
				if(bRpHasColorAttachment == true)
				{
					if((sampleCount &rpSampleCount) == prosper::SampleCountFlags::None)
					{
						debug::exec_debug_validation_callback(
							GetContext(),DebugReportObjectTypeEXT::Pipeline,"Begin draw: Incompatible sample count between currently bound render pass '" +rp->GetDebugName() +
							"' (with sample count " +util::to_string(rpSampleCount) +") and shader pipeline '" +
							*GetDebugName(pipelineIdx) +"' (with sample count "+util::to_string(sampleCount) +")!"
						);
						return false;
					}
					if(rpSampleCount != img.GetSampleCount())
					{
						debug::exec_debug_validation_callback(
							GetContext(),DebugReportObjectTypeEXT::Pipeline,"Begin draw: Incompatible sample count between currently bound render pass '" +rp->GetDebugName() +
							"' (with sample count " +util::to_string(rpSampleCount) +") and render target attachment " +std::to_string(i) +" ('" +img.GetDebugName() +"')" +
							" for image '" +img.GetDebugName() +"', which has sample count of " +util::to_string(sampleCount) +")!"
						);
						return false;
					}
				}
			}
		}
	}
#endif
	if(recordFlags != RecordFlags::None)
	{
		auto nDynamicScissors = info->GetDynamicScissorBoxesCount();
		auto nDynamicViewports = info->GetDynamicViewportsCount();
		auto bScissor = (nDynamicScissors > 0u && (recordFlags &RecordFlags::RenderPassTargetAsScissor) != RecordFlags::None) ? true : false;
		auto bViewport = (nDynamicViewports > 0u && (recordFlags &RecordFlags::RenderPassTargetAsViewport) != RecordFlags::None) ? true : false;
		if(bScissor || bViewport)
		{
			prosper::Extent2D extents {width,height};
			if(bScissor)
			{
				if(cmdBuffer->RecordSetScissor(extents.width,extents.height) == false)
					return false;
			}
			if(bViewport)
			{
				if(cmdBuffer->RecordSetViewport(extents.width,extents.height,0,0,0.f /* minDepth */,1.f /* maxDepth */) == false)
					return false;
			}
		}
	}
	SetCurrentDrawCommandBuffer(cmdBuffer,pipelineIdx);
	return true;
}
bool prosper::ShaderGraphics::BeginDraw(const std::shared_ptr<prosper::ICommandBuffer> &cmdBuffer,uint32_t pipelineIdx,RecordFlags recordFlags)
{
	prosper::Extent2D extents;
	if(cmdBuffer->IsPrimary())
	{
		prosper::IImage *img;
		if(cmdBuffer->GetPrimaryCommandBufferPtr()->GetActiveRenderPassTarget(nullptr,&img) == false || img == nullptr)
			return false;
		extents = img->GetExtents();
	}
	else
	{
		auto *fb = cmdBuffer->GetSecondaryCommandBufferPtr()->GetCurrentFramebuffer();
		if(fb == nullptr)
			return false;
		extents = {fb->GetWidth(),fb->GetHeight()};
	}
	return BeginDrawViewport(cmdBuffer,extents.width,extents.height,pipelineIdx,recordFlags);
}
bool prosper::ShaderGraphics::Draw() {return true;}
void prosper::ShaderGraphics::EndDraw() {UnbindPipeline(); SetCurrentDrawCommandBuffer(nullptr);}
