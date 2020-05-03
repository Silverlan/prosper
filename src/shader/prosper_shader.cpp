/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "vk_context.hpp"
#include "prosper_util.hpp"
#include "prosper_glstospv.hpp"
#include "prosper_command_buffer.hpp"
#include "debug/prosper_debug_lookup_map.hpp"
#include <misc/glsl_to_spirv.h>
#include <misc/graphics_pipeline_create_info.h>
#include <wrappers/device.h>
#include <wrappers/render_pass.h>
#include <wrappers/graphics_pipeline_manager.h>
#include <wrappers/compute_pipeline_manager.h>
#include <wrappers/pipeline_cache.h>
#include <wrappers/shader_module.h>
#include <misc/descriptor_set_create_info.h>
#include <misc/render_pass_create_info.h>
#include <misc/image_view_create_info.h>
#include <iostream>
#include <fsys/filesystem.h>
#include <sharedutils/util.h>

prosper::DescriptorSetInfo::DescriptorSetInfo(const std::vector<Binding> &bindings)
	: bindings(bindings)
{}
prosper::DescriptorSetInfo::DescriptorSetInfo(DescriptorSetInfo *parent,const std::vector<Binding> &bindings)
	: parent(parent),bindings(bindings)
{}
bool prosper::DescriptorSetInfo::WasBaked() const {return m_bWasBaked;}
bool prosper::DescriptorSetInfo::IsValid() const {return WasBaked();}
prosper::DescriptorSetInfo::Binding::Binding(DescriptorType type,ShaderStageFlags shaderStages,uint32_t descriptorArraySize,uint32_t bindingIndex)
	: bindingIndex(bindingIndex),type(type),shaderStages(shaderStages),descriptorArraySize(descriptorArraySize)
{}

///////////////////////////

decltype(prosper::Shader::s_logCallback) prosper::Shader::s_logCallback = nullptr;
void prosper::Shader::SetLogCallback(const std::function<void(Shader&,ShaderStage,const std::string&,const std::string&)> &fLogCallback) {s_logCallback = fLogCallback;}

prosper::Shader::Shader(IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: std::enable_shared_from_this<Shader>(),
	ContextObject(context),m_pipelineBindPoint(PipelineBindPoint::Graphics),m_identifier(identifier)
{
	if(vsShader.empty() == false)
		(m_stages.at(umath::to_integral(ShaderStage::Vertex)) = std::make_shared<ShaderStageData>())->path = vsShader;
	if(fsShader.empty() == false)
		(m_stages.at(umath::to_integral(ShaderStage::Fragment)) = std::make_shared<ShaderStageData>())->path = fsShader;
	if(gsShader.empty() == false)
		(m_stages.at(umath::to_integral(ShaderStage::Geometry)) = std::make_shared<ShaderStageData>())->path = gsShader;
	SetPipelineCount(1u);
}
prosper::Shader::Shader(IPrContext &context,const std::string &identifier,const std::string &csShader)
	: ContextObject(context),m_pipelineBindPoint(PipelineBindPoint::Compute),m_identifier(identifier)
{
	if(csShader.empty() == false)
		(m_stages.at(umath::to_integral(ShaderStage::Compute)) = std::make_shared<ShaderStageData>())->path = csShader;
	SetPipelineCount(1u);
}
void prosper::Shader::Release(bool bDelete)
{
	ClearPipelines();
	for(auto &stage : m_stages)
		stage.reset();
	if(bDelete == true)
		delete this;
}
void prosper::Shader::SetStageSourceFilePath(ShaderStage stage,const std::string &filePath)
{
	if(filePath.empty())
	{
		m_stages.at(umath::to_integral(stage)) = nullptr;
		return;
	}
	(m_stages.at(umath::to_integral(stage)) = std::make_shared<ShaderStageData>())->path = filePath;
	switch(stage)
	{
		case ShaderStage::Compute:
			m_pipelineBindPoint = PipelineBindPoint::Compute;
			break;
		default:
			m_pipelineBindPoint = PipelineBindPoint::Graphics;
			break;
	}
}

const std::string &prosper::Shader::GetIdentifier() const {return m_identifier;}

void prosper::Shader::SetPipelineCount(uint32_t count) {m_pipelineInfos.resize(count,{std::numeric_limits<Anvil::PipelineID>::max(),nullptr});}

bool prosper::Shader::IsGraphicsShader() const {return m_pipelineBindPoint == PipelineBindPoint::Graphics;}
bool prosper::Shader::IsComputeShader() const {return m_pipelineBindPoint == PipelineBindPoint::Compute;}
prosper::PipelineBindPoint prosper::Shader::GetPipelineBindPoint() const {return m_pipelineBindPoint;}

Anvil::ShaderModule *prosper::Shader::GetModule(ShaderStage stage)
{
	auto *stageModule = GetStage(stage);
	if(stageModule == nullptr)
		return nullptr;
	return stageModule->module.get();
}

prosper::ShaderStageData *prosper::Shader::GetStage(ShaderStage stage)
{
	auto &stageModule = m_stages.at(umath::to_integral(stage));
	return stageModule.get();
}

const prosper::ShaderStageData *prosper::Shader::GetStage(ShaderStage stage) const {return const_cast<prosper::Shader*>(this)->GetStage(stage);}

static std::string g_shaderLocation = "shaders";
void prosper::Shader::SetRootShaderLocation(const std::string &location)
{
	g_shaderLocation = location;
}
const std::string &prosper::Shader::GetRootShaderLocation() {return g_shaderLocation;}

bool prosper::Shader::InitializeSources(bool bReload)
{
	auto &context = GetContext();
	for(auto i=decltype(m_stages.size()){0};i<m_stages.size();++i)
	{
		auto &stage = m_stages.at(i);
		if(stage == nullptr || stage->path.empty())
			continue;
		std::string infoLog;
		std::string debugInfoLog;
		stage->spirvBlob.clear();

		auto shaderLocation = g_shaderLocation;
		if(shaderLocation.empty() == false)
			shaderLocation += '\\';
		auto bSuccess = prosper::glsl_to_spv(context,i,shaderLocation +stage->path,stage->spirvBlob,&infoLog,&debugInfoLog,bReload);
		if(bSuccess == false)
		{
			if(s_logCallback != nullptr)
				s_logCallback(*this,static_cast<ShaderStage>(i),infoLog,debugInfoLog);
			return false;
		}
	}
	return true;
}

bool prosper::Shader::IsValid() const {return m_bValid;}

void prosper::Shader::SetIdentifier(const std::string &identifier) {m_identifier = identifier;}
uint32_t prosper::Shader::GetCurrentPipelineIndex() const {return m_currentPipelineIdx;}
std::shared_ptr<prosper::IPrimaryCommandBuffer> prosper::Shader::GetCurrentCommandBuffer() const {return m_currentCmd.lock();}

uint32_t prosper::Shader::GetPipelineCount() const {return m_pipelineInfos.size();}
void prosper::Shader::ReloadPipelines(bool bReloadSourceCode) {Initialize(bReloadSourceCode);}
void prosper::Shader::Initialize(bool bReloadSourceCode)
{
	auto bValidation = GetContext().IsValidationEnabled();
	if(bValidation)
		std::cout<<"[VK] Initializing shader '"<<GetIdentifier()<<"'"<<std::endl;
	m_bValid = false;
	ClearPipelines();
	if(bValidation)
		std::cout<<"[VK] Initializing shader sources..."<<std::endl;
	if(InitializeSources(bReloadSourceCode) == false)
		return;
	if(bValidation)
		std::cout<<"[VK] Initializing shader stages..."<<std::endl;
	InitializeStages();
	if(bValidation)
		std::cout<<"[VK] Initializing shader pipeline..."<<std::endl;
	InitializePipeline();
	m_bValid = true;

	OnPipelinesInitialized();
	if(m_bFirstTimeInit == true)
	{
		OnInitialized();
		m_bFirstTimeInit = false;
	}
	if(bValidation)
		std::cout<<"[VK] Shader successfully initialized!"<<std::endl;
}

void prosper::Shader::OnInitialized() {}
void prosper::Shader::OnPipelinesInitialized() {}
bool prosper::Shader::ShouldInitializePipeline(uint32_t pipelineIdx) {return true;}

void prosper::Shader::InitializePipeline() {}

void prosper::Shader::InitializeStages()
{
	auto &context = GetContext();
	auto &dev = static_cast<VlkContext&>(context).GetDevice();
	for(auto i=decltype(m_stages.size()){0};i<m_stages.size();++i)
	{
		auto &stage = m_stages.at(i);
		if(stage == nullptr)
			continue;
		stage->stage = static_cast<prosper::ShaderStage>(i);
		/*auto shaderPtr = Anvil::GLSLShaderToSPIRVGenerator::create(
			dev,
			Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
			stage->source,
			stage->stage
		);
		stage->module = Anvil::ShaderModule::create_from_spirv_generator(dev,shaderPtr);*/
		auto &spirvBlob = stage->spirvBlob;
		const std::string entryPointName = "main";
		stage->module = Anvil::ShaderModule::create_from_spirv_blob(
			&dev,
			spirvBlob.data(),spirvBlob.size(),
			(stage->stage == prosper::ShaderStage::Compute) ? entryPointName : "",
			(stage->stage == prosper::ShaderStage::Fragment) ? entryPointName : "",
			(stage->stage == prosper::ShaderStage::Geometry) ? entryPointName : "",
			(stage->stage == prosper::ShaderStage::TessellationControl) ? entryPointName : "",
			(stage->stage == prosper::ShaderStage::TessellationEvaluation) ? entryPointName : "",
			(stage->stage == prosper::ShaderStage::Vertex) ? entryPointName : ""
		);
		stage->entryPoint.reset(
			new Anvil::ShaderModuleStageEntryPoint(
				entryPointName,
				stage->module.get(),
				static_cast<Anvil::ShaderStage>(stage->stage)
			)
		);
	}
}

std::shared_ptr<prosper::IDescriptorSetGroup> prosper::Shader::CreateDescriptorSetGroup(uint32_t setIdx,uint32_t pipelineIdx) const
{
	auto *pipeline = GetPipelineInfo(pipelineIdx);
	if(pipeline == nullptr)
		return nullptr;
	auto *dsInfoItems = pipeline->get_ds_create_info_items();
	if(dsInfoItems == nullptr || setIdx >= dsInfoItems->size())
		return nullptr;
	return static_cast<VlkContext&>(GetContext()).CreateDescriptorSetGroup(Anvil::DescriptorSetCreateInfoUniquePtr(new Anvil::DescriptorSetCreateInfo(*dsInfoItems->at(setIdx))));
}

void prosper::Shader::InitializeDescriptorSetGroup(Anvil::BasePipelineCreateInfo &pipelineInfo)
{
	if(m_dsInfos.empty())
		return;
	std::vector<const Anvil::DescriptorSetCreateInfo*> dsInfos;
	dsInfos.reserve(m_dsInfos.size());
	for(auto &dsInfo : m_dsInfos)
		dsInfos.push_back(dsInfo.get());
	pipelineInfo.set_descriptor_set_create_info(&dsInfos);
	m_dsInfos.clear();
}
void prosper::Shader::SetDebugName(uint32_t pipelineIdx,const std::string &name)
{
	if(prosper::debug::is_debug_mode_enabled() == false || pipelineIdx >= m_pipelineInfos.size())
		return;
	m_pipelineInfos.at(pipelineIdx).debugName = name;
}
const std::string *prosper::Shader::GetDebugName(uint32_t pipelineIdx) const
{
	if(prosper::debug::is_debug_mode_enabled() == false || pipelineIdx >= m_pipelineInfos.size())
		return nullptr;
	return &m_pipelineInfos.at(pipelineIdx).debugName;
}
void prosper::Shader::OnPipelineInitialized(uint32_t pipelineIdx)
{
	SetDebugName(pipelineIdx,"shader_" +GetIdentifier() +"_pipeline" +std::to_string(pipelineIdx));
	auto *vkPipeline = GetPipelineManager()->get_pipeline(m_pipelineInfos.at(pipelineIdx).id);
	if(vkPipeline != nullptr)
		prosper::debug::register_debug_shader_pipeline(vkPipeline,{this,pipelineIdx});
}
void prosper::Shader::ClearPipelines()
{
	auto *pipelineManager = GetPipelineManager();
	if(pipelineManager == nullptr)
		return;
	GetContext().WaitIdle();
	for(auto &pipelineInfo : m_pipelineInfos)
	{
		if(pipelineInfo.id == std::numeric_limits<Anvil::PipelineID>::max())
			continue;
		prosper::debug::deregister_debug_object(pipelineManager->get_pipeline(pipelineInfo.id));
		pipelineManager->delete_pipeline(pipelineInfo.id);
		pipelineInfo.id = std::numeric_limits<Anvil::PipelineID>::max();
	}
}
bool prosper::Shader::GetSourceFilePath(ShaderStage stage,std::string &sourceFilePath) const
{
	auto *ptrStage = GetStage(stage);
	if(ptrStage == nullptr)
		return false;
	sourceFilePath = ptrStage->path;
	return true;
}
std::vector<std::string> prosper::Shader::GetSourceFilePaths() const
{
	std::vector<std::string> r {};
	r.reserve(m_stages.size());
	for(auto &stage : m_stages)
	{
		if(stage == nullptr)
			continue;
		r.push_back(stage->path);
	}
	return r;
}

Anvil::PipelineLayout *prosper::Shader::GetPipelineLayout(uint32_t pipelineIdx) const
{
	auto *pipelineManager = GetPipelineManager();
	if(pipelineManager == nullptr || IsValid() == false || pipelineIdx >= m_pipelineInfos.size())
		return nullptr;
	return pipelineManager->get_pipeline_layout(m_pipelineInfos.at(pipelineIdx).id);
}

const Anvil::BasePipelineCreateInfo *prosper::Shader::GetPipelineInfo(uint32_t pipelineIdx) const
{
	auto *pipelineManager = GetPipelineManager();
	if(pipelineManager == nullptr || IsValid() == false || pipelineIdx >= m_pipelineInfos.size())
		return nullptr;
	return pipelineManager->get_pipeline_create_info(m_pipelineInfos.at(pipelineIdx).id);
}
bool prosper::Shader::GetShaderStatistics(vk::ShaderStatisticsInfoAMD &stats,prosper::ShaderStage stage,uint32_t pipelineIdx) const
{
	auto *pipelineManager = GetPipelineManager();
	if(pipelineManager == nullptr || pipelineIdx >= m_pipelineInfos.size())
		return false;
	return pipelineManager->get_shader_statistics(m_pipelineInfos.at(pipelineIdx).id,static_cast<Anvil::ShaderStage>(stage),&reinterpret_cast<VkShaderStatisticsInfoAMD&>(stats));
}
void prosper::Shader::SetBaseShader(Shader &shader) {m_basePipeline = shader.shared_from_this();}
void prosper::Shader::ClearBaseShader() {m_basePipeline = {};}
const Anvil::ShaderModuleStageEntryPoint *prosper::Shader::GetModuleStageEntryPoint(prosper::ShaderStage stage,uint32_t pipelineIdx) const
{
	auto *info = GetPipelineInfo(pipelineIdx);
	if(info == nullptr)
		return nullptr;
	const Anvil::ShaderModuleStageEntryPoint *r;
	if(info->get_shader_stage_properties(static_cast<Anvil::ShaderStage>(stage),&r) == false)
		return nullptr;
	return r;
}
bool prosper::Shader::GetPipelineId(Anvil::PipelineID &pipelineId,uint32_t pipelineIdx) const
{
	if(pipelineIdx >= m_pipelineInfos.size())
		return false;
	pipelineId = m_pipelineInfos.at(pipelineIdx).id;
	return true;
}
size_t prosper::Shader::GetBaseTypeHashCode() const {return typeid(Shader).hash_code();}

bool prosper::Shader::RecordPushConstants(uint32_t size,const void *data,uint32_t offset)
{
	if(m_currentPipelineIdx == std::numeric_limits<uint32_t>::max())
		return false;
	auto cmdBuffer = GetCurrentCommandBuffer();
	auto *info = GetPipelineInfo(m_currentPipelineIdx);
	if(cmdBuffer == nullptr || info == nullptr)
		return false;
	for(auto &range : info->get_push_constant_ranges())
	{
		if(offset >= range.offset && (offset +size) <= (range.offset +range.size))
			return cmdBuffer->RecordPushConstants(*GetPipelineLayout(m_currentPipelineIdx),static_cast<ShaderStageFlags>(range.stages.get_vk()),offset,size,data);
	}
	return false;
}

bool prosper::Shader::RecordBindDescriptorSets(const std::vector<prosper::IDescriptorSet*> &descSets,uint32_t firstSet,const std::vector<uint32_t> &dynamicOffsets)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && cmdBuffer->RecordBindDescriptorSets(GetPipelineBindPoint(),*GetPipelineLayout(m_currentPipelineIdx),firstSet,descSets,dynamicOffsets);
}

bool prosper::Shader::RecordBindDescriptorSet(prosper::IDescriptorSet &descSet,uint32_t firstSet,const std::vector<uint32_t> &dynamicOffsets)
{
#if 0
	if(GetContext().IsValidationEnabled())
	{
		// Make sure all input images have the VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL layout
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
						// Does not work?
						// descSet.get_storage_image_binding_properties(i,j,nullptr,&imgView);
						break;
				}
				if(imgView == nullptr)
					continue;
				auto *img = imgView->get_create_info_ptr()->get_parent_image();
				if(img == nullptr)
					continue;
				ImageLayout imgLayout;
				if(prosper::debug::get_last_recorded_image_layout(*GetCurrentCommandBuffer(),img,imgLayout) == false || imgLayout == ImageLayout::ShaderReadOnlyOptimal)
					continue;
				debug::exec_debug_validation_callback(
					vk::DebugReportObjectTypeEXT::eImage,"Bind descriptor set: Image 0x" +::util::to_hex_string(reinterpret_cast<uint64_t>(img->get_image())) +
					" has to be in layout " +vk::to_string(vk::ImageLayout::eShaderReadOnlyOptimal) +", but is in layout " +
					vk::to_string(static_cast<vk::ImageLayout>(imgLayout)) +"!"
				);
			}
		}
	}
#endif
	auto cmdBuffer = GetCurrentCommandBuffer();
	auto *ptrDescSet = &descSet;
	return cmdBuffer != nullptr && cmdBuffer->RecordBindDescriptorSets(GetPipelineBindPoint(),*GetPipelineLayout(m_currentPipelineIdx),firstSet,{ptrDescSet},dynamicOffsets);
}

static std::unordered_map<prosper::ICommandBuffer*,std::pair<prosper::Shader*,uint32_t>> s_boundShaderPipeline = {};
const std::unordered_map<prosper::ICommandBuffer*,std::pair<prosper::Shader*,uint32_t>> &prosper::Shader::GetBoundPipelines() {return s_boundShaderPipeline;}
prosper::Shader *prosper::Shader::GetBoundPipeline(prosper::ICommandBuffer &cmdBuffer,uint32_t &outPipelineIdx)
{
	auto it = s_boundShaderPipeline.find(&cmdBuffer);
	if(it == s_boundShaderPipeline.end())
		return nullptr;
	outPipelineIdx = it->second.second;
	return it->second.first;
}
void prosper::Shader::SetCurrentDrawCommandBuffer(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &cmdBuffer,uint32_t pipelineIdx)
{
	if(cmdBuffer == nullptr)
	{
		m_currentCmd = {};
		return;
	}
	m_currentCmd = cmdBuffer;
	s_boundShaderPipeline[cmdBuffer.get()] = {std::pair<prosper::Shader*,uint32_t>{this,pipelineIdx}};
}
void prosper::Shader::UnbindPipeline()
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	auto it = s_boundShaderPipeline.find(cmdBuffer.get());
	if(it != s_boundShaderPipeline.end())
		s_boundShaderPipeline.erase(it);
	m_currentPipelineIdx = std::numeric_limits<uint32_t>::max();
	OnPipelineUnbound();
}
bool prosper::Shader::BindPipeline(prosper::ICommandBuffer&cmdBuffer,uint32_t pipelineIdx)
{
	if(pipelineIdx >= m_pipelineInfos.size())
		return false;
	m_currentPipelineIdx = pipelineIdx;
	auto pipelineId = m_pipelineInfos.at(m_currentPipelineIdx).id;
	auto r = pipelineId != std::numeric_limits<Anvil::PipelineID>::max() && cmdBuffer.RecordBindPipeline(m_pipelineBindPoint,static_cast<Anvil::PipelineID>(pipelineId));
	if(r == true)
		OnPipelineBound();
	return r;
}

Anvil::DescriptorSetCreateInfoUniquePtr prosper::DescriptorSetInfo::ToAnvilDescriptorSetInfo() const
{
	auto dsInfo = Anvil::DescriptorSetCreateInfo::create();
	for(auto &binding : bindings)
	{
		dsInfo->add_binding(
			binding.bindingIndex,static_cast<Anvil::DescriptorType>(binding.type),binding.descriptorArraySize,
			static_cast<Anvil::ShaderStageFlagBits>(binding.shaderStages)
		);
	}
	return dsInfo;
}
std::unique_ptr<Anvil::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::Bake()
{
	if(m_bWasBaked == false)
	{
		m_bWasBaked = true;
		auto nextIdx = 0u;
		for(auto &binding : bindings)
		{
			auto &bindingIdx = binding.bindingIndex;
			if(bindingIdx == std::numeric_limits<uint32_t>::max())
				bindingIdx = nextIdx;
			++nextIdx;
			//nextIdx = bindingIdx +binding.descriptorArraySize;
		}
	}
	return ToAnvilDescriptorSetInfo();
}

void prosper::Shader::AddDescriptorSetGroup(Anvil::BasePipelineCreateInfo &pipelineInfo,DescriptorSetInfo &descSetInfo)
{
	if(descSetInfo.parent != nullptr)
	{
		auto &parent = *descSetInfo.parent;
		descSetInfo.parent = nullptr;
		auto childBindings = descSetInfo.bindings;
		descSetInfo.bindings = parent.bindings;

		descSetInfo.bindings.reserve(descSetInfo.bindings.size() +childBindings.size());
		for(auto &childBinding : childBindings)
			descSetInfo.bindings.push_back(childBinding);
	}
	m_dsInfos.emplace_back(descSetInfo.Bake());
	descSetInfo.setIndex = m_dsInfos.size() -1u;
	if(m_dsInfos.size() > 8)
		throw std::logic_error("Attempted to add more than 8 descriptor sets to shader. This exceeds the limit on some GPUs!");
}

::util::WeakHandle<const prosper::Shader> prosper::Shader::GetHandle() const {return ::util::WeakHandle<const Shader>(shared_from_this());}
::util::WeakHandle<prosper::Shader> prosper::Shader::GetHandle() {return ::util::WeakHandle<Shader>(shared_from_this());}

bool prosper::Shader::AttachPushConstantRange(Anvil::BasePipelineCreateInfo &pipelineInfo,uint32_t offset,uint32_t size,prosper::ShaderStageFlags stages) {return pipelineInfo.attach_push_constant_range(offset,size,static_cast<Anvil::ShaderStageFlagBits>(stages));}

///////////////////////////

void prosper::util::set_graphics_pipeline_polygon_mode(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::PolygonMode polygonMode)
{
	Anvil::CullModeFlags cullModeFlags;
	Anvil::FrontFace frontFace;
	float lineWidth;
	pipelineInfo.get_rasterization_properties(nullptr,&cullModeFlags,&frontFace,&lineWidth);
	pipelineInfo.set_rasterization_properties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_line_width(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,float lineWidth)
{
	Anvil::CullModeFlags cullModeFlags;
	Anvil::FrontFace frontFace;
	Anvil::PolygonMode polygonMode;
	pipelineInfo.get_rasterization_properties(&polygonMode,&cullModeFlags,&frontFace,nullptr);
	pipelineInfo.set_rasterization_properties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_cull_mode_flags(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::CullModeFlags cullModeFlags)
{
	Anvil::FrontFace frontFace;
	Anvil::PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.get_rasterization_properties(&polygonMode,nullptr,&frontFace,&lineWidth);
	pipelineInfo.set_rasterization_properties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_front_face(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,Anvil::FrontFace frontFace)
{
	Anvil::CullModeFlags cullModeFlags;
	Anvil::PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.get_rasterization_properties(&polygonMode,&cullModeFlags,nullptr,&lineWidth);
	pipelineInfo.set_rasterization_properties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_generic_alpha_color_blend_attachment_properties(Anvil::GraphicsPipelineCreateInfo &pipelineInfo)
{
	pipelineInfo.set_color_blend_attachment_properties(
		0u,true,Anvil::BlendOp::ADD,Anvil::BlendOp::ADD,
		Anvil::BlendFactor::SRC_ALPHA,Anvil::BlendFactor::ONE_MINUS_SRC_ALPHA,
		Anvil::BlendFactor::SRC_ALPHA,Anvil::BlendFactor::ONE_MINUS_SRC_ALPHA,
		Anvil::ColorComponentFlagBits::R_BIT | Anvil::ColorComponentFlagBits::G_BIT | Anvil::ColorComponentFlagBits::B_BIT | Anvil::ColorComponentFlagBits::A_BIT
	);
}
static Anvil::DynamicState get_anvil_dynamic_state(prosper::util::DynamicStateFlags state)
{
	switch(state)
	{
		case prosper::util::DynamicStateFlags::Viewport:
			return Anvil::DynamicState::VIEWPORT;
		case prosper::util::DynamicStateFlags::Scissor:
			return Anvil::DynamicState::SCISSOR;
		case prosper::util::DynamicStateFlags::LineWidth:
			return Anvil::DynamicState::LINE_WIDTH;
		case prosper::util::DynamicStateFlags::DepthBias:
			return Anvil::DynamicState::DEPTH_BIAS;
		case prosper::util::DynamicStateFlags::BlendConstants:
			return Anvil::DynamicState::BLEND_CONSTANTS;
		case prosper::util::DynamicStateFlags::DepthBounds:
			return Anvil::DynamicState::DEPTH_BOUNDS;
		case prosper::util::DynamicStateFlags::StencilCompareMask:
			return Anvil::DynamicState::STENCIL_COMPARE_MASK;
		case prosper::util::DynamicStateFlags::StencilWriteMask:
			return Anvil::DynamicState::STENCIL_WRITE_MASK;
		case prosper::util::DynamicStateFlags::StencilReference:
			return Anvil::DynamicState::STENCIL_REFERENCE;
#if 0
		case prosper::util::DynamicStateFlags::ViewportWScalingNV:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportWScalingNV);
		case prosper::util::DynamicStateFlags::DiscardRectangleEXT:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eDiscardRectangleEXT);
		case prosper::util::DynamicStateFlags::SampleLocationsEXT:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eSampleLocationsEXT);
		case prosper::util::DynamicStateFlags::ViewportShadingRatePaletteNV:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportShadingRatePaletteNV);
		case prosper::util::DynamicStateFlags::ViewportCoarseSampleOrderNV:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportCoarseSampleOrderNV);
		case prosper::util::DynamicStateFlags::ExclusiveScissorNV:
			return static_cast<Anvil::DynamicState>(vk::DynamicState::eExclusiveScissorNV);
#endif
	}
	return Anvil::DynamicState{};
}
static prosper::util::DynamicStateFlags get_prosper_dynamic_state(Anvil::DynamicState state)
{
	switch(state)
	{
		case Anvil::DynamicState::VIEWPORT:
			return prosper::util::DynamicStateFlags::Viewport;
		case Anvil::DynamicState::SCISSOR:
			return prosper::util::DynamicStateFlags::Scissor;
		case Anvil::DynamicState::LINE_WIDTH:
			return prosper::util::DynamicStateFlags::LineWidth;
		case Anvil::DynamicState::DEPTH_BIAS:
			return prosper::util::DynamicStateFlags::DepthBias;
		case Anvil::DynamicState::BLEND_CONSTANTS:
			return prosper::util::DynamicStateFlags::BlendConstants;
		case Anvil::DynamicState::DEPTH_BOUNDS:
			return prosper::util::DynamicStateFlags::DepthBounds;
		case Anvil::DynamicState::STENCIL_COMPARE_MASK:
			return prosper::util::DynamicStateFlags::StencilCompareMask;
		case Anvil::DynamicState::STENCIL_WRITE_MASK:
			return prosper::util::DynamicStateFlags::StencilWriteMask;
		case Anvil::DynamicState::STENCIL_REFERENCE:
			return prosper::util::DynamicStateFlags::StencilReference;
#if 0
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportWScalingNV):
			return prosper::util::DynamicStateFlags::ViewportWScalingNV;
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eDiscardRectangleEXT):
			return prosper::util::DynamicStateFlags::DiscardRectangleEXT;
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eSampleLocationsEXT):
			return prosper::util::DynamicStateFlags::SampleLocationsEXT;
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportShadingRatePaletteNV):
			return prosper::util::DynamicStateFlags::ViewportShadingRatePaletteNV;
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eViewportCoarseSampleOrderNV):
			return prosper::util::DynamicStateFlags::ViewportCoarseSampleOrderNV;
		case static_cast<Anvil::DynamicState>(vk::DynamicState::eExclusiveScissorNV):
			return prosper::util::DynamicStateFlags::ExclusiveScissorNV;
#endif
	}
	return prosper::util::DynamicStateFlags::None;
}
static std::vector<Anvil::DynamicState> get_enabled_anvil_states(prosper::util::DynamicStateFlags states)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(states));
	std::vector<Anvil::DynamicState> r {};
	r.reserve(values.size());
	for(auto &v : values)
		r.push_back(get_anvil_dynamic_state(static_cast<prosper::util::DynamicStateFlags>(v)));
	return r;
}
void prosper::util::set_dynamic_states_enabled(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states,bool enabled)
{
	pipelineInfo.toggle_dynamic_states(enabled,get_enabled_anvil_states(states));
}
bool prosper::util::are_dynamic_states_enabled(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states)
{
	return (states &get_enabled_dynamic_states(pipelineInfo)) == states;
}
prosper::util::DynamicStateFlags prosper::util::get_enabled_dynamic_states(Anvil::GraphicsPipelineCreateInfo &pipelineInfo)
{
	const Anvil::DynamicState *states;
	auto numStates = 0u;
	pipelineInfo.get_enabled_dynamic_states(&states,&numStates);

	auto r = prosper::util::DynamicStateFlags::None;
	for(auto i=decltype(numStates){0u};i<numStates;++i)
		r |= get_prosper_dynamic_state(static_cast<Anvil::DynamicState>(states[i]));
	return r;
}
