/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define VK_ENABLE_GLSLANG

#ifdef VK_ENABLE_GLSLANG
#include "prosper_includes.hpp"
#include "prosper_glsl.hpp"
#include "prosper_context.hpp"
#include "shader/prosper_shader.hpp"
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util_path.hpp>
#include <unordered_set>
#include <sstream>
#include <cassert>
#include <mpParser.h>

static unsigned int get_line_break(std::string &str, int pos = 0)
{
	auto len = str.length();
	while(pos < len && str[pos] != '\n')
		pos++;
	return pos;
}

static void print_available_shader_descriptor_set_bindings(const std::unordered_map<std::string, std::string> &definitions, std::stringstream &ss)
{
	std::unordered_map<std::string, std::vector<std::string>> descSetInfos;
	for(auto &[id, val] : definitions) {
		const char *descSetPrefix = "DESCRIPTOR_SET_";
		if(!ustring::compare(id.c_str(), descSetPrefix, true, strlen(descSetPrefix)))
			continue;
		auto sub = id.substr(strlen(descSetPrefix));
		std::string dsName;
		std::optional<std::string> bindingName {};
		const char *bindingPrefix = "_BINDING_";
		auto posBinding = sub.find(bindingPrefix);
		if(posBinding != std::string::npos) {
			dsName = sub.substr(0, posBinding);
			bindingName = sub.substr(posBinding + strlen(bindingPrefix));
		}
		else
			dsName = sub;
		auto it = descSetInfos.find(dsName);
		if(it == descSetInfos.end())
			it = descSetInfos.insert(std::make_pair(dsName, std::vector<std::string> {})).first;
		if(bindingName)
			it->second.push_back(*bindingName);
	}

	ss << "Available descriptor set bindings for this shader:\r\n";
	for(auto &[descSet, bindings] : descSetInfos) {
		ss << std::left << std::setw(20) << descSet << ": ";
		auto first = true;
		for(auto &binding : bindings) {
			if(first)
				first = false;
			else
				ss << ", ";
			ss << binding;
		}
		ss << "\r\n";
	}
}

static bool translate_layout_ids(std::string &shader, const std::unordered_map<std::string, std::string> &definitions, std::string &outErr)
{
	const char *LAYOUT_ID = "LAYOUT_ID(";
	auto startPos = shader.find("#line 1");
	if(startPos == std::string::npos)
		startPos = 0;
	auto pos = shader.find(LAYOUT_ID, startPos);
	while(pos != std::string::npos) {
		pos += strlen(LAYOUT_ID);
		auto endPos = shader.find(')', pos);
		auto innerContents = shader.substr(pos, endPos - pos);
		std::vector<std::string> args;
		ustring::explode(innerContents, ",", args);
		if(args.size() == 2 && !ustring::is_integer(args[0])) {
			auto dsId = args[0];
			dsId = "DESCRIPTOR_SET_" + dsId;

			auto bindingId = args[1];
			uint32_t bindingIndex = 0;
			auto resBindingId = dsId + "_RESOURCE_BINDING_" + bindingId;
			bindingId = dsId + "_BINDING_" + bindingId;

			innerContents = dsId + "," + bindingId + "," + resBindingId;
			shader = shader.substr(0, pos) + innerContents + shader.substr(endPos);
			endPos = pos + innerContents.size();
		}
		pos = shader.find(LAYOUT_ID, endPos);
	}
	return true;
}

// Including shader files can be expensive, so we cache all included files for the future
static std::unordered_map<std::string, std::string> g_globalIncludeFileCache;
static std::mutex g_globalIncludeFileCacheMutex;

static bool glsl_preprocessing(const std::string &path, std::string &shader, const std::unordered_map<std::string, std::string> &definitions, std::string &outErr, std::unordered_set<std::string> &includeCache, std::vector<prosper::glsl::IncludeLine> &includeLines, unsigned int &lineId,
  std::stack<std::string> &includeStack, bool = false)
{
	if(!translate_layout_ids(shader, definitions, outErr))
		return false;

	auto depth = includeStack.size();
	if(!includeLines.empty() && includeLines.back().lineId == lineId)
		includeLines.back() = prosper::glsl::IncludeLine(lineId, path, depth);
	else
		includeLines.push_back(prosper::glsl::IncludeLine(lineId, path, depth));
	includeLines.back().includeStack = includeStack;
	//auto incIdx = includeLines.size() -1;
	std::string sub = FileManager::GetPath(const_cast<std::string &>(path));
	auto len = shader.length();
	unsigned int br;
	unsigned int brLast = 0;
	do {
		//std::cout<<"Length: "<<len<<"("<<shader.length()<<")"<<std::endl;
		br = get_line_break(shader, brLast);
		std::string l = shader.substr(brLast, br - brLast);
		ustring::remove_whitespace(l);
		if(l.length() >= 8) {
			if(l.substr(0, 8) == "#include") {
				std::string inc = l.substr(8, l.length());
				ustring::remove_whitespace(inc);
				ustring::remove_quotes(inc);
				util::Path includePath {};
				if(inc.empty() == false && inc.front() == '/')
					includePath = inc;
				else
					includePath = sub.substr(sub.find_first_of("/\\") + 1) + inc;
				includePath = util::Path::CreatePath(prosper::Shader::GetRootShaderLocation()) + includePath;
				auto ext = ufile::get_file_extension(includePath.GetString(), prosper::glsl::get_glsl_file_extensions());
				if(!ext) {
					if(includePath.GetString().substr(includePath.GetString().length() - 5) != ".glsl")
						includePath += ".glsl";
				}
				auto &strPath = includePath.GetString();
				constexpr auto useIncludeCache = false;
				if(!useIncludeCache || includeCache.find(strPath) == includeCache.end()) {
					includeCache.insert(strPath);

					g_globalIncludeFileCacheMutex.lock();
					auto it = g_globalIncludeFileCache.find(strPath);
					auto isInCache = it != g_globalIncludeFileCache.end();
					g_globalIncludeFileCacheMutex.unlock();
					if(isInCache) {
						shader = shader.substr(0, brLast) + it->second + shader.substr(br);
						br = brLast + it->second.length();
						len = shader.length();
						continue;
					}
					auto f = filemanager::open_file(includePath.GetString(), filemanager::FileMode::Read | filemanager::FileMode::Binary);
					if(f != NULL) {
						unsigned long long flen = f->GetSize();
						std::vector<char> data(flen + 1);
						f->Read(data.data(), flen);
						data[flen] = '\0';
						std::string subShader = data.data();
						lineId++; // #include-directive
						includeStack.push(includePath.GetString());
						if(glsl_preprocessing(includePath.GetString(), subShader, definitions, outErr, includeCache, includeLines, lineId, includeStack) == false)
							return false;
						includeLines.push_back(prosper::glsl::IncludeLine(lineId, path, depth, true));
						includeLines.back().includeStack = includeStack;
						includeStack.pop();
						lineId--; // Why?
						          //subShader = std::string("\n#line 1\n") +subShader +std::string("\n#line ") +std::to_string(lineId) +std::string("\n");
						g_globalIncludeFileCacheMutex.lock();
						g_globalIncludeFileCache[strPath] = subShader;
						g_globalIncludeFileCacheMutex.unlock();
						shader = shader.substr(0, brLast) + std::string("//") + l + std::string("\n") + subShader + shader.substr(br);
						br = CUInt32(brLast) + CUInt32(l.length()) + 3 + CUInt32(subShader.length());
					}
					else {
						outErr = "Unable to include file '" + includePath.GetString() + "' (In: '" + path + "'): File not found!";
						shader = shader.substr(0, brLast) + shader.substr(br);
						br = brLast;
						return false;
					}
				}
				else {
					shader = shader.substr(0, brLast) + shader.substr(br);
					br = brLast;
				}
				len = shader.length();
			}
		}
		lineId++;
		brLast = br + 1;
	} while(br < len);
	return true;
}

static bool glsl_preprocessing(prosper::IPrContext &context, prosper::ShaderStage stage, const std::string &path, std::string &shader, std::string &err, std::unordered_set<std::string> &includeCache, std::vector<prosper::glsl::IncludeLine> &includeLines, unsigned int &lineId,
  std::stack<std::string> &includeStack, const std::string &prefixCode = {}, std::unordered_map<std::string, std::string> definitions = {}, bool bHlsl = false)
{
	lineId = 0;
	auto r = glsl_preprocessing(path, shader, definitions, err, includeCache, includeLines, lineId, includeStack, true);
	if(r == false)
		return false;
	// Custom definitions
	prosper::glsl::Definitions glslDefinitions {};
	context.GetGLSLDefinitions(glslDefinitions);
	definitions["LAYOUT_ID(setIndex,bindingIndex,resourceBindingIndex)"] = glslDefinitions.layoutId;
	definitions["LAYOUT_PUSH_CONSTANTS()"] = glslDefinitions.layoutPushConstants;
	definitions["SH_VERTEX_INDEX"] = glslDefinitions.vertexIndex;
	definitions["SH_INSTANCE_INDEX"] = glslDefinitions.instanceIndex;
	definitions["API_COORD_TRANSFORM(T)"] = '(' + glslDefinitions.apiCoordTransform + ')';
	definitions["API_SCREEN_SPACE_TRANSFORM(T)"] = '(' + glslDefinitions.apiScreenSpaceTransform + ')';
	definitions["API_DEPTH_TRANSFORM(T)"] = '(' + glslDefinitions.apiDepthTransform + ')';
	//definitions["__MAX_VERTEX_TEXTURE_IMAGE_UNITS__"] = std::to_string(maxTextureUnits);
	std::string prefix = (bHlsl == false) ? "GLS_" : "HLS_";
	switch(stage) {
	case prosper::ShaderStage::Fragment:
		definitions[prefix + "FRAGMENT_SHADER"] = "1";
		break;
	case prosper::ShaderStage::Vertex:
		definitions[prefix + "VERTEX_SHADER"] = "1";
		break;
	case prosper::ShaderStage::Geometry:
		definitions[prefix + "GEOMETRY_SHADER"] = "1";
		break;
	case prosper::ShaderStage::Compute:
		definitions[prefix + "COMPUTE_SHADER"] = "1";
		break;
	}

	auto vendor = context.GetPhysicalDeviceVendor();
	switch(vendor) {
	case prosper::Vendor::AMD:
		definitions[prefix + "VENDOR_AMD"] = "1";
		break;
	case prosper::Vendor::Nvidia:
		definitions[prefix + "VENDOR_NVIDIA"] = "1";
		break;
	case prosper::Vendor::Intel:
		definitions[prefix + "VENDOR_INTEL"] = "1";
		break;
	}

	lineId = static_cast<unsigned int>(definitions.size() + 1);
	std::string def;
	for(auto &[key, val] : definitions)
		def += "#define " + key + " " + val + "\n";

	// See http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
	static const std::string STANDARD_PREFIX_CODE = "const float GAMMA = 2.2;\n"
	                                                "const float INV_GAMMA = 1.0 / GAMMA;\n"
	                                                "vec3 linear_to_srgb(vec3 color) { return pow(color, vec3(INV_GAMMA)); }\n"
	                                                "vec3 srgb_to_linear(vec3 srgbIn) { return pow(srgbIn.xyz, vec3(GAMMA)); }\n";

	// This is additional code that should be inserted to the top of the glsl file.
	auto prefixCodeTranslated = STANDARD_PREFIX_CODE + prefixCode;
	includeCache.clear();
	if(!glsl_preprocessing("", prefixCodeTranslated, definitions, err, includeCache, includeLines, lineId, includeStack, true))
		return false;
	def += prefixCodeTranslated;
	def += "\n#line 1\n";

	// Can't add anything before #version, so look for it first
	size_t posAppend = 0;
	size_t posStart = shader.find("#version");
	auto posExt = shader.find("#extension", posStart);
	while(posExt != std::string::npos) {
		posStart = posExt;
		posExt = shader.find("#extension", posStart + 1);
	}
	if(posStart != std::string::npos) {
		size_t posNl = shader.find("\n", posStart);
		if(posNl != std::string::npos)
			posAppend = posNl;
		else
			posAppend = shader.length();
		lineId++;
	}
	shader = shader.substr(0, posAppend) + "\n" + def + shader.substr(posAppend + 1, shader.length());
	return context.ApplyGLSLPostProcessing(shader, err);
}

static size_t parse_definition(const std::string &glslShader, std::unordered_map<std::string, std::string> &outDefinitions, size_t startPos = 0)
{
	auto pos = glslShader.find("#define ", startPos);
	if(pos == std::string::npos)
		return pos;
	pos += 8;
	auto nameStart = glslShader.find_first_not_of(" ", pos);
	auto nameEnd = glslShader.find(' ', nameStart);
	if(nameEnd == std::string::npos)
		return nameEnd;
	auto name = glslShader.substr(nameStart, nameEnd - nameStart);
	auto valStart = glslShader.find_first_not_of(" ", nameEnd);
	auto valEnd = glslShader.find_first_of("\r\n", valStart);
	if(valEnd == std::string::npos)
		return valEnd;
	auto val = glslShader.substr(valStart, valEnd - valStart);
	ustring::remove_whitespace(name);
	ustring::remove_whitespace(val);
	auto it = outDefinitions.find(name);
	if(it == outDefinitions.end())
		outDefinitions.insert(std::make_pair(name, val));
	return valEnd;
}

static void parse_definitions(const std::string &glslShader, std::unordered_map<std::string, std::string> &outDefinitions)
{
	auto nextPos = parse_definition(glslShader, outDefinitions);
	while(nextPos != std::string::npos)
		nextPos = parse_definition(glslShader, outDefinitions, nextPos);
}

static void parse_definitions(const std::string &glslShader, std::unordered_map<std::string, int32_t> &outDefinitions)
{
	std::unordered_map<std::string, std::string> definitions {};
	parse_definitions(glslShader, definitions);

	struct Expression {
		std::string expression;
		mup::ParserX parser {};
		std::optional<int> value {};

		std::vector<std::string> variables {};
	};
	std::unordered_map<std::string, Expression> expressions {};
	expressions.reserve(definitions.size());
	for(auto &pair : definitions) {
		Expression expr {};
		expr.expression = pair.second;
		expr.parser.SetExpr(pair.second);
		try {
			for(auto &a : expr.parser.GetExprVar())
				expr.variables.push_back(a.first);
		}
		catch(const mup::ParserError &err) {
			continue;
		}
		expressions.insert(std::make_pair(pair.first, expr));
	}
	std::function<bool(Expression &)> fParseExpression = nullptr;
	fParseExpression = [&fParseExpression, &expressions](Expression &expr) {
		for(auto &var : expr.variables) {
			auto it = expressions.find(var);
			if(it == expressions.end())
				return false;
			if(it->second.value.has_value() == false && fParseExpression(it->second) == false)
				return false;
			if(expr.parser.IsConstDefined(var))
				continue;
			try {
				expr.parser.DefineConst(var, mup::int_type {*it->second.value});
			}
			catch(const mup::ParserError &err) {
				return false;
			}
		}
		try {
			expr.parser.SetExpr(expr.expression);
			auto &val = expr.parser.Eval();
			expr.value = val.GetFloat();
		}
		catch(const mup::ParserError &err) {
			return false;
		}
		return true;
	};
	for(auto &pair : expressions) {
		auto &expr = pair.second;
		fParseExpression(expr);
		if(pair.second.value.has_value() == false)
			continue;
		outDefinitions.insert(std::make_pair(pair.first, *pair.second.value));
	}
}

static size_t parse_layout_binding(const std::string &glslShader, const std::unordered_map<std::string, int32_t> &definitions, std::vector<std::optional<prosper::IPrContext::ShaderDescriptorSetInfo>> &outDescSetInfos,
  std::vector<prosper::IPrContext::ShaderMacroLocation> &outMacroLocations, size_t startPos)
{
	static const std::unordered_set<std::string> g_imageTypes = {"sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler2DRect", "sampler1DArray", "sampler2DArray", "samplerCubeArray", "samplerBuffer", "sampler2DMS", "sampler2DMSArray", "sampler1DShadow", "sampler2DShadow",
	  "samplerCubeShadow", "sampler2DRectShadow", "sampler1DArrayShadow", "sampler2DArrayShadow", "samplerCubeArrayShadow", "image1D", "image2D", "image3D", "imageCube", "image2DRect", "image1DArray", "image2DArray", "imageCubeArray", "imageBuffer", "image2DMS", "image2DMSArray"};
	auto pos = glslShader.find("LAYOUT_ID(", startPos);
	if(pos == std::string::npos)
		return pos;
	auto macroPos = pos;
	pos += 10;
	auto end = glslShader.find(")", pos);
	if(end == std::string::npos)
		return end;
	auto macroEnd = end + 1;
	auto strArgs = glslShader.substr(pos, end - pos);
	std::vector<std::string> args {};
	ustring::explode(strArgs, ",", args);
	if(args.size() < 2)
		return end;
	auto &set = args.at(0);
	auto &binding = args.at(1);
	std::optional<uint32_t> setIndex {};
	std::optional<uint32_t> bindingIndex {};
	if(ustring::is_integer(set))
		setIndex = ustring::to_int(set);
	else {
		// Set is a definition
		auto defName = "DESCRIPTOR_SET_" + set;
		auto itSet = definitions.find(defName);
		if(itSet != definitions.end())
			setIndex = itSet->second;
	}
	if(ustring::is_integer(binding))
		bindingIndex = ustring::to_int(binding);
	else {
		// Binding is a definition
		auto defName = "DESCRIPTOR_SET_" + set + "_BINDING_" + binding;
		auto itBinding = definitions.find(defName);
		if(itBinding != definitions.end())
			bindingIndex = itBinding->second;
	}

	std::vector<std::string> uniformArgs {};
	std::string strType;
	if(setIndex.has_value() && bindingIndex.has_value()) {
		// Determine type
		auto defEndPos = umath::min(glslShader.find(';', macroEnd), glslShader.find('{', macroEnd));
		auto type = prosper::IPrContext::ShaderDescriptorSetBindingInfo::Type::Buffer;
		uint32_t numBindings = 1;
		if(defEndPos != std::string::npos) {
			auto line = glslShader.substr(macroEnd, defEndPos - macroEnd);
			auto argStartPos = line.find_last_of(')');
			line = line.substr(argStartPos + 1);
			ustring::explode_whitespace(line, uniformArgs);
			if(uniformArgs.size() > 1) {
				strType = uniformArgs.at(1);
				auto itType = g_imageTypes.find(strType);
				if(itType != g_imageTypes.end())
					type = prosper::IPrContext::ShaderDescriptorSetBindingInfo::Type::Image;

				if(type == prosper::IPrContext::ShaderDescriptorSetBindingInfo::Type::Image) // TODO: We also need this for buffers
				{
					auto &arg = uniformArgs.back();
					auto brStart = arg.find('[');
					auto brEnd = arg.find(']', brStart);
					if(brEnd != std::string::npos) {
						++brStart;
						// It's an array
						auto strNumBindings = arg.substr(brStart, brEnd - brStart);
						if(ustring::is_integer(strNumBindings))
							numBindings = ustring::to_int(strNumBindings);
						else {
							// Binding count is a definition
							auto itBinding = definitions.find(strNumBindings);
							if(itBinding != definitions.end())
								numBindings = itBinding->second;
						}
					}
				}
			}
		}
		//

		if(*setIndex >= outDescSetInfos.size())
			outDescSetInfos.resize(*setIndex + 1);
		auto &setInfo = outDescSetInfos.at(*setIndex);
		if(setInfo.has_value() == false)
			setInfo = prosper::IPrContext::ShaderDescriptorSetInfo {};
		if(*bindingIndex >= setInfo->bindingPoints.size())
			setInfo->bindingPoints.resize(*bindingIndex + 1);
		setInfo->bindingPoints.at(*bindingIndex).type = type;
		setInfo->bindingPoints.at(*bindingIndex).namedType = strType;
		setInfo->bindingPoints.at(*bindingIndex).bindingCount = numBindings;
		setInfo->bindingPoints.at(*bindingIndex).args = std::move(uniformArgs);

		outMacroLocations.push_back({});
		auto &macroLocation = outMacroLocations.back();
		macroLocation.macroStart = macroPos;
		macroLocation.macroEnd = macroEnd;
		macroLocation.setIndexIndex = *setIndex;
		macroLocation.bindingIndexIndex = *bindingIndex;
		// setInfo->bindingPoints.at(*bindingIndex) = true;
	}
	return end;
}

void prosper::IPrContext::ParseShaderUniforms(const std::string &glslShader, std::unordered_map<std::string, int32_t> &definitions, std::vector<std::optional<ShaderDescriptorSetInfo>> &outDescSetInfos, std::vector<ShaderMacroLocation> &outMacroLocations)
{
	parse_definitions(glslShader, definitions);

	auto nextPos = parse_layout_binding(glslShader, definitions, outDescSetInfos, outMacroLocations, 0u);
	while(nextPos != std::string::npos)
		nextPos = parse_layout_binding(glslShader, definitions, outDescSetInfos, outMacroLocations, nextPos);
}

void prosper::glsl::translate_error(const std::string &shaderCode, const std::string &errorMsg, const std::string &pathMainShader, const std::vector<prosper::glsl::IncludeLine> &includeLines, Int32 lineOffset, std::string *err)
{
	std::vector<std::string> errorLines;
	ustring::explode(errorMsg, "\n", errorLines);
	auto numLines = errorLines.size();
	for(UInt i = 0; i < numLines; i++) {
		auto &msg = errorLines[i];
		ustring::remove_whitespace(msg);
		if(!msg.empty()) {
			auto bAMD = false;
			auto bWarning = false;
			if(msg.substr(0, 9) == "ERROR: 0:")
				bAMD = true;
			else if(msg.substr(0, 11) == "WARNING: 0:") {
				bAMD = true;
				bWarning = true;
			}
			auto bNvidia = (!bAMD && msg.substr(0, 2) == "0(") ? true : false;
			if(bAMD == true || bNvidia == true) {
				auto st = (bAMD == true) ? ((bWarning == true) ? 11 : 9) : 2;
				auto end = (bAMD == true) ? msg.find_first_of(':', st) : msg.find_first_of(')', st);
				if(end != ustring::NOT_FOUND) {
					auto line = atoi(msg.substr(st, end - st).c_str());
					auto lineInternal = line;
					std::string file;
					std::stack<std::string> includeStack;
					auto bFound = false;
					auto numIncludeLines = includeLines.size();
					for(UInt idx = 0; idx < numIncludeLines; idx++) {
						auto it = includeLines.begin() + idx;
						if(CUInt32(line) >= it->lineId && idx < CUInt32(numIncludeLines - 1))
							continue;
						if(it != includeLines.begin()) {
							if(idx < CUInt32(numIncludeLines - 1) || CUInt32(line) < it->lineId) {
								it--;
								idx--;
							}
							auto errLine = line - it->lineId + 1;
							for(auto idx2 = idx; idx2 > 0;) {
								idx2--;
								auto it2 = includeLines.begin() + idx2;
								if(it2->line == it->line && it2->depth == it->depth) {
									errLine += includeLines[idx2 + 1].lineId - it2->lineId;
									if(it2->ret == false)
										break;
								}
							}
							line = errLine;
							file = it->line;
							includeStack = it->includeStack;
							bFound = true;
						}
						break;
					}
					if(bFound == false)
						file = pathMainShader;
					file = FileManager::GetCanonicalizedPath(file);
					std::stringstream msgOut;
					msgOut << msg.substr(0, st) << std::to_string(line) << ((bAMD == true) ? (std::string(": '") + file + std::string("':")) : (std::string(")('") + file + std::string("')"))) << msg.substr(end + 1);
					lineInternal = line;

					auto contents = filemanager::read_file(file);
					if(contents) {
						std::vector<std::string> codeLines;
						ustring::explode(*contents, "\n", codeLines);
						if(lineInternal <= codeLines.size()) {
							auto lineInternalId = lineInternal - 1;
							for(Int32 iline = umath::max(lineInternalId - 2, 0); iline <= umath::min(lineInternalId + 2, static_cast<Int32>(codeLines.size() - 1)); iline++) {
								if(iline == lineInternalId)
									msgOut << "\n  > " << codeLines[iline];
								else
									msgOut << "\n    " << codeLines[iline];
							}
						}
						else
							msgOut << "\nUnable to display shader contents: Erroneous line " << lineInternal << " exceeds number of lines in file '" << file << "' (" << codeLines.size() << ")!";

						if(!includeStack.empty()) {
							msgOut << "\nInclude Stack:\n";
							std::vector<std::string> list;
							list.reserve(includeStack.size());
							while(!includeStack.empty()) {
								list.push_back(includeStack.top());
								includeStack.pop();
							}
							std::string prefix = "  ";
							for(auto it = list.rbegin(); it != list.rend(); ++it) {
								auto &item = *it;
								msgOut << prefix << item << "\n";
								prefix += "  ";
							}
						}
					}
					else
						msgOut << "\nUnable to display shader contents: Failed to open file '" << file << "' for reading!";
					msg = msgOut.str();
				}
			}
			*err += msg;
			if(i < (numLines - 1))
				*err += "\n";
		}
	}
}

void prosper::glsl::dump_parsed_shader(IPrContext &context, uint32_t stage, const std::string &shaderFile, const std::string &fileName)
{
	auto f = FileManager::OpenFile(shaderFile.c_str(), "r");
	if(f == nullptr)
		return;
	auto shaderCode = f->ReadString();
	std::vector<IncludeLine> includeLines;
	unsigned int lineOffset = 0;
	std::string err;
	std::stack<std::string> includeStack;
	std::unordered_set<std::string> includeCache;
	if(glsl_preprocessing(context, static_cast<prosper::ShaderStage>(stage), fileName, shaderCode, err, includeCache, includeLines, lineOffset, includeStack) == false)
		return;
	auto fOut = FileManager::OpenFile<VFilePtrReal>(fileName.c_str(), "w");
	if(fOut == nullptr)
		return;
	fOut->WriteString(shaderCode);
}

const std::vector<std::string> &prosper::glsl::get_glsl_file_extensions()
{
	static std::vector<std::string> exts {"comp", "frag", "geom", "tesc", "tese", "vert"};
	static_assert(umath::to_integral(prosper::ShaderStage::Count) == 6, "Update this list when new stages are added!");
	return exts;
}

bool prosper::glsl::is_glsl_file_extension(const std::string &ext)
{
	using namespace ustring::string_switch_ci;
	switch(hash(ext)) {
	case "comp"_:
	case "frag"_:
	case "geom"_:
	case "tesc"_:
	case "tese"_:
	case "vert"_:
		return true;
	default:
		return false;
	}
	static_assert(umath::to_integral(prosper::ShaderStage::Count) == 6, "Update this list when new stages are added!");
	return false;
}

std::string prosper::glsl::get_shader_file_extension(prosper::ShaderStage stage)
{
	switch(stage) {
	case prosper::ShaderStage::Compute:
		return "comp";
	case prosper::ShaderStage::Fragment:
		return "frag";
	case prosper::ShaderStage::Geometry:
		return "geom";
	case prosper::ShaderStage::TessellationControl:
		return "tesc";
	case prosper::ShaderStage::TessellationEvaluation:
		return "tese";
	case prosper::ShaderStage::Vertex:
		return "vert";
	}
	static_assert(umath::to_integral(prosper::ShaderStage::Count) == 6, "Update this list when new stages are added!");
	return "glsl";
}

std::optional<std::string> prosper::glsl::find_shader_file(prosper::ShaderStage stage, const std::string &fileName, std::string *optOutExt)
{
	std::string ext;
	if(ufile::get_extension(fileName, &ext) == false) {
		ext = get_shader_file_extension(stage);
		auto fullFileName = fileName + '.' + ext;
		return filemanager::exists(fullFileName) ? fullFileName : std::optional<std::string> {};
	}
	else {
		if(optOutExt)
			*optOutExt = ext;
	}
	return filemanager::exists(fileName) ? fileName : std::optional<std::string> {};
}

std::optional<std::string> prosper::glsl::load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog)
{
	std::vector<IncludeLine> includeLines;
	uint32_t lineOffset;
	return load_glsl(context, stage, fileName, infoLog, debugInfoLog, includeLines, lineOffset);
}

std::optional<std::string> prosper::glsl::load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog, std::vector<IncludeLine> &outIncludeLines, uint32_t &outLineOffset, const std::string &prefixCode,
  const std::unordered_map<std::string, std::string> &definitions, bool applyPreprocessing)
{
	std::string ext;
	auto optFileName = find_shader_file(stage, "shaders/" + fileName, &ext);
	if(optFileName.has_value() == false) {
		if(infoLog)
			*infoLog = "File not found: " + fileName;
		return {};
	}
	auto f = FileManager::OpenFile(optFileName->c_str(), "r");
	if(f == nullptr) {
		if(infoLog != nullptr)
			*infoLog = std::string("Unable to open file '") + fileName + std::string("'!");
		return {};
	}
	auto hlsl = false;
	if(ustring::compare<std::string>(ext, "hls", false))
		hlsl = true;
	auto shaderCode = f->ReadString();
	unsigned int lineOffset = 0;
	std::string err;
	std::stack<std::string> includeStack;
	std::unordered_set<std::string> includeCache;
	if(applyPreprocessing && glsl_preprocessing(context, stage, *optFileName, shaderCode, err, includeCache, outIncludeLines, lineOffset, includeStack, prefixCode, definitions, hlsl) == false) {
		if(infoLog != nullptr) {
			*infoLog = std::string("Module: \"") + fileName + "\"\n" + err;

			if(!includeStack.empty()) {
				std::vector<std::string> includeList;
				includeList.reserve(includeStack.size());
				while(!includeStack.empty()) {
					includeList.push_back(std::move(includeStack.top()));
					includeStack.pop();
				}

				std::stringstream ssIncludeStack;
				ssIncludeStack << "Include Stack:\r\n";
				std::string t = "  ";
				for(auto it = includeList.rbegin(); it != includeList.rend(); ++it) {
					auto &path = *it;
					ssIncludeStack << t << path << "\r\n";
					t += "  ";
				}
				*infoLog += "\r\n" + ssIncludeStack.str();
			}
		}
		return {};
	}
	return shaderCode;
}

#endif
