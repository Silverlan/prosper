/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#include "shader/prosper_shader_manager.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"
#include "prosper_pipeline_cache.hpp"
#include <shaderinfo.h>
#include <sharedutils/util_string.h>
#include <iostream>

prosper::ShaderManager::ShaderManager(Context &context)
	: ContextObject(context)
{}
util::WeakHandle<ShaderInfo> prosper::ShaderManager::PreRegisterShader(const std::string &identifier)
{
	auto lidentifier = identifier;
	ustring::to_lower(lidentifier);
	auto it = m_shaderInfo.find(lidentifier);
	if(it != m_shaderInfo.end())
		return it->second;
	auto shaderInfo = std::make_shared<ShaderInfo>(lidentifier);
	it = m_shaderInfo.insert(std::make_pair(lidentifier,shaderInfo)).first;
	return it->second;
}

::util::WeakHandle<prosper::Shader> prosper::ShaderManager::RegisterShader(const std::string &identifier,const std::function<Shader*(Context&,const std::string&,bool&)> &fFactory)
{
	if(GetContext().IsValidationEnabled())
		std::cout<<"[VK] Registering shader '"<<identifier<<"'..."<<std::endl;
	auto wpShaderInfo = PreRegisterShader(identifier);
	auto lidentifier = identifier;
	ustring::to_lower(lidentifier);
	auto bExternalOwnership = false;
	auto *ptrShader = fFactory(GetContext(),lidentifier,bExternalOwnership);
	std::shared_ptr<Shader> shader = nullptr;
	if(bExternalOwnership == false)
		shader = std::shared_ptr<Shader>(ptrShader,[](Shader *shader) {shader->Release();});
	else
		shader = std::shared_ptr<Shader>(ptrShader,[](Shader *shader) {shader->Release(false);});
	m_shaders[lidentifier] = shader;
	auto wpShader = ::util::WeakHandle<Shader>(shader);
	wpShaderInfo.get()->SetShader(std::make_shared<::util::WeakHandle<Shader>>(wpShader));
	shader->Initialize();
	return wpShader;
}
util::WeakHandle<prosper::Shader> prosper::ShaderManager::RegisterShader(const std::string &identifier,const std::function<Shader*(Context&,const std::string&)> &fFactory)
{
	return RegisterShader(identifier,[fFactory](Context &context,const std::string &identifier,bool &bExternalOwnership) {
		bExternalOwnership = false;
		return fFactory(context,identifier);
	});
}
util::WeakHandle<prosper::Shader> prosper::ShaderManager::GetShader(const std::string &identifier) const
{
	auto lidentifier = identifier;
	auto it = m_shaders.find(lidentifier);
	return (it != m_shaders.end()) ? ::util::WeakHandle<Shader>(it->second) : ::util::WeakHandle<Shader>();
}
const std::unordered_map<std::string,std::shared_ptr<prosper::Shader>> &prosper::ShaderManager::GetShaders() const {return m_shaders;}
bool prosper::ShaderManager::RemoveShader(Shader &shader)
{
	auto it = std::find_if(m_shaders.begin(),m_shaders.end(),[&shader](const std::pair<std::string,std::shared_ptr<Shader>> &pair) {
		return pair.second.get() == &shader;
	});
	if(it == m_shaders.end())
		return false;
	m_shaders.erase(it);
	return true;
}
