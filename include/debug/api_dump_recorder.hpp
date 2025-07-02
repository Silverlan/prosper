// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_API_DUMP_RECORDER_HPP__
#define __PROSPER_API_DUMP_RECORDER_HPP__

#include "prosper_definitions.hpp"

#ifdef PR_DEBUG_API_DUMP

#include "buffers/prosper_buffer.hpp"
#include "image/prosper_image.hpp"
#include "prosper_render_pass.hpp"
#include <sharedutils/magic_enum.hpp>
#include <sharedutils/util_debug.h>
#include <sstream>

namespace prosper::debug {
	DLLPROSPER void set_api_dump_enabled(bool enabled);
	DLLPROSPER bool is_api_dump_enabled();

	struct DLLPROSPER BaseDumpValue {
		virtual ~BaseDumpValue() {}
		virtual std::unique_ptr<BaseDumpValue> Clone() const = 0;
	};

	struct DLLPROSPER DataDumpValue : BaseDumpValue {
		DataDumpValue(const std::string &value) : value {value} {}
		DataDumpValue(const std::string_view &value) : value {value} {}
		DataDumpValue(const char *value) : value {value} {}
		std::unique_ptr<BaseDumpValue> Clone() const override { return std::make_unique<DataDumpValue>(value); }
		std::string value;
	};

	struct DLLPROSPER DumpKeyValue {
		DumpKeyValue();
		DumpKeyValue(const DumpKeyValue &other);
		DumpKeyValue(const std::string &key, std::unique_ptr<BaseDumpValue> &&value);
		std::string key;
		std::unique_ptr<BaseDumpValue> value;
		DumpKeyValue &operator=(const DumpKeyValue &other);
	};

	struct DLLPROSPER StructDumpValue : BaseDumpValue {
		StructDumpValue(const std::string &typeName) : typeName {typeName} {}
		std::unique_ptr<BaseDumpValue> Clone() const override
		{
			auto ret = std::make_unique<StructDumpValue>(typeName);
			for(auto &kv : keyvalues)
				ret->keyvalues.push_back({kv.key, kv.value->Clone()});
			return ret;
		}
		std::string typeName;
		std::vector<DumpKeyValue> keyvalues;
		template<typename T>
		void AddKeyValue(const std::string &key, const T &val);
	};

	template<typename T>
	    requires(std::is_enum_v<T>)
	std::unique_ptr<BaseDumpValue> dump(const T &val)
	{
		if constexpr(std::is_same_v<T, MemoryFeatureFlags> || std::is_same_v<T, QueueFamilyFlags> || std::is_same_v<T, BufferUsageFlags> || std::is_same_v<T, ImageUsageFlags> || std::is_same_v<T, SampleCountFlags> || std::is_same_v<T, ImageCreateFlags>
		  || std::is_same_v<T, ImageAspectFlags> || std::is_same_v<T, AccessFlags> || std::is_same_v<T, PipelineStageFlags> || std::is_same_v<T, ShaderStageFlags> || std::is_same_v<T, StencilFaceFlags> || std::is_same_v<T, QueryResultFlags> || std::is_same_v<T, DescriptorBindingFlags>
		  || std::is_same_v<T, PipelineCreateFlags> || std::is_same_v<T, CullModeFlags> || std::is_same_v<T, ColorComponentFlags> || std::is_same_v<T, DebugMessageSeverityFlags> || std::is_same_v<T, MemoryPropertyFlags> || std::is_same_v<T, QueryPipelineStatisticFlags>
		  || std::is_same_v<T, DebugReportFlags> || std::is_same_v<T, FormatFeatureFlags>)
			return std::make_unique<DataDumpValue>(magic_enum::flags::enum_name(val).data());
		return std::make_unique<DataDumpValue>(magic_enum::enum_name(val));
	}

	template<typename T>
	    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
	std::unique_ptr<BaseDumpValue> dump(const T &val)
	{
		return std::make_unique<DataDumpValue>(std::to_string(val));
	}

	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const std::string &val);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const bool &val);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::BufferCopy &copyInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::ImageSubresourceLayers &layers);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::Extent3D &extent3d);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::Extent2D &extent2d);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::Offset3D &offset3d);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::CopyInfo &copyInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::ContextObject &co);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::IRenderPass &rp);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::Query &query);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::IShaderPipelineLayout &layout);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::IDescriptorSet &descSet);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::ClearValue &clearValue);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::ClearColorValue &color);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::ClearDepthStencilValue &depthStencil);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::ImageSubresourceRange &range);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::ClearImageInfo &clearImgInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::PipelineBarrierInfo &barrierInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::BufferBarrier &bufferBarrier);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::ImageBarrier &imageBarrier);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::BufferImageCopyInfo &copyInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::BlitInfo &blitInfo);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::ImageResolve &resolve);
	DLLPROSPER std::unique_ptr<BaseDumpValue> dump(const prosper::util::RenderPassCreateInfo::AttachmentInfo &attInfo);
	template<typename T>
	    requires(std::is_same_v<T, Vector2> || std::is_same_v<T, Vector2i> || std::is_same_v<T, Vector3> || std::is_same_v<T, Vector3i> || std::is_same_v<T, Vector4> || std::is_same_v<T, Vector4i>)
	std::unique_ptr<BaseDumpValue> dump(const T &vec)
	{
		auto dv = std::make_unique<StructDumpValue>("Vector");
		dv->AddKeyValue("x", vec.x);
		dv->AddKeyValue("y", vec.y);
		if constexpr(std::is_same_v<T, Vector3> || std::is_same_v<T, Vector3i> || std::is_same_v<T, Vector4> || std::is_same_v<T, Vector4i>) {
			dv->AddKeyValue("z", vec.z);
			if constexpr(std::is_same_v<T, Vector4> || std::is_same_v<T, Vector4i>)
				dv->AddKeyValue("w", vec.w);
		}
		return dv;
	}

	template<typename T>
	std::unique_ptr<BaseDumpValue> dump(const std::vector<T> &vec)
	{
		auto dv = std::make_unique<StructDumpValue>("std::vector");
		size_t i = 0;
		for(auto &v : vec)
			dv->AddKeyValue("[" + std::to_string(i++) + "]", v);
		return dv;
	}

	template<typename T, std::size_t N>
	std::unique_ptr<BaseDumpValue> dump(const std::array<T, N> &arr)
	{
		auto dv = std::make_unique<StructDumpValue>("std::array");
		size_t i = 0;
		for(auto &v : arr)
			dv->AddKeyValue("[" + std::to_string(i++) + "]", v);
		return dv;
	}

	template<typename T>
	std::unique_ptr<BaseDumpValue> dump(const std::optional<T> &opt)
	{
		if(opt.has_value())
			return dump(opt.value());
		return std::make_unique<DataDumpValue>("std::nullopt");
	}

	template<typename T>
	std::unique_ptr<BaseDumpValue> dump(const std::shared_ptr<T> &ptr)
	{
		if(ptr)
			return dump(*ptr);
		return std::make_unique<DataDumpValue>("std::shared_ptr(nullptr)");
	}

	template<typename T>
	std::unique_ptr<BaseDumpValue> dump(const std::unique_ptr<T> &ptr)
	{
		if(ptr)
			return dump(*ptr);
		return std::make_unique<DataDumpValue>("std::unique_ptr(nullptr)");
	}

	template<typename T>
	    requires(!std::is_same_v<T, void>)
	std::unique_ptr<BaseDumpValue> dump(const T *ptr)
	{
		if(ptr)
			return dump(*ptr);
		return std::make_unique<DataDumpValue>("nullptr");
	}

	template<typename T>
	void StructDumpValue::AddKeyValue(const std::string &key, const T &val)
	{
		keyvalues.push_back({key, dump(val)});
	}

	struct DLLPROSPER ApiDumpRecorder {
		struct DLLPROSPER Record {
			struct DLLPROSPER Wrapper {
				Wrapper(ApiDumpRecorder &recorder, Record &record) : recorder {recorder}, record {record} {}
				~Wrapper()
				{
					auto *parent = recorder.GetParent();
					if(parent) {
						parent->m_recordMutex.lock();
						parent->m_recordSets.back().push_back(record);
						parent->m_recordMutex.unlock();
					}
					recorder.m_recordMutex.unlock();
				}
				Record *operator->() { return &record; }
				ApiDumpRecorder &recorder;
				Record &record;
			};
			Record(const std::string &funcName) : funcName {funcName}, callTrace {::util::debug::get_formatted_stack_backtrace_string()} {}
			Record(const Record &other);
			~Record();
			template<typename T>
			void AddArgument(const std::string &name, const T &val)
			{
				argNames.push_back(name);
				argValues.push_back(std::move(dump(val)));
			}
			void Print(std::stringstream &ss) const;
			Record &operator=(const Record &other);
			std::string contextObjectName;
			std::string funcName;
			std::vector<std::string> argNames;
			std::vector<std::unique_ptr<BaseDumpValue>> argValues;
			std::string retType = "void";
			std::string callTrace;
		};
		ApiDumpRecorder(prosper::ContextObject *contextObject = nullptr, ApiDumpRecorder *parent = nullptr);
		~ApiDumpRecorder();
		void Clear();
		void Print(std::stringstream &ss) const;
		void PrintCallTrace(uint64_t cmdIdx, std::stringstream &ss, int32_t recordSet = 0) const;
		template<typename TReturn>
		Record::Wrapper AddRecord(const std::string &funcName)
		{
			m_recordMutex.lock();
			auto &records = m_recordSets.back();
			records.push_back({funcName});
			auto &rec = records.back();
			rec.retType = typeid(TReturn).name();
			rec.contextObjectName = m_contextObject->GetDebugName();
			return Record::Wrapper {*this, rec};
		}
		ApiDumpRecorder *GetParent() { return m_parent; }
	  private:
		std::vector<std::vector<Record>> m_recordSets;
		std::mutex m_recordMutex;
		ApiDumpRecorder *m_parent = nullptr;
		prosper::ContextObject *m_contextObject = nullptr;
	};
};

#endif
#endif
