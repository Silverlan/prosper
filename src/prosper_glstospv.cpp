/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_prosper.h"
#define VK_ENABLE_GLSLANG

#ifdef VK_ENABLE_GLSLANG
#include "prosper_includes.hpp"
#include "prosper_glstospv.hpp"
#include "prosper_context.hpp"
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util_string.h>
#include <sstream>

static unsigned int get_line_break(std::string &str,int pos=0)
{
	auto len = str.length();
	while(pos < len && str[pos] != '\n')
		pos++;
	return pos;
}

struct IncludeLine
{
	IncludeLine(UInt32 line,const std::string &l,UInt32 dp=0,bool bRet=false)
		: lineId(line),line(l),ret(bRet),depth(dp)
	{}
	IncludeLine()
		: IncludeLine(0,"")
	{}
	UInt32 lineId;
	std::string line;
	bool ret;
	UInt32 depth;
};

static bool glsl_preprocessing(const std::string &path,std::string &shader,std::string *err,std::vector<IncludeLine> &includeLines,unsigned int &lineId,UInt32 depth=0,bool=false)
{
	if(!includeLines.empty() && includeLines.back().lineId == lineId)
		includeLines.back() = IncludeLine(lineId,path,depth);
	else
		includeLines.push_back(IncludeLine(lineId,path,depth));
	//auto incIdx = includeLines.size() -1;
	std::string sub = FileManager::GetPath(const_cast<std::string&>(path));
	auto len = shader.length();
	unsigned int br;
	unsigned int brLast = 0;
	do
	{
		//std::cout<<"Length: "<<len<<"("<<shader.length()<<")"<<std::endl;
		br = get_line_break(shader,brLast);
		std::string l = shader.substr(brLast,br -brLast);
		ustring::remove_whitespace(l);
		if(l.length() >= 8)
		{
			if(l.substr(0,8) == "#include")
			{
				std::string inc = l.substr(8,l.length());
				ustring::remove_whitespace(inc);
				ustring::remove_quotes(inc);
				std::string includePath = "";
				if(inc.empty() == false && inc.front() == '/')
					includePath = inc;
				else
					includePath = sub.substr(sub.find_first_of("/\\") +1) +inc;
				includePath = "shaders\\" +FileManager::GetCanonicalizedPath(includePath);
				if(includePath.substr(includePath.length() -4) != ".gls")
					includePath += ".gls";
				auto f = FileManager::OpenFile(includePath.c_str(),"rb");
				if(f != NULL)
				{
					unsigned long long flen = f->GetSize();
					std::vector<char> data(flen +1);
					f->Read(data.data(),flen);
					data[flen] = '\0';
					std::string subShader = data.data();
					lineId++; // #include-directive
					if(glsl_preprocessing(includePath,subShader,err,includeLines,lineId,depth +1) == false)
						return false;
					includeLines.push_back(IncludeLine(lineId,path,depth,true));
					lineId--; // Why?
					//subShader = std::string("\n#line 1\n") +subShader +std::string("\n#line ") +std::to_string(lineId) +std::string("\n");
					shader = shader.substr(0,brLast) +std::string("//") +l +std::string("\n") +subShader +shader.substr(br);
					br = CUInt32(brLast) +CUInt32(l.length()) +3 +CUInt32(subShader.length());
				}
				else
				{
					if(err != NULL)
						*err = "Unable to include file '" +includePath +"' (In: '" +path +"'): File not found!";
					shader = shader.substr(0,brLast) +shader.substr(br);
					br = brLast;
					return false;
				}
				len = shader.length();
			}
		}
		lineId++;
		brLast = br +1;
	}
	while(br < len);
	return true;
}

static bool glsl_preprocessing(prosper::Context &context,Anvil::ShaderStage stage,const std::string &path,std::string &shader,std::string *err,std::vector<IncludeLine> &includeLines,unsigned int &lineId,bool bHlsl=false)
{
	lineId = 0;
	auto r = glsl_preprocessing(path,shader,err,includeLines,lineId,0,true);
	if(r == false)
		return false;
	// Custom definitions
	std::unordered_map<std::string,std::string> definitions;
	//definitions["__MAX_VERTEX_TEXTURE_IMAGE_UNITS__"] = std::to_string(maxTextureUnits);
	std::string prefix = (bHlsl == false) ? "GLS_" : "HLS_";
	switch(stage)
	{
		case Anvil::ShaderStage::FRAGMENT:
			definitions[prefix +"FRAGMENT_SHADER"] = "1";
			break;
		case Anvil::ShaderStage::VERTEX:
			definitions[prefix +"VERTEX_SHADER"] = "1";
			break;
		case Anvil::ShaderStage::GEOMETRY:
			definitions[prefix +"GEOMETRY_SHADER"] = "1";
			break;
		case Anvil::ShaderStage::COMPUTE:
			definitions[prefix +"COMPUTE_SHADER"] = "1";
			break;
	}

	auto &properties = context.GetDevice().get_physical_device_properties();
	switch(properties.core_vk1_0_properties_ptr->vendor_id)
	{
		case umath::to_integral(prosper::Vendor::AMD):
			definitions[prefix +"VENDOR_AMD"] = "1";
			break;
		case umath::to_integral(prosper::Vendor::Nvidia):
			definitions[prefix +"VENDOR_NVIDIA"] = "1";
			break;
		case umath::to_integral(prosper::Vendor::Intel):
			definitions[prefix +"VENDOR_INTEL"] = "1";
			break;
	}

	lineId = static_cast<unsigned int>(definitions.size() +1);
	std::string def;
	std::unordered_map<std::string,std::string>::iterator itDef;
	for(itDef=definitions.begin();itDef!=definitions.end();itDef++)
		def += "#define " +itDef->first +" " +itDef->second +"\n";
	def += "#line 1\n";

	// Can't add anything before #version, so look for it first
	size_t posAppend = 0;
	size_t posVersion = shader.find("#version");
	if(posVersion != std::string::npos)
	{
		size_t posNl = shader.find("\n",posVersion);
		if(posNl != std::string::npos)
			posAppend = posNl;
		else
			posAppend = shader.length();
		lineId++;
	}
	shader = shader.substr(0,posAppend) +"\n" +def +shader.substr(posAppend +1,shader.length());
	return true;
}

static void glsl_translate_error(const std::string &shaderCode,const std::string &errorMsg,const std::string &pathMainShader,const std::vector<IncludeLine> &includeLines,Int32 lineOffset,std::string *err)
{
	std::vector<std::string> codeLines;
	ustring::explode(shaderCode,"\n",codeLines);

	std::vector<std::string> errorLines;
	ustring::explode(errorMsg,"\n",errorLines);
	auto numLines = errorLines.size();
	for(UInt i=0;i<numLines;i++)
	{
		auto &msg = errorLines[i];
		ustring::remove_whitespace(msg);
		if(!msg.empty())
		{
			auto bAMD = false;
			auto bWarning = false;
			if(msg.substr(0,9) == "ERROR: 0:")
				bAMD = true;
			else if(msg.substr(0,11) == "WARNING: 0:")
			{
				bAMD = true;
				bWarning = true;
			}
			auto bNvidia = (!bAMD && msg.substr(0,2) == "0(") ? true : false;
			if(bAMD == true || bNvidia == true)
			{
				auto st = (bAMD == true) ? ((bWarning == true) ? 11 : 9) : 2;
				auto end = (bAMD == true) ? msg.find_first_of(':',st) : msg.find_first_of(')',st);
				if(end != ustring::NOT_FOUND)
				{
					auto line = atoi(msg.substr(st,end -st).c_str());
					auto lineInternal = line;
					std::string file;
					auto bFound = false;
					auto numIncludeLines = includeLines.size();
					for(UInt idx=0;idx<numIncludeLines;idx++)
					{
						auto it = includeLines.begin() +idx;
						if(CUInt32(line) >= it->lineId && idx < CUInt32(numIncludeLines -1))
							continue;
						if(it != includeLines.begin())
						{
							if(idx < CUInt32(numIncludeLines -1) || CUInt32(line) < it->lineId)
							{
								it--;
								idx--;
							}
							auto errLine = line -it->lineId +1;
							for(auto idx2=idx;idx2>0;)
							{
								idx2--;
								auto it2 = includeLines.begin() +idx2;
								if(it2->line == it->line && it2->depth == it->depth)
								{
									errLine += includeLines[idx2 +1].lineId -it2->lineId;
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
					msgOut<<msg.substr(0,st)<<std::to_string(line)<<
						((bAMD == true) ? (std::string(": '") +file +std::string("':")) :
						(std::string(")('") +file +std::string("')")))<<
						msg.substr(end +1);
					lineInternal += lineOffset;
					if(lineInternal <= codeLines.size())
					{
						auto lineInternalId = lineInternal -1;
						for(Int32 iline=umath::max(lineInternalId -2,0);iline<=umath::min(lineInternalId +2,static_cast<Int32>(codeLines.size() -1));iline++)
						{
							if(iline == lineInternalId)
								msgOut<<"\n  > "<<codeLines[iline];
							else
								msgOut<<"\n    "<<codeLines[iline];
						}
					}
					msg = msgOut.str();
				}
			}
			*err += msg;
			if(i < (numLines -1))
				*err += "\n";
		}
	}
}

static bool glsl_to_spv(prosper::Context &context,Anvil::ShaderStage stage,const char *pshader,std::vector<unsigned int> &spirv,std::string *infoLog,std::string *debugInfoLog,const std::string &fileName,bool bHlsl=false)
{
	std::string shaderCode{pshader};
	std::vector<IncludeLine> includeLines;
	unsigned int lineOffset = 0;
	if(glsl_preprocessing(context,stage,fileName,shaderCode,infoLog,includeLines,lineOffset,bHlsl) == false)
	{
		if(infoLog != nullptr)
			*infoLog = std::string("Module: \"") +fileName +"\"\n" +(*infoLog);
		return false;
	}
	auto &dev = context.GetDevice();
	auto shaderPtr = Anvil::GLSLShaderToSPIRVGenerator::create(
		&dev,
		Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
		shaderCode,
		stage
	);
	if(shaderPtr == nullptr)
		return false;
	if(shaderPtr->get_spirv_blob_size() == 0u)
	{
		auto &shaderInfoLog = shaderPtr->get_shader_info_log();
		if(shaderInfoLog.empty() == false)
		{
			if(infoLog != nullptr)
			{
				glsl_translate_error(shaderCode,shaderInfoLog,fileName,includeLines,CUInt32(lineOffset),infoLog);
				*infoLog = std::string("Shader File: \"") +fileName +"\"\n" +(*infoLog);
			}
			if(debugInfoLog != nullptr)
				*debugInfoLog = shaderPtr->get_debug_info_log();
		}
		else
		{
			auto &programInfoLog = shaderPtr->get_program_info_log();
			if(programInfoLog.empty() == false)
			{
				if(infoLog != nullptr)
				{
					glsl_translate_error(shaderCode,programInfoLog,fileName,includeLines,CUInt32(lineOffset),infoLog);
					*infoLog = std::string("Shader File: \"") +fileName +"\"\n" +(*infoLog);
				}
				if(debugInfoLog != nullptr)
					*debugInfoLog = shaderPtr->get_program_debug_info_log();
			}
			else if(infoLog != nullptr)
				*infoLog = "An unknown error has occurred!";
		}
		return false;
	}
	auto shaderMod = Anvil::ShaderModule::create_from_spirv_generator(&dev,shaderPtr.get());
	spirv = shaderMod->get_spirv_blob();
	return true;
}

void prosper::dump_parsed_shader(Context &context,uint32_t stage,const std::string &shaderFile,const std::string &fileName)
{
	auto f = FileManager::OpenFile(shaderFile.c_str(),"r");
	if(f == nullptr)
		return;
	auto shaderCode = f->ReadString();
	std::vector<IncludeLine> includeLines;
	unsigned int lineOffset = 0;
	if(glsl_preprocessing(context,static_cast<Anvil::ShaderStage>(stage),fileName,shaderCode,nullptr,includeLines,lineOffset) == false)
		return;
	auto fOut = FileManager::OpenFile<VFilePtrReal>(fileName.c_str(),"w");
	if(fOut == nullptr)
		return;
	fOut->WriteString(shaderCode);
}

bool prosper::glsl_to_spv(Context &context,uint32_t stage,const std::string &fileName,std::vector<unsigned int> &spirv,std::string *infoLog,std::string *debugInfoLog,bool bReload)
{
	auto fName = fileName;
	std::string ext;
	if(!ufile::get_extension(fileName,&ext))
	{
		auto fNameSpv = "cache/" +fileName +".spv";
		auto bSpvExists = FileManager::Exists(fNameSpv);
		if(bSpvExists == true)
		{
			ext = "spv";
			fName = fNameSpv;
		}
		if(bSpvExists == false || bReload == true)
		{
			auto fNameGls = fileName +".gls";
			if(FileManager::Exists(fNameGls))
			{
				ext = "gls";
				fName = fNameGls;
			}
			else if(bSpvExists == false)
			{
				ext = "hls";
				fName = fileName +".hls";
			}
		}
	}
	ustring::to_lower(ext);
	if(!FileManager::Exists(fName))
	{
		if(infoLog != nullptr)
			*infoLog = std::string("File '") +fName +std::string("' not found!");
		return false;
	}
	if(ext != "gls" && ext != "hls") // We'll assume it's a SPIR-V file
	{
		auto f = FileManager::OpenFile(fName.c_str(),"rb");
		if(f == nullptr)
		{
			if(infoLog != nullptr)
				*infoLog = std::string("Unable to open file '") +fName +std::string("'!");
			return false;
		}
		auto sz = f->GetSize();
		assert((sz %sizeof(unsigned int)) == 0);
		if((sz %sizeof(unsigned int)) != 0)
			return false;
		auto origSize = spirv.size();
		spirv.resize(origSize +sz /sizeof(unsigned int));
		f->Read(spirv.data() +origSize,sz);
		return true;
	}
	auto f = FileManager::OpenFile(fName.c_str(),"r");
	if(f == nullptr)
	{
		if(infoLog != nullptr)
			*infoLog = std::string("Unable to open file '") +fName +std::string("'!");
		return false;
	}
	auto content = f->ReadString();
	auto r = ::glsl_to_spv(context,static_cast<Anvil::ShaderStage>(stage),content.c_str(),spirv,infoLog,debugInfoLog,fName,(ext == "hls") ? true : false);
	if(r == false)
		return r;
	auto spirvName = "cache/" +fName.substr(0,fName.length() -ext.length()) +"spv";
	FileManager::CreatePath(ufile::get_path_from_filename(spirvName).c_str());
	auto fOut = FileManager::OpenFile<VFilePtrReal>(spirvName.c_str(),"wb");
	if(fOut == nullptr)
		return r;
	fOut->Write(spirv.data(),spirv.size() *sizeof(unsigned int));
	return r;
}

#endif
