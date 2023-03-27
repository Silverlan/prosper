/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define VK_ENABLE_GLSLANG

#ifdef VK_ENABLE_GLSLANG
#include "prosper_includes.hpp"
#include "prosper_glsl.hpp"
#include "shader/prosper_shader.hpp"
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util_string.h>
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

static bool glsl_preprocessing(const std::string &path, std::string &shader, std::string *err, std::vector<prosper::glsl::IncludeLine> &includeLines, unsigned int &lineId, UInt32 depth = 0, bool = false)
{
	if(!includeLines.empty() && includeLines.back().lineId == lineId)
		includeLines.back() = prosper::glsl::IncludeLine(lineId, path, depth);
	else
		includeLines.push_back(prosper::glsl::IncludeLine(lineId, path, depth));
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
				std::string includePath = "";
				if(inc.empty() == false && inc.front() == '/')
					includePath = inc;
				else
					includePath = sub.substr(sub.find_first_of("/\\") + 1) + inc;
				includePath = prosper::Shader::GetRootShaderLocation() + "\\" + FileManager::GetCanonicalizedPath(includePath);
				if(includePath.substr(includePath.length() - 4) != ".gls")
					includePath += ".gls";
				auto f = FileManager::OpenFile(includePath.c_str(), "rb");
				if(f != NULL) {
					unsigned long long flen = f->GetSize();
					std::vector<char> data(flen + 1);
					f->Read(data.data(), flen);
					data[flen] = '\0';
					std::string subShader = data.data();
					lineId++; // #include-directive
					if(glsl_preprocessing(includePath, subShader, err, includeLines, lineId, depth + 1) == false)
						return false;
					includeLines.push_back(prosper::glsl::IncludeLine(lineId, path, depth, true));
					lineId--; // Why?
					          //subShader = std::string("\n#line 1\n") +subShader +std::string("\n#line ") +std::to_string(lineId) +std::string("\n");
					shader = shader.substr(0, brLast) + std::string("//") + l + std::string("\n") + subShader + shader.substr(br);
					br = CUInt32(brLast) + CUInt32(l.length()) + 3 + CUInt32(subShader.length());
				}
				else {
					if(err != NULL)
						*err = "Unable to include file '" + includePath + "' (In: '" + path + "'): File not found!";
					shader = shader.substr(0, brLast) + shader.substr(br);
					br = brLast;
					return false;
				}
				len = shader.length();
			}
		}
		lineId++;
		brLast = br + 1;
	} while(br < len);
	return true;
}

static bool glsl_preprocessing(prosper::IPrContext &context, prosper::ShaderStage stage, const std::string &path, std::string &shader, std::string &err, std::vector<prosper::glsl::IncludeLine> &includeLines, unsigned int &lineId,
  std::unordered_map<std::string, std::string> definitions = {}, bool bHlsl = false)
{
	lineId = 0;
	auto r = glsl_preprocessing(path, shader, &err, includeLines, lineId, 0, true);
	if(r == false)
		return false;
	// Custom definitions
	prosper::glsl::Definitions glslDefinitions {};
	context.GetGLSLDefinitions(glslDefinitions);
	definitions["LAYOUT_ID(setIndex,bindingIndex)"] = glslDefinitions.layoutId;
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
	std::unordered_map<std::string, std::string>::iterator itDef;
	for(itDef = definitions.begin(); itDef != definitions.end(); itDef++)
		def += "#define " + itDef->first + " " + itDef->second + "\n";
	def += "#line 1\n";

	// Can't add anything before #version, so look for it first
	size_t posAppend = 0;
	size_t posVersion = shader.find("#version");
	if(posVersion != std::string::npos) {
		size_t posNl = shader.find("\n", posVersion);
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
				expr.parser.DefineConst(var, *it->second.value);
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
		auto itSet = definitions.find(set);
		if(itSet != definitions.end())
			setIndex = itSet->second;
	}
	if(ustring::is_integer(binding))
		bindingIndex = ustring::to_int(binding);
	else {
		// Binding is a definition
		auto itBinding = definitions.find(binding);
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
	std::vector<std::string> codeLines;
	ustring::explode(shaderCode, "\n", codeLines);

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
							bFound = true;
						}
						break;
					}
					if(bFound == false)
						file = pathMainShader;
					file = FileManager::GetCanonicalizedPath(file);
					std::stringstream msgOut;
					msgOut << msg.substr(0, st) << std::to_string(line) << ((bAMD == true) ? (std::string(": '") + file + std::string("':")) : (std::string(")('") + file + std::string("')"))) << msg.substr(end + 1);
					lineInternal += lineOffset;
					if(lineInternal <= codeLines.size()) {
						auto lineInternalId = lineInternal - 1;
						for(Int32 iline = umath::max(lineInternalId - 2, 0); iline <= umath::min(lineInternalId + 2, static_cast<Int32>(codeLines.size() - 1)); iline++) {
							if(iline == lineInternalId)
								msgOut << "\n  > " << codeLines[iline];
							else
								msgOut << "\n    " << codeLines[iline];
						}
					}
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
	if(glsl_preprocessing(context, static_cast<prosper::ShaderStage>(stage), fileName, shaderCode, err, includeLines, lineOffset) == false)
		return;
	auto fOut = FileManager::OpenFile<VFilePtrReal>(fileName.c_str(), "w");
	if(fOut == nullptr)
		return;
	fOut->WriteString(shaderCode);
}

std::optional<std::string> prosper::glsl::find_shader_file(const std::string &fileName, std::string *optOutExt)
{
	std::string ext;
	if(ufile::get_extension(fileName, &ext) == false) {
		auto fileNameGls = fileName + ".gls";
		if(FileManager::Exists(fileNameGls))
			return fileNameGls;
		auto fileNameHls = fileName + ".hls";
		if(FileManager::Exists(fileNameHls))
			return fileNameHls;
		return {};
	}
	else {
		if(optOutExt)
			*optOutExt = ext;
	}
	return FileManager::Exists(fileName) ? fileName : std::optional<std::string> {};
}

std::optional<std::string> prosper::glsl::load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog)
{
	std::vector<IncludeLine> includeLines;
	uint32_t lineOffset;
	return load_glsl(context, stage, fileName, infoLog, debugInfoLog, includeLines, lineOffset);
}

std::optional<std::string> prosper::glsl::load_glsl(IPrContext &context, prosper::ShaderStage stage, const std::string &fileName, std::string *infoLog, std::string *debugInfoLog, std::vector<IncludeLine> &outIncludeLines, uint32_t &outLineOffset,
  const std::unordered_map<std::string, std::string> &definitions, bool applyPreprocessing)
{
	std::string ext;
	auto optFileName = find_shader_file("shaders/" + fileName, &ext);
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
	std::vector<IncludeLine> includeLines;
	unsigned int lineOffset = 0;
	std::string err;
	if(applyPreprocessing && glsl_preprocessing(context, stage, *optFileName, shaderCode, err, includeLines, lineOffset, definitions, hlsl) == false) {
		if(infoLog != nullptr)
			*infoLog = std::string("Module: \"") + fileName + "\"\n" + err;
		return {};
	}
	return shaderCode;
}

#endif
