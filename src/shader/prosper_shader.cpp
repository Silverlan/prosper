/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader.hpp"
#include "shader/prosper_pipeline_create_info.hpp"
#include "shader/prosper_pipeline_manager.hpp"
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

void prosper::Shader::SetPipelineCount(uint32_t count) {m_pipelineInfos.resize(count,{});}

bool prosper::Shader::IsGraphicsShader() const {return m_pipelineBindPoint == PipelineBindPoint::Graphics;}
bool prosper::Shader::IsComputeShader() const {return m_pipelineBindPoint == PipelineBindPoint::Compute;}
prosper::PipelineBindPoint prosper::Shader::GetPipelineBindPoint() const {return m_pipelineBindPoint;}

prosper::ShaderModule *prosper::Shader::GetModule(ShaderStage stage)
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
		stage->module = context.CreateShaderModuleFromSPIRVBlob(
			spirvBlob,
			stage->stage,
			entryPointName
		);
		stage->entryPoint.reset(
			new prosper::ShaderModuleStageEntryPoint(
				entryPointName,
				stage->module.get(),
				stage->stage
			)
		);
	}
}

std::shared_ptr<prosper::IDescriptorSetGroup> prosper::Shader::CreateDescriptorSetGroup(uint32_t setIdx,uint32_t pipelineIdx) const
{
	auto *pipeline = GetPipelineInfo(pipelineIdx);
	if(pipeline == nullptr)
		return nullptr;
	if(setIdx >= pipeline->descSetInfos.size())
		return nullptr;
	return static_cast<VlkContext&>(GetContext()).CreateDescriptorSetGroup(*pipeline->descSetInfos.at(setIdx));
}

void prosper::Shader::InitializeDescriptorSetGroup(prosper::BasePipelineCreateInfo &pipelineInfo)
{
	auto *pipeline = GetPipelineInfo(m_currentPipelineIdx);
	if(pipeline == nullptr)
		return;
	std::vector<const prosper::DescriptorSetCreateInfo*> dsInfos;
	dsInfos.reserve(pipeline->descSetInfos.size());
	for(auto &dsInfo : pipeline->descSetInfos)
		dsInfos.push_back(dsInfo.get());
	pipelineInfo.SetDescriptorSetCreateInfo(&dsInfos);
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
	// auto *vkPipeline = GetPipelineManager()->GetPipelineInfo(m_pipelineInfos.at(pipelineIdx).id);
	// if(vkPipeline != nullptr)
	// 	prosper::debug::register_debug_shader_pipeline(vkPipeline,{this,pipelineIdx});
}
void prosper::Shader::ClearPipelines()
{
	GetContext().WaitIdle();
	for(auto &pipelineInfo : m_pipelineInfos)
	{
		if(pipelineInfo.id == std::numeric_limits<Anvil::PipelineID>::max())
			continue;
		// prosper::debug::deregister_debug_object(pipelineManager->GetPipelineInfo(pipelineInfo.id));
		GetContext().ClearPipeline(IsGraphicsShader(),pipelineInfo.id);
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

void prosper::Shader::SetBaseShader(Shader &shader) {m_basePipeline = shader.shared_from_this();}
void prosper::Shader::ClearBaseShader() {m_basePipeline = {};}
const prosper::ShaderModuleStageEntryPoint *prosper::Shader::GetModuleStageEntryPoint(prosper::ShaderStage stage,uint32_t pipelineIdx) const
{
	auto *stageData = GetStage(stage);
	if(stageData == nullptr)
		return nullptr;
	return stageData->entryPoint.get();
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
	for(auto &range : info->pushConstantRanges)
	{
		if(offset >= range.offset && (offset +size) <= (range.offset +range.size))
			return cmdBuffer->RecordPushConstants(*this,m_currentPipelineIdx,range.stages,offset,size,data);
	}
	return false;
}

const prosper::PipelineInfo *prosper::Shader::GetPipelineInfo(PipelineID id) const {return const_cast<Shader*>(this)->GetPipelineInfo(id);}
prosper::PipelineInfo *prosper::Shader::GetPipelineInfo(PipelineID id)
{
	return (id < m_pipelineInfos.size()) ? &m_pipelineInfos.at(id) : nullptr;
}
prosper::BasePipelineCreateInfo *prosper::Shader::GetPipelineCreateInfo(PipelineID id)
{
	auto *info = GetPipelineInfo(id);
	return info ? info->createInfo.get() : nullptr;
}

bool prosper::Shader::RecordBindDescriptorSets(const std::vector<prosper::IDescriptorSet*> &descSets,uint32_t firstSet,const std::vector<uint32_t> &dynamicOffsets)
{
	auto cmdBuffer = GetCurrentCommandBuffer();
	return cmdBuffer != nullptr && cmdBuffer->RecordBindDescriptorSets(GetPipelineBindPoint(),*this,m_currentPipelineIdx,firstSet,descSets,dynamicOffsets);
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
	return cmdBuffer != nullptr && cmdBuffer->RecordBindDescriptorSets(GetPipelineBindPoint(),*this,m_currentPipelineIdx,firstSet,{ptrDescSet},dynamicOffsets);
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
std::unique_ptr<prosper::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::ToProsperDescriptorSetInfo() const
{
	auto dsInfo = DescriptorSetCreateInfo::Create();
	for(auto &binding : bindings)
	{
		dsInfo->AddBinding(
			binding.bindingIndex,binding.type,binding.descriptorArraySize,
			binding.shaderStages
		);
	}
	return dsInfo;
}
std::unique_ptr<Anvil::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::ToAnvilDescriptorSetInfo() const
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
std::unique_ptr<prosper::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::Bake()
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
	return ToProsperDescriptorSetInfo();
}

prosper::ShaderModule::ShaderModule(
	const std::vector<uint32_t> &spirvBlob,
	const std::string&          in_opt_cs_entrypoint_name,
	const std::string&          in_opt_fs_entrypoint_name,
	const std::string&          in_opt_gs_entrypoint_name,
	const std::string&          in_opt_tc_entrypoint_name,
	const std::string&          in_opt_te_entrypoint_name,
	const std::string&          in_opt_vs_entrypoint_name
)
	: m_csEntrypointName      (in_opt_cs_entrypoint_name),
	m_fsEntrypointName      (in_opt_fs_entrypoint_name),
	m_gsEntrypointName      (in_opt_gs_entrypoint_name),
	m_tcEntrypointName      (in_opt_tc_entrypoint_name),
	m_teEntrypointName      (in_opt_te_entrypoint_name),
	m_vsEntrypointName      (in_opt_vs_entrypoint_name),
	m_spirvData{spirvBlob}
{}

const std::optional<std::vector<uint32_t>> &prosper::ShaderModule::GetSPIRVData() const {return m_spirvData;}

void prosper::Shader::AddDescriptorSetGroup(prosper::BasePipelineCreateInfo &pipelineInfo,DescriptorSetInfo &descSetInfo)
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
	auto &pipelineInitInfo = m_pipelineInfos.at(m_currentPipelineIdx);
	pipelineInitInfo.descSetInfos.emplace_back(descSetInfo.Bake());
	descSetInfo.setIndex = pipelineInitInfo.descSetInfos.size() -1u;
	if(pipelineInitInfo.descSetInfos.size() > 8)
		throw std::logic_error("Attempted to add more than 8 descriptor sets to shader. This exceeds the limit on some GPUs!");
}

::util::WeakHandle<const prosper::Shader> prosper::Shader::GetHandle() const {return ::util::WeakHandle<const Shader>(shared_from_this());}
::util::WeakHandle<prosper::Shader> prosper::Shader::GetHandle() {return ::util::WeakHandle<Shader>(shared_from_this());}

bool prosper::Shader::AttachPushConstantRange(prosper::BasePipelineCreateInfo &pipelineInfo,uint32_t offset,uint32_t size,prosper::ShaderStageFlags stages)
{
	auto &pipelineInitInfo = m_pipelineInfos.at(m_currentPipelineIdx);
	pipelineInitInfo.pushConstantRanges.push_back({offset,size,stages});
	return pipelineInfo.AttachPushConstantRange(offset,size,stages);
}

///////////////////////////

void prosper::util::set_graphics_pipeline_polygon_mode(prosper::GraphicsPipelineCreateInfo &pipelineInfo,prosper::PolygonMode polygonMode)
{
	prosper::CullModeFlags cullModeFlags;
	prosper::FrontFace frontFace;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(nullptr,&cullModeFlags,&frontFace,&lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_line_width(prosper::GraphicsPipelineCreateInfo &pipelineInfo,float lineWidth)
{
	prosper::CullModeFlags cullModeFlags;
	prosper::FrontFace frontFace;
	prosper::PolygonMode polygonMode;
	pipelineInfo.GetRasterizationProperties(&polygonMode,&cullModeFlags,&frontFace,nullptr);
	pipelineInfo.SetRasterizationProperties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_cull_mode_flags(prosper::GraphicsPipelineCreateInfo &pipelineInfo,prosper::CullModeFlags cullModeFlags)
{
	prosper::FrontFace frontFace;
	prosper::PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(&polygonMode,nullptr,&frontFace,&lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_graphics_pipeline_front_face(prosper::GraphicsPipelineCreateInfo &pipelineInfo,prosper::FrontFace frontFace)
{
	prosper::CullModeFlags cullModeFlags;
	prosper::PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(&polygonMode,&cullModeFlags,nullptr,&lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode,cullModeFlags,frontFace,lineWidth);
}
void prosper::util::set_generic_alpha_color_blend_attachment_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo)
{
	pipelineInfo.SetColorBlendAttachmentProperties(
		0u,true,prosper::BlendOp::Add,prosper::BlendOp::Add,
		prosper::BlendFactor::SrcAlpha,prosper::BlendFactor::OneMinusSrcAlpha,
		prosper::BlendFactor::SrcAlpha,prosper::BlendFactor::OneMinusSrcAlpha,
		prosper::ColorComponentFlags::RBit | prosper::ColorComponentFlags::GBit | prosper::ColorComponentFlags::BBit | prosper::ColorComponentFlags::ABit
	);
}
static prosper::DynamicState get_anvil_dynamic_state(prosper::util::DynamicStateFlags state)
{
	switch(state)
	{
		case prosper::util::DynamicStateFlags::Viewport:
			return prosper::DynamicState::Viewport;
		case prosper::util::DynamicStateFlags::Scissor:
			return prosper::DynamicState::Scissor;
		case prosper::util::DynamicStateFlags::LineWidth:
			return prosper::DynamicState::LineWidth;
		case prosper::util::DynamicStateFlags::DepthBias:
			return prosper::DynamicState::DepthBias;
		case prosper::util::DynamicStateFlags::BlendConstants:
			return prosper::DynamicState::BlendConstants;
		case prosper::util::DynamicStateFlags::DepthBounds:
			return prosper::DynamicState::DepthBounds;
		case prosper::util::DynamicStateFlags::StencilCompareMask:
			return prosper::DynamicState::StencilCompareMask;
		case prosper::util::DynamicStateFlags::StencilWriteMask:
			return prosper::DynamicState::StencilWriteMask;
		case prosper::util::DynamicStateFlags::StencilReference:
			return prosper::DynamicState::StencilReference;
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
	return prosper::DynamicState{};
}
static prosper::util::DynamicStateFlags get_prosper_dynamic_state(prosper::DynamicState state)
{
	switch(state)
	{
		case prosper::DynamicState::Viewport:
			return prosper::util::DynamicStateFlags::Viewport;
		case prosper::DynamicState::Scissor:
			return prosper::util::DynamicStateFlags::Scissor;
		case prosper::DynamicState::LineWidth:
			return prosper::util::DynamicStateFlags::LineWidth;
		case prosper::DynamicState::DepthBias:
			return prosper::util::DynamicStateFlags::DepthBias;
		case prosper::DynamicState::BlendConstants:
			return prosper::util::DynamicStateFlags::BlendConstants;
		case prosper::DynamicState::DepthBounds:
			return prosper::util::DynamicStateFlags::DepthBounds;
		case prosper::DynamicState::StencilCompareMask:
			return prosper::util::DynamicStateFlags::StencilCompareMask;
		case prosper::DynamicState::StencilWriteMask:
			return prosper::util::DynamicStateFlags::StencilWriteMask;
		case prosper::DynamicState::StencilReference:
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
static std::vector<prosper::DynamicState> get_enabled_dynamic_states(prosper::util::DynamicStateFlags states)
{
	auto values = umath::get_power_of_2_values(umath::to_integral(states));
	std::vector<prosper::DynamicState> r {};
	r.reserve(values.size());
	for(auto &v : values)
		r.push_back(get_anvil_dynamic_state(static_cast<prosper::util::DynamicStateFlags>(v)));
	return r;
}
void prosper::util::set_dynamic_states_enabled(prosper::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states,bool enabled)
{
	pipelineInfo.ToggleDynamicStates(enabled,::get_enabled_dynamic_states(states));
}
bool prosper::util::are_dynamic_states_enabled(prosper::GraphicsPipelineCreateInfo &pipelineInfo,DynamicStateFlags states)
{
	return (states &get_enabled_dynamic_states(pipelineInfo)) == states;
}
prosper::util::DynamicStateFlags prosper::util::get_enabled_dynamic_states(prosper::GraphicsPipelineCreateInfo &pipelineInfo)
{
	const prosper::DynamicState *states;
	auto numStates = 0u;
	pipelineInfo.GetEnabledDynamicStates(&states,&numStates);

	auto r = prosper::util::DynamicStateFlags::None;
	for(auto i=decltype(numStates){0u};i<numStates;++i)
		r |= get_prosper_dynamic_state(static_cast<prosper::DynamicState>(states[i]));
	return r;
}
