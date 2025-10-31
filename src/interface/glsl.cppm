// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "prosper_definitions.hpp"

export module pragma.prosper:glsl;

export import std.compat;

export namespace prosper {
	enum class ShaderStage : uint8_t;
	class IPrContext;
	enum class Vendor : uint32_t;
	namespace glsl {
		struct DLLPROSPER IncludeLine {
			IncludeLine(uint32_t line, const std::string &l, uint32_t dp = 0, bool bRet = false) : lineId(line), line(l), ret(bRet), depth(dp) {}
			IncludeLine() : IncludeLine(0, "") {}
			uint32_t lineId;
			std::string line;
			bool ret;
			uint32_t depth;
			std::stack<std::string> includeStack;
		};
		DLLPROSPER void translate_error(const std::string &shaderCode, const std::string &errorMsg, const std::string &pathMainShader, const std::vector<IncludeLine> &includeLines, int32_t lineOffset, std::string *err);
		struct DLLPROSPER Definitions {
			std::string layoutPushConstants;
			std::string layoutId; // Arguments: set, binding
			std::string vertexIndex;
			std::string instanceIndex;
			std::string apiCoordTransform = "T";       // Arguments: T (mat4)
			std::string apiScreenSpaceTransform = "T"; // Arguments: T (mat4)
			std::string apiDepthTransform = "T";       // Arguments: T (mat4)
		};
		DLLPROSPER const std::vector<std::string> &get_glsl_file_extensions();
		DLLPROSPER std::string get_shader_file_extension(prosper::ShaderStage stage);
		DLLPROSPER bool is_glsl_file_extension(const std::string &ext);
		DLLPROSPER std::optional<std::string> find_shader_file(prosper::ShaderStage stage, const std::string &fileName, std::string *optOutExt = nullptr);
		DLLPROSPER void dump_parsed_shader(IPrContext &context, uint32_t stage, const std::string &shaderFile, const std::string &fileName);
		DLLPROSPER std::optional<std::string> load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog, std::vector<IncludeLine> &outIncludeLines, uint32_t &outLineOffset, const std::string &prefixCode = {},
		  const std::unordered_map<std::string, std::string> &definitions = {}, bool applyPreprocessing = true);
		DLLPROSPER std::optional<std::string> load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog);
	};
};
