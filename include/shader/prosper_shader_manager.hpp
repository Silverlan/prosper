#ifndef __PROSPER_SHADER_MANAGER_HPP__
#define __PROSPER_SHADER_MANAGER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_context_object.hpp"
#include <sharedutils/util_weak_handle.hpp>
#include <shader/prosper_shader.hpp>
#include <unordered_map>
#include <functional>
#include <string>

namespace util {class ShaderInfo;};
#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Shader;
	class PipelineCache;
	using ShaderIndex = uint32_t;
	class DLLPROSPER ShaderManager
		: public ContextObject
	{
	public:
		ShaderManager(IPrContext &context);
		~ShaderManager()=default;

		::util::WeakHandle<::util::ShaderInfo> PreRegisterShader(const std::string &identifier);
		void RegisterShader(const std::string &identifier,const std::function<Shader*(IPrContext&,const std::string&)> &fFactory);
		void RegisterShader(const std::string &identifier,const std::function<Shader*(IPrContext&,const std::string&,bool&)> &fFactory);
		::util::WeakHandle<Shader> GetShader(const std::string &identifier) const;
		Shader *GetShader(ShaderIndex index) const;
		const std::vector<std::shared_ptr<Shader>> &GetShaders() const;
		const std::unordered_map<std::string,ShaderIndex> &GetShaderNameToIndexTable() const {return m_shaderNameToIndex;}
		bool RemoveShader(Shader &shader);

		template<class T>
			const Shader *FindShader() const;
		template<class T>
			Shader *FindShader();

		ShaderManager(const ShaderManager&)=delete;
		ShaderManager &operator=(const ShaderManager&)=delete;
	private:
		::util::WeakHandle<Shader> LoadShader(const std::string &identifier);
		std::unordered_map<std::string,ShaderIndex> m_shaderNameToIndex;
		std::vector<std::shared_ptr<Shader>> m_shaders;

		std::unordered_map<std::string,std::function<Shader*(IPrContext&,const std::string&,bool&)>> m_shaderFactories {};

		// Pre-registered shaders
		std::unordered_map<std::string,std::shared_ptr<::util::ShaderInfo>> m_shaderInfo;
	};
};

template<class T>
	const prosper::Shader *prosper::ShaderManager::FindShader() const
{
	return const_cast<ShaderManager*>(this)->FindShader<T>();
}
template<class T>
	prosper::Shader *prosper::ShaderManager::FindShader()
{
	auto it = std::find_if(m_shaders.begin(),m_shaders.end(),[](const std::shared_ptr<Shader> &shader) {
		return typeid(T) == typeid(*shader);
	});
	return (it != m_shaders.end()) ? it->get() : nullptr;
}
#pragma warning(pop)

#endif
