// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.pipeline_create_info;
import :shader_system.pipeline_loader;
import :shader_system.shader;

prosper::ShaderBindState::ShaderBindState(ICommandBuffer &cmdBuf) : commandBuffer {cmdBuf} {}

///////////////////////////

prosper::DescriptorSetInfo::DescriptorSetInfo(const char *name, const std::vector<Binding> &bindings) : m_name {name}, bindings(bindings) {}
prosper::DescriptorSetInfo::DescriptorSetInfo(DescriptorSetInfo *parent, const std::vector<Binding> &bindings) : m_name {nullptr}, parent(parent), bindings(bindings) {}
bool prosper::DescriptorSetInfo::WasBaked() const { return m_bWasBaked; }
bool prosper::DescriptorSetInfo::IsValid() const { return WasBaked(); }
const char *prosper::DescriptorSetInfo::GetName() const { return parent ? parent->GetName() : m_name; }
prosper::detail::DescriptorSetInfoBinding::DescriptorSetInfoBinding(const char *name, DescriptorType type, ShaderStageFlags shaderStages, uint32_t descriptorArraySize, uint32_t bindingIndex, PrDescriptorSetBindingFlags flags)
    : name {name}, bindingIndex(bindingIndex), type(type), shaderStages(shaderStages), descriptorArraySize(descriptorArraySize), flags {flags}
{
}

prosper::detail::DescriptorSetInfoBinding::DescriptorSetInfoBinding(const char *name, DescriptorType type, ShaderStageFlags shaderStages, PrDescriptorSetBindingFlags flags) : DescriptorSetInfoBinding {name, type, shaderStages, 1u, std::numeric_limits<uint32_t>::max(), flags} {}

///////////////////////////

decltype(prosper::Shader::s_logCallback) prosper::Shader::s_logCallback = nullptr;
void prosper::Shader::SetLogCallback(const std::function<void(Shader &, ShaderStage, const std::string &, const std::string &)> &fLogCallback) { s_logCallback = fLogCallback; }
const std::function<void(prosper::Shader &, prosper::ShaderStage, const std::string &, const std::string &)> &prosper::Shader::GetLogCallback() { return s_logCallback; }

prosper::Shader::Shader(IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader)
    : std::enable_shared_from_this<Shader>(), ContextObject(context), m_pipelineBindPoint(PipelineBindPoint::Graphics), m_identifier(identifier)
{
	if(vsShader.empty() == false)
		(m_stages.at(pragma::math::to_integral(ShaderStage::Vertex)) = std::make_shared<ShaderStageData>())->path = vsShader;
	if(fsShader.empty() == false)
		(m_stages.at(pragma::math::to_integral(ShaderStage::Fragment)) = std::make_shared<ShaderStageData>())->path = fsShader;
	if(gsShader.empty() == false)
		(m_stages.at(pragma::math::to_integral(ShaderStage::Geometry)) = std::make_shared<ShaderStageData>())->path = gsShader;
	SetPipelineCount(1u);
}
prosper::Shader::Shader(IPrContext &context, const std::string &identifier, const std::string &csShader) : ContextObject(context), m_pipelineBindPoint(PipelineBindPoint::Compute), m_identifier(identifier)
{
	if(csShader.empty() == false)
		(m_stages.at(pragma::math::to_integral(ShaderStage::Compute)) = std::make_shared<ShaderStageData>())->path = csShader;
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
std::array<std::shared_ptr<prosper::ShaderStageData>, pragma::math::to_integral(prosper::ShaderStage::Count)> &prosper::Shader::GetStages() { return m_stages; }
void prosper::Shader::SetStageSourceFilePath(ShaderStage stage, const std::string &filePath)
{
	if(filePath.empty()) {
		m_stages.at(pragma::math::to_integral(stage)) = nullptr;
		return;
	}
	(m_stages.at(pragma::math::to_integral(stage)) = std::make_shared<ShaderStageData>())->path = filePath;
	switch(stage) {
	case ShaderStage::Compute:
		m_pipelineBindPoint = PipelineBindPoint::Compute;
		break;
	default:
		m_pipelineBindPoint = PipelineBindPoint::Graphics;
		break;
	}
}

std::optional<std::string> prosper::Shader::GetStageSourceFilePath(ShaderStage stage) const
{
	auto &stageData = m_stages.at(pragma::math::to_integral(stage));
	if(stageData == nullptr)
		return {};
	return stageData->path;
}

const std::string &prosper::Shader::GetIdentifier() const { return m_identifier; }

void prosper::Shader::SetPipelineCount(uint32_t count, bool flushLoad)
{
	if(flushLoad)
		FlushLoad();
	m_pipelineInfos.resize(count, {});
	m_cachedPipelineIds.resize(count, std::numeric_limits<PipelineID>::max());
}

bool prosper::Shader::IsGraphicsShader() const { return m_pipelineBindPoint == PipelineBindPoint::Graphics; }
bool prosper::Shader::IsComputeShader() const { return m_pipelineBindPoint == PipelineBindPoint::Compute; }
bool prosper::Shader::IsRaytracingShader() const { return m_pipelineBindPoint == PipelineBindPoint::RayTracingKHR; }
prosper::PipelineBindPoint prosper::Shader::GetPipelineBindPoint() const { return m_pipelineBindPoint; }

prosper::ShaderModule *prosper::Shader::GetModule(ShaderStage stage)
{
	auto *stageModule = GetStage(stage);
	if(stageModule == nullptr)
		return nullptr;
	return stageModule->module.get();
}

prosper::ShaderStageData *prosper::Shader::GetStage(ShaderStage stage)
{
	auto &stageModule = m_stages.at(pragma::math::to_integral(stage));
	return stageModule.get();
}

const prosper::ShaderStageData *prosper::Shader::GetStage(ShaderStage stage) const { return const_cast<Shader *>(this)->GetStage(stage); }

static std::string g_shaderLocation = "shaders";
void prosper::Shader::SetRootShaderLocation(const std::string &location) { g_shaderLocation = location; }
const std::string &prosper::Shader::GetRootShaderLocation() { return g_shaderLocation; }

void prosper::Shader::GetShaderPreprocessorDefinitions(std::unordered_map<std::string, std::string> &outDefinitions, std::string &outPrefixCode)
{
	auto &context = GetContext();
	std::array<uint32_t, pragma::math::to_integral(DescriptorResourceType::Count)> resourceBindingIndices;
	for(size_t i = 0; i < resourceBindingIndices.size(); ++i)
		resourceBindingIndices[i] = context.GetReservedDescriptorResourceCount(static_cast<DescriptorResourceType>(i));
	for(size_t iSet = 0; auto &dsInfo : m_shaderResources.descSetInfos) {
		std::string dsName {dsInfo->GetName()};
		pragma::string::to_upper(dsName);
		auto glslDsName = "DESCRIPTOR_SET_" + dsName;
		outDefinitions[glslDsName] = std::to_string(iSet);
		auto numBindings = dsInfo->GetBindingCount();
		for(size_t iBinding = 0; iBinding < numBindings; ++iBinding) {
			std::string bindingName {dsInfo->GetBindingName(iBinding)};
			pragma::string::to_upper(bindingName);
			auto glslBindingName = glslDsName + "_BINDING_" + bindingName;
			outDefinitions[glslBindingName] = std::to_string(iBinding);

			DescriptorType descType;
			uint32_t arraySize;
			if(dsInfo->GetBindingPropertiesByBindingIndex(iBinding, &descType, &arraySize)) {
				auto resType = get_descriptor_resource_type(descType);
				if(resType) {
					auto &idx = resourceBindingIndices[pragma::math::to_integral(*resType)];
					auto glslRecBindingName = glslDsName + "_RESOURCE_BINDING_" + bindingName;
					outDefinitions[glslRecBindingName] = std::to_string(idx);
					idx += arraySize;
				}
			}
		}
		++iSet;
	}
}

bool prosper::Shader::InitializeSources(bool bReload)
{
	auto &context = GetContext();
	std::string infoLog;
	std::string debugInfoLog;

	std::unordered_map<std::string, std::string> definitions;
	std::string prefixCode;
	GetShaderPreprocessorDefinitions(definitions, prefixCode);

	ShaderStage stage;
	if(context.InitializeShaderSources(*this, bReload, infoLog, debugInfoLog, stage, prefixCode, definitions) == false) {
		if(s_logCallback != nullptr)
			s_logCallback(*this, stage, infoLog, debugInfoLog);
		return false;
	}
	return true;
}

bool prosper::Shader::IsValid() const { return m_bValid; }

void prosper::Shader::SetIdentifier(const std::string &identifier) { m_identifier = identifier; }

uint32_t prosper::Shader::GetPipelineCount() const
{
	FlushLoad();
	return m_pipelineInfos.size();
}
void prosper::Shader::ReloadPipelines(bool bReloadSourceCode) { Initialize(bReloadSourceCode); }
void prosper::Shader::Initialize(bool bReloadSourceCode)
{
	auto &context = GetContext();
	auto &loader = context.GetPipelineLoader();
	if(loader.IsShaderQueued(GetIndex()))
		FlushLoad();

	auto shouldLog = context.ShouldLog(pragma::util::LogSeverity::Debug);
	if(shouldLog)
		context.Log("Initializing shader '" + GetIdentifier() + "'", pragma::util::LogSeverity::Debug);
	m_bValid = false;
	m_loading = true;
	ClearPipelines();

	ClearShaderResources();
	InitializeShaderResources();

	auto fInit = [this, shouldLog, &context, bReloadSourceCode]() -> bool {
		if(shouldLog)
			context.Log("Initializing shader sources for '" + GetIdentifier() + "'...", pragma::util::LogSeverity::Debug);
		context.StartProfiling("Initialize shader sources");
		auto res = InitializeSources(bReloadSourceCode);
		context.EndProfiling();
		if(res == false)
			return false;
		if(shouldLog)
			context.Log("Initializing shader stages for '" + GetIdentifier() + "'...", pragma::util::LogSeverity::Debug);
		context.StartProfiling("Initialize shader stages");
		InitializeStages();
		context.EndProfiling();
		if(shouldLog)
			context.Log("Initializing shader pipelines for '" + GetIdentifier() + "'...", pragma::util::LogSeverity::Debug);
		if(m_enableMultiThreadedPipelineInitialization) {
			context.StartProfiling("Initialize shader pipelines");
			InitializePipeline();
			context.EndProfiling();
		}
		if(shouldLog)
			context.Log("Initialization of shader '" + GetIdentifier() + "' is complete.", pragma::util::LogSeverity::Debug);
		return true;
	};
	loader.Init(GetIndex(), fInit);
}
void prosper::Shader::FinalizeInitialization()
{
	m_bValid = true;
	m_loading = false;
	if(!m_enableMultiThreadedPipelineInitialization)
		InitializePipeline();
	OnPipelinesInitialized();
	if(m_bFirstTimeInit == true) {
		OnInitialized();
		m_bFirstTimeInit = false;
	}
	auto bValidation = GetContext().IsValidationEnabled();
	if(bValidation)
		std::cout << "[PR] Shader successfully initialized!" << std::endl;
}

void prosper::Shader::OnInitialized() {}
void prosper::Shader::OnPipelinesInitialized() {}
bool prosper::Shader::ShouldInitializePipeline(uint32_t pipelineIdx) { return true; }
void prosper::Shader::ClearShaderResources() { m_shaderResources = {}; }

void prosper::Shader::InitializePipeline() {}

void prosper::Shader::InitializeStages()
{
	auto &context = GetContext();
	for(auto i = decltype(m_stages.size()) {0}; i < m_stages.size(); ++i) {
		auto &stage = m_stages.at(i);
		if(stage == nullptr)
			continue;
		stage->stage = static_cast<ShaderStage>(i);
		/*auto shaderPtr = Anvil::GLSLShaderToSPIRVGenerator::create(
			dev,
			Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
			stage->source,
			stage->stage
		);
		stage->module = Anvil::ShaderModule::create_from_spirv_generator(dev,shaderPtr);*/
		const std::string entryPointName = "main";
		stage->module = context.CreateShaderModuleFromStageData(stage->program, stage->stage, entryPointName);
		stage->entryPoint.reset(new ShaderModuleStageEntryPoint(entryPointName, stage->module.get(), stage->stage));
	}
}

std::shared_ptr<prosper::IDescriptorSetGroup> prosper::Shader::CreateDescriptorSetGroup(uint32_t setIdx) const
{
	if(setIdx >= m_shaderResources.descSetInfos.size())
		return nullptr;
	return GetContext().CreateDescriptorSetGroup(*m_shaderResources.descSetInfos.at(setIdx));
}

void prosper::Shader::InitializeDescriptorSetGroups(BasePipelineCreateInfo &pipelineInfo)
{
	std::vector<const DescriptorSetCreateInfo *> dsInfos;
	dsInfos.reserve(m_shaderResources.descSetInfos.size());
	for(auto &dsInfo : m_shaderResources.descSetInfos)
		dsInfos.push_back(dsInfo.get());
	pipelineInfo.SetDescriptorSetCreateInfo(&dsInfos);
}

void prosper::Shader::SetDebugName(uint32_t pipelineIdx, const std::string &name)
{
	auto &infos = GetPipelineInfos();
	if(/*prosper::debug::is_debug_mode_enabled() == false || */ pipelineIdx >= infos.size())
		return;
	infos[pipelineIdx].debugName = name;
}
const std::string *prosper::Shader::GetDebugName(uint32_t pipelineIdx) const
{
	auto &infos = const_cast<Shader *>(this)->GetPipelineInfos();
	if(/*prosper::debug::is_debug_mode_enabled() == false || */ pipelineIdx >= infos.size())
		return nullptr;
	return &infos[pipelineIdx].debugName;
}
void prosper::Shader::OnPipelineInitialized(uint32_t pipelineIdx)
{
	SetDebugName(pipelineIdx, "shader_" + GetIdentifier() + "_pipeline" + std::to_string(pipelineIdx));
	// auto *vkPipeline = GetPipelineManager()->GetPipelineInfo(m_pipelineInfos.at(pipelineIdx).id);
	// if(vkPipeline != nullptr)
	// 	prosper::debug::register_debug_shader_pipeline(vkPipeline,{this,pipelineIdx});
}
void prosper::Shader::ClearPipelines()
{
	GetContext().WaitIdle();
	auto &loader = GetContext().GetPipelineLoader();
	if(loader.IsShaderQueued(GetIndex()))
		FlushLoad();
	for(auto &pipelineInfo : m_pipelineInfos) {
		if(pipelineInfo.id == std::numeric_limits<PipelineID>::max())
			continue;
		// prosper::debug::deregister_debug_object(pipelineManager->GetPipelineInfo(pipelineInfo.id));
		GetContext().ClearPipeline(IsGraphicsShader(), pipelineInfo.id);
		pipelineInfo.id = std::numeric_limits<PipelineID>::max();
	}
}
bool prosper::Shader::GetSourceFilePath(ShaderStage stage, std::string &sourceFilePath) const
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
	for(auto &stage : m_stages) {
		if(stage == nullptr)
			continue;
		r.push_back(stage->path);
	}
	return r;
}

void prosper::Shader::SetBaseShader(Shader &shader) { m_basePipeline = shader.shared_from_this(); }
void prosper::Shader::ClearBaseShader() { m_basePipeline = {}; }
const prosper::ShaderModuleStageEntryPoint *prosper::Shader::GetModuleStageEntryPoint(ShaderStage stage, uint32_t pipelineIdx) const
{
	auto *stageData = GetStage(stage);
	if(stageData == nullptr)
		return nullptr;
	return stageData->entryPoint.get();
}
bool prosper::Shader::GetPipelineId(PipelineID &pipelineId, uint32_t pipelineIdx, bool waitForLoad) const
{
	const PipelineInfo *info = nullptr;
	if(!waitForLoad)
		info = (pipelineIdx < m_pipelineInfos.size()) ? &m_pipelineInfos[pipelineIdx] : nullptr;
	else
		info = GetPipelineInfo(pipelineIdx);
	if(!info)
		return false;
	pipelineId = info->id;
	return true;
}
size_t prosper::Shader::GetBaseTypeHashCode() const { return typeid(Shader).hash_code(); }

bool prosper::Shader::RecordPushConstants(ShaderBindState &bindState, uint32_t size, const void *data, uint32_t offset) const
{
	if(bindState.pipelineIdx == std::numeric_limits<uint32_t>::max())
		return false;
	for(auto &range : m_shaderResources.pushConstantRanges) {
		if(offset >= range.offset && (offset + size) <= (range.offset + range.size))
			return bindState.commandBuffer.RecordPushConstants(const_cast<Shader &>(*this), bindState.pipelineIdx, range.stages, offset, size, data);
	}
	return false;
}

const prosper::PipelineInfo *prosper::Shader::GetPipelineInfo(PipelineID id) const { return const_cast<Shader *>(this)->GetPipelineInfo(id); }
prosper::PipelineInfo *prosper::Shader::GetPipelineInfo(PipelineID id)
{
	FlushLoad();
	return (id < m_pipelineInfos.size()) ? &m_pipelineInfos.at(id) : nullptr;
}
const prosper::BasePipelineCreateInfo *prosper::Shader::GetPipelineCreateInfo(PipelineID id) const { return const_cast<Shader *>(this)->GetPipelineCreateInfo(id); }
prosper::BasePipelineCreateInfo *prosper::Shader::GetPipelineCreateInfo(PipelineID id)
{
	auto *info = GetPipelineInfo(id);
	return info ? info->createInfo.get() : nullptr;
}

bool prosper::Shader::RecordBindDescriptorSets(ShaderBindState &bindState, const std::vector<IDescriptorSet *> &descSets, uint32_t firstSet, const std::vector<uint32_t> &dynamicOffsets) const
{
	return bindState.commandBuffer.RecordBindDescriptorSets(GetPipelineBindPoint(), const_cast<Shader &>(*this), bindState.pipelineIdx, firstSet, descSets, dynamicOffsets);
}

bool prosper::Shader::RecordBindDescriptorSet(ShaderBindState &bindState, IDescriptorSet &descSet, uint32_t firstSet, const std::vector<uint32_t> &dynamicOffsets) const
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
	auto *ptrDescSet = &descSet;
	return bindState.commandBuffer.RecordBindDescriptorSets(GetPipelineBindPoint(), const_cast<Shader &>(*this), bindState.pipelineIdx, firstSet, {ptrDescSet}, dynamicOffsets);
}

static std::unordered_map<prosper::ICommandBuffer *, std::pair<prosper::Shader *, uint32_t>> s_boundShaderPipeline = {};
static std::mutex s_boundShaderPipelineMutex;
prosper::Shader *prosper::Shader::GetBoundPipeline(ICommandBuffer &cmdBuffer, uint32_t &outPipelineIdx)
{
	std::scoped_lock lock {s_boundShaderPipelineMutex};
	auto it = s_boundShaderPipeline.find(&cmdBuffer);
	if(it == s_boundShaderPipeline.end())
		return nullptr;
	outPipelineIdx = it->second.second;
	return it->second.first;
}
bool prosper::Shader::RecordBindPipeline(ShaderBindState &bindState, uint32_t pipelineIdx) const
{
	auto *pipeline = GetPipelineInfo(pipelineIdx);
	if(!pipeline)
		return false;
	bindState.pipelineIdx = pipelineIdx;
	return bindState.commandBuffer.RecordBindShaderPipeline(const_cast<Shader &>(*this), bindState.pipelineIdx);
}
void prosper::Shader::BakePipeline(uint32_t pipelineIdx) const
{
	PipelineID pipelineId;
	if(!GetPipelineId(pipelineId, pipelineIdx))
		return;
	GetContext().BakeShaderPipeline(pipelineId, GetPipelineBindPoint());
}
void prosper::Shader::FlushLoad() const
{
	if(!m_loading)
		return;
	m_loading = false;
	GetContext().GetPipelineLoader().Flush();
}
void prosper::Shader::BakePipelines() const
{
	FlushLoad();
	auto n = m_pipelineInfos.size();
	for(auto i = decltype(n) {0u}; i < n; ++i)
		BakePipeline(i);
}

prosper::Shader *prosper::find_shader(const IPrContext &context, const std::type_info &typeInfo)
{
	auto &shaderManager = context.GetShaderManager();
	return shaderManager.FindShader(typeInfo);
}

std::unique_ptr<prosper::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::ToProsperDescriptorSetInfo() const
{
	auto dsInfo = DescriptorSetCreateInfo::Create(GetName());
	for(auto &binding : bindings) {
		dsInfo->AddBinding(binding.name, binding.bindingIndex, binding.type, binding.descriptorArraySize, binding.shaderStages, DescriptorBindingFlags::None, binding.flags);
	}
	return dsInfo;
}
std::unique_ptr<prosper::DescriptorSetCreateInfo> prosper::DescriptorSetInfo::Bake()
{
	if(m_bWasBaked == false) {
		m_bWasBaked = true;
		auto nextIdx = 0u;
		for(auto &binding : bindings) {
			auto &bindingIdx = binding.bindingIndex;
			if(bindingIdx == std::numeric_limits<uint32_t>::max())
				bindingIdx = nextIdx;
			++nextIdx;
			//nextIdx = bindingIdx +binding.descriptorArraySize;
		}
	}
	return ToProsperDescriptorSetInfo();
}

prosper::ShaderModule::ShaderModule(const std::shared_ptr<ShaderStageProgram> &shaderStageProgram, const std::string &in_opt_cs_entrypoint_name, const std::string &in_opt_fs_entrypoint_name, const std::string &in_opt_gs_entrypoint_name, const std::string &in_opt_tc_entrypoint_name,
  const std::string &in_opt_te_entrypoint_name, const std::string &in_opt_vs_entrypoint_name)
    : m_csEntrypointName(in_opt_cs_entrypoint_name), m_fsEntrypointName(in_opt_fs_entrypoint_name), m_gsEntrypointName(in_opt_gs_entrypoint_name), m_tcEntrypointName(in_opt_tc_entrypoint_name), m_teEntrypointName(in_opt_te_entrypoint_name), m_vsEntrypointName(in_opt_vs_entrypoint_name),
      m_shaderStageProgram {shaderStageProgram}
{
}

prosper::PipelineID prosper::Shader::InitPipelineId(uint32_t pipelineIdx)
{
	if(pipelineIdx < m_cachedPipelineIds.size() && m_cachedPipelineIds[pipelineIdx] != std::numeric_limits<PipelineID>::max())
		return m_cachedPipelineIds[pipelineIdx];
	if(pipelineIdx >= m_cachedPipelineIds.size())
		m_cachedPipelineIds.resize(pipelineIdx, std::numeric_limits<PipelineID>::max());
	m_cachedPipelineIds[pipelineIdx] = GetContext().ReserveShaderPipeline();
	return m_cachedPipelineIds[pipelineIdx];
}

uint32_t prosper::Shader::AddDescriptorSetGroup(DescriptorSetInfo &descSetInfo)
{
	if(descSetInfo.parent != nullptr) {
		auto &parent = *descSetInfo.parent;
		descSetInfo.parent = nullptr;
		auto childBindings = descSetInfo.bindings;
		descSetInfo.bindings = parent.bindings;
		descSetInfo.m_name = parent.m_name;

		descSetInfo.bindings.reserve(descSetInfo.bindings.size() + childBindings.size());
		for(auto &childBinding : childBindings)
			descSetInfo.bindings.push_back(childBinding);
	}
	m_shaderResources.descSetInfos.emplace_back(descSetInfo.Bake());
	descSetInfo.setIndex = m_shaderResources.descSetInfos.size() - 1u;
	if(m_shaderResources.descSetInfos.size() > 8)
		throw std::logic_error("Attempted to add more than 8 descriptor sets to shader. This exceeds the limit on some GPUs!");
	return descSetInfo.setIndex;
}

std::optional<uint32_t> prosper::Shader::FindDescriptorSetIndex(const std::string &name) const
{
	auto it = std::find_if(m_shaderResources.descSetInfos.begin(), m_shaderResources.descSetInfos.end(), [&name](const std::shared_ptr<DescriptorSetCreateInfo> &dsInfo) { return pragma::string::compare(name.c_str(), dsInfo->GetName(), false); });
	return (it != m_shaderResources.descSetInfos.end()) ? (it - m_shaderResources.descSetInfos.begin()) : std::optional<uint32_t> {};
}

pragma::util::WeakHandle<const prosper::Shader> prosper::Shader::GetHandle() const { return pragma::util::WeakHandle<const Shader>(shared_from_this()); }
pragma::util::WeakHandle<prosper::Shader> prosper::Shader::GetHandle() { return pragma::util::WeakHandle<Shader>(shared_from_this()); }

bool prosper::Shader::AttachPushConstantRange(uint32_t offset, uint32_t size, ShaderStageFlags stages)
{
	for(auto &range : m_shaderResources.pushConstantRanges) {
		if(range.stages != stages)
			continue;
		if((range.offset + range.size) == offset) {
			range.size += size;
			break;
		}
		else if((offset + size) == range.offset) {
			range.offset = offset;
			break;
		}
	}
	m_shaderResources.pushConstantRanges.push_back({offset, size, stages});
	return true; // pipelineInfo.AttachPushConstantRange(offset,size,stages);
}

///////////////////////////

void prosper::util::set_graphics_pipeline_polygon_mode(GraphicsPipelineCreateInfo &pipelineInfo, PolygonMode polygonMode)
{
	CullModeFlags cullModeFlags;
	FrontFace frontFace;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(nullptr, &cullModeFlags, &frontFace, &lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode, cullModeFlags, frontFace, lineWidth);
}
void prosper::util::set_graphics_pipeline_line_width(GraphicsPipelineCreateInfo &pipelineInfo, float lineWidth)
{
	CullModeFlags cullModeFlags;
	FrontFace frontFace;
	PolygonMode polygonMode;
	pipelineInfo.GetRasterizationProperties(&polygonMode, &cullModeFlags, &frontFace, nullptr);
	pipelineInfo.SetRasterizationProperties(polygonMode, cullModeFlags, frontFace, lineWidth);
}
void prosper::util::set_graphics_pipeline_cull_mode_flags(GraphicsPipelineCreateInfo &pipelineInfo, CullModeFlags cullModeFlags)
{
	FrontFace frontFace;
	PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(&polygonMode, nullptr, &frontFace, &lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode, cullModeFlags, frontFace, lineWidth);
}
void prosper::util::set_graphics_pipeline_front_face(GraphicsPipelineCreateInfo &pipelineInfo, FrontFace frontFace)
{
	CullModeFlags cullModeFlags;
	PolygonMode polygonMode;
	float lineWidth;
	pipelineInfo.GetRasterizationProperties(&polygonMode, &cullModeFlags, nullptr, &lineWidth);
	pipelineInfo.SetRasterizationProperties(polygonMode, cullModeFlags, frontFace, lineWidth);
}
void prosper::util::set_generic_alpha_color_blend_attachment_properties(GraphicsPipelineCreateInfo &pipelineInfo)
{
	pipelineInfo.SetColorBlendAttachmentProperties(0u, true, BlendOp::Add, BlendOp::Add, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha,
	  ColorComponentFlags::RBit | ColorComponentFlags::GBit | ColorComponentFlags::BBit | ColorComponentFlags::ABit);
}
static prosper::DynamicState get_anvil_dynamic_state(prosper::util::DynamicStateFlags state)
{
	switch(state) {
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
	return prosper::DynamicState {};
}
static prosper::util::DynamicStateFlags get_prosper_dynamic_state(prosper::DynamicState state)
{
	switch(state) {
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
	auto values = pragma::math::get_power_of_2_values(pragma::math::to_integral(states));
	std::vector<prosper::DynamicState> r {};
	r.reserve(values.size());
	for(auto &v : values)
		r.push_back(get_anvil_dynamic_state(static_cast<prosper::util::DynamicStateFlags>(v)));
	return r;
}
void prosper::util::set_dynamic_states_enabled(GraphicsPipelineCreateInfo &pipelineInfo, DynamicStateFlags states, bool enabled) { pipelineInfo.ToggleDynamicStates(enabled, ::get_enabled_dynamic_states(states)); }
bool prosper::util::are_dynamic_states_enabled(GraphicsPipelineCreateInfo &pipelineInfo, DynamicStateFlags states) { return (states & get_enabled_dynamic_states(pipelineInfo)) == states; }
prosper::util::DynamicStateFlags prosper::util::get_enabled_dynamic_states(GraphicsPipelineCreateInfo &pipelineInfo)
{
	const DynamicState *states;
	auto numStates = 0u;
	pipelineInfo.GetEnabledDynamicStates(&states, &numStates);

	auto r = DynamicStateFlags::None;
	for(auto i = decltype(numStates) {0u}; i < numStates; ++i)
		r |= get_prosper_dynamic_state(static_cast<DynamicState>(states[i]));
	return r;
}
