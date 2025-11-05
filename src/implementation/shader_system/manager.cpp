// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :shader_system.manager;
import :shader_system.shader;

prosper::ShaderManager::ShaderManager(IPrContext &context) : ContextObject(context) {}
util::WeakHandle<::util::ShaderInfo> prosper::ShaderManager::PreRegisterShader(const std::string &identifier)
{
	auto lidentifier = identifier;
	ustring::to_lower(lidentifier);
	auto it = m_shaderInfo.find(lidentifier);
	if(it != m_shaderInfo.end())
		return it->second;
	auto shaderInfo = std::make_shared<::util::ShaderInfo>(lidentifier);
	it = m_shaderInfo.insert(std::make_pair(lidentifier, shaderInfo)).first;
	return it->second;
}

::util::WeakHandle<prosper::Shader> prosper::ShaderManager::LoadShader(const std::string &identifier)
{
	auto it = m_shaderNameToIndex.find(identifier);
	if(it != m_shaderNameToIndex.end())
		return m_shaders.at(it->second);
	auto itFactory = m_shaderFactories.find(identifier);
	if(itFactory == m_shaderFactories.end())
		return {};
	auto wpShaderInfo = PreRegisterShader(identifier);
	auto &factory = itFactory->second;
	auto bExternalOwnership = false;
	auto *ptrShader = factory(GetContext(), identifier, bExternalOwnership);
	ptrShader->SetIndex(m_shaders.size());
	std::shared_ptr<Shader> shader = nullptr;
	if(bExternalOwnership == false)
		shader = std::shared_ptr<Shader>(ptrShader, [](Shader *shader) { shader->Release(); });
	else
		shader = std::shared_ptr<Shader>(ptrShader, [](Shader *shader) { shader->Release(false); });
	if(m_shaders.size() == m_shaders.capacity())
		m_shaders.reserve(m_shaders.size() * 1.1 + 100);
	m_shaders.push_back(shader);
	m_shaderNameToIndex.insert(std::make_pair(identifier, m_shaders.size() - 1));
	auto wpShader = ::util::WeakHandle<Shader>(shader);
	wpShaderInfo.get()->SetShader(std::make_shared<::util::WeakHandle<Shader>>(wpShader));
	auto &context = GetContext();
	context.Log("Initializing shader '" + identifier + "'...");
	shader->Initialize();
	return wpShader;
}

void prosper::ShaderManager::RegisterShader(const std::string &identifier, const std::function<Shader *(IPrContext &, const std::string &, bool &)> &fFactory)
{
	if(GetContext().IsValidationEnabled())
		std::cout << "[PR] Registering shader '" << identifier << "'..." << std::endl;
	auto wpShaderInfo = PreRegisterShader(identifier);
	auto lidentifier = identifier;
	ustring::to_lower(lidentifier);

	// The shader will be initialized lazily (whenever it's accessed for the first time)
	m_shaderFactories[lidentifier] = fFactory;
}
void prosper::ShaderManager::RegisterShader(const std::string &identifier, const std::function<Shader *(IPrContext &, const std::string &)> &fFactory)
{
	RegisterShader(identifier, [fFactory](IPrContext &context, const std::string &identifier, bool &bExternalOwnership) {
		bExternalOwnership = false;
		return fFactory(context, identifier);
	});
	if(GetContext().IsMultiThreadedRenderingEnabled())
		LoadShader(identifier); // Immediately start loading the shader
}
prosper::Shader *prosper::ShaderManager::GetShader(ShaderIndex index) const { return (index < m_shaders.size()) ? m_shaders.at(index).get() : nullptr; }
util::WeakHandle<prosper::Shader> prosper::ShaderManager::GetShader(const std::string &identifier) const
{
	return const_cast<ShaderManager *>(this)->LoadShader(identifier);
	/*auto lidentifier = identifier;
	auto it = m_shaders.find(lidentifier);
	return (it != m_shaders.end()) ? ::util::WeakHandle<Shader>(it->second) : ::util::WeakHandle<Shader>();*/
}
const std::vector<std::shared_ptr<prosper::Shader>> &prosper::ShaderManager::GetShaders() const { return m_shaders; }
bool prosper::ShaderManager::RemoveShader(Shader &shader)
{
	auto it = std::find_if(m_shaders.begin(), m_shaders.end(), [&shader](const std::shared_ptr<Shader> &shaderOther) { return shaderOther.get() == &shader; });
	if(it == m_shaders.end())
		return false;
	m_shaders.at(it - m_shaders.begin()) = nullptr;
	auto itName = m_shaderNameToIndex.find(shader.GetIdentifier());
	if(itName != m_shaderNameToIndex.end())
		m_shaderNameToIndex.erase(itName);
	// m_shaders.erase(it);
	return true;
}

prosper::Shader *prosper::ShaderManager::FindShader(const std::type_info &typeInfo)
{
	auto it = std::find_if(m_shaders.begin(), m_shaders.end(), [&typeInfo](const std::shared_ptr<Shader> &shader) { return typeInfo == typeid(*shader); });
	return (it != m_shaders.end()) ? it->get() : nullptr;
}
