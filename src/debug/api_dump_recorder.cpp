// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "prosper_context.hpp"
#include "debug/api_dump_recorder.hpp"
#include <sharedutils/util.h>
#include "queries/prosper_query.hpp"
#include "prosper_util.hpp"

using namespace prosper;
#ifdef PR_DEBUG_API_DUMP
auto g_apiDumpEnabled = false;
void debug::set_api_dump_enabled(bool enabled) { g_apiDumpEnabled = enabled; }
bool debug::is_api_dump_enabled() { return g_apiDumpEnabled; }

std::unique_ptr<debug::BaseDumpValue> debug::dump(const std::string &val) { return std::make_unique<DataDumpValue>(val); }

std::unique_ptr<debug::BaseDumpValue> debug::dump(const bool &val) { return std::make_unique<DataDumpValue>(val ? "true" : "false"); }

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::BufferCopy &copyInfo)
{
	auto dv = std::make_unique<StructDumpValue>("BufferCopy");
	dv->AddKeyValue("srcOffset", copyInfo.srcOffset);
	dv->AddKeyValue("dstOffset", copyInfo.dstOffset);
	dv->AddKeyValue("size", copyInfo.size);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::ImageSubresourceLayers &layers)
{
	auto dv = std::make_unique<StructDumpValue>("ImageSubresourceLayers");
	dv->AddKeyValue("aspectMask", layers.aspectMask);
	dv->AddKeyValue("mipLevel", layers.mipLevel);
	dv->AddKeyValue("baseArrayLayer", layers.baseArrayLayer);
	dv->AddKeyValue("layerCount", layers.layerCount);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::Extent3D &extent3d)
{
	auto dv = std::make_unique<StructDumpValue>("Extent3D");
	dv->AddKeyValue("width", extent3d.width);
	dv->AddKeyValue("height", extent3d.height);
	dv->AddKeyValue("depth", extent3d.depth);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::Extent2D &extent2d)
{
	auto dv = std::make_unique<StructDumpValue>("Extent2D");
	dv->AddKeyValue("width", extent2d.width);
	dv->AddKeyValue("height", extent2d.height);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::Offset3D &offset3d)
{
	auto dv = std::make_unique<StructDumpValue>("Offset3D");
	dv->AddKeyValue("x", offset3d.x);
	dv->AddKeyValue("y", offset3d.y);
	dv->AddKeyValue("z", offset3d.z);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::CopyInfo &copyInfo)
{
	auto dv = std::make_unique<StructDumpValue>("CopyInfo");
	dv->AddKeyValue("width", copyInfo.width);
	dv->AddKeyValue("height", copyInfo.height);
	dv->AddKeyValue("srcSubresource", copyInfo.srcSubresource);
	dv->AddKeyValue("dstSubresource", copyInfo.dstSubresource);
	dv->AddKeyValue("srcOffset", copyInfo.srcOffset);
	dv->AddKeyValue("dstOffset", copyInfo.dstOffset);
	dv->AddKeyValue("srcImageLayout", copyInfo.srcImageLayout);
	dv->AddKeyValue("dstImageLayout", copyInfo.dstImageLayout);
	return dv;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::ContextObject &co)
{
	auto td = std::make_unique<StructDumpValue>("ContextObject");
	td->AddKeyValue("debugName", co.GetDebugName());
	td->AddKeyValue("address", "0x" + ::util::to_hex_string(&co));
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::IRenderPass &rp)
{
	auto &createInfo = rp.GetCreateInfo();
	auto td = std::make_unique<StructDumpValue>("RenderPass");
	td->AddKeyValue("debugName", rp.GetDebugName());
	td->AddKeyValue("address", "0x" + ::util::to_hex_string(&rp));
	td->AddKeyValue("attachments", createInfo.attachments);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::IShaderPipelineLayout &layout)
{
	auto td = std::make_unique<StructDumpValue>("ShaderPipelineLayout");
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::IDescriptorSet &descSet)
{
	auto td = std::make_unique<StructDumpValue>("DescriptorSet");
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::ClearValue &clearValue)
{
	auto td = std::make_unique<StructDumpValue>("ClearValue");
	td->AddKeyValue("color", clearValue.color);
	td->AddKeyValue("depthStencil", clearValue.depthStencil);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::ClearColorValue &color)
{
	auto td = std::make_unique<StructDumpValue>("ClearColorValue");
	td->AddKeyValue("float32", color.float32);
	td->AddKeyValue("int32", color.int32);
	td->AddKeyValue("uint32", color.uint32);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::ClearDepthStencilValue &depthStencil)
{
	auto td = std::make_unique<StructDumpValue>("ClearDepthStencilValue");
	td->AddKeyValue("depth", depthStencil.depth);
	td->AddKeyValue("stencil", depthStencil.stencil);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::Query &query)
{
	auto td = std::make_unique<StructDumpValue>("OcclusionQuery");
	td->AddKeyValue("queryId", query.GetQueryId());
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::ImageSubresourceRange &range)
{
	auto td = std::make_unique<StructDumpValue>("ImageSubresourceRange");
	td->AddKeyValue("baseMipLevel", range.baseMipLevel);
	td->AddKeyValue("levelCount", range.levelCount);
	td->AddKeyValue("baseArrayLayer", range.baseArrayLayer);
	td->AddKeyValue("layerCount", range.layerCount);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::ClearImageInfo &clearImgInfo)
{
	auto td = std::make_unique<StructDumpValue>("ClearImageInfo");
	td->AddKeyValue("subresourceRange", clearImgInfo.subresourceRange);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::PipelineBarrierInfo &barrierInfo)
{
	auto td = std::make_unique<StructDumpValue>("PipelineBarrierInfo");
	td->AddKeyValue("srcStageMask", barrierInfo.srcStageMask);
	td->AddKeyValue("dstStageMask", barrierInfo.dstStageMask);
	td->AddKeyValue("bufferBarriers", barrierInfo.bufferBarriers);
	td->AddKeyValue("imageBarriers", barrierInfo.imageBarriers);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::BufferBarrier &bufferBarrier)
{
	auto td = std::make_unique<StructDumpValue>("BufferBarrier");
	td->AddKeyValue("dstAccessMask", bufferBarrier.dstAccessMask);
	td->AddKeyValue("srcAccessMask", bufferBarrier.srcAccessMask);
	td->AddKeyValue("buffer", bufferBarrier.buffer);
	td->AddKeyValue("dstQueueFamilyIndex", bufferBarrier.dstQueueFamilyIndex);
	td->AddKeyValue("offset", bufferBarrier.offset);
	td->AddKeyValue("size", bufferBarrier.size);
	td->AddKeyValue("srcQueueFamilyIndex", bufferBarrier.srcQueueFamilyIndex);
	return td;
}
std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::ImageBarrier &imageBarrier)
{
	auto td = std::make_unique<StructDumpValue>("ImageBarrier");
	td->AddKeyValue("dstAccessMask", imageBarrier.dstAccessMask);
	td->AddKeyValue("srcAccessMask", imageBarrier.srcAccessMask);
	td->AddKeyValue("dstQueueFamilyIndex", imageBarrier.dstQueueFamilyIndex);
	td->AddKeyValue("image", imageBarrier.image);
	td->AddKeyValue("newLayout", imageBarrier.newLayout);
	td->AddKeyValue("oldLayout", imageBarrier.oldLayout);
	td->AddKeyValue("srcQueueFamilyIndex", imageBarrier.srcQueueFamilyIndex);
	td->AddKeyValue("subresourceRange", imageBarrier.subresourceRange);
	td->AddKeyValue("aspectMask", imageBarrier.aspectMask);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::BufferImageCopyInfo &copyInfo)
{
	auto td = std::make_unique<StructDumpValue>("BufferImageCopyInfo");
	td->AddKeyValue("bufferOffset", copyInfo.bufferOffset);
	td->AddKeyValue("bufferExtent", copyInfo.bufferExtent);
	td->AddKeyValue("imageOffset", copyInfo.imageOffset);
	td->AddKeyValue("imageExtent", copyInfo.imageExtent);
	td->AddKeyValue("mipLevel", copyInfo.mipLevel);
	td->AddKeyValue("baseArrayLayer", copyInfo.baseArrayLayer);
	td->AddKeyValue("layerCount", copyInfo.layerCount);
	td->AddKeyValue("aspectMask", copyInfo.aspectMask);
	td->AddKeyValue("dstImageLayout", copyInfo.dstImageLayout);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::BlitInfo &blitInfo)
{
	auto td = std::make_unique<StructDumpValue>("BlitInfo");
	td->AddKeyValue("srcSubresourceLayer", blitInfo.srcSubresourceLayer);
	td->AddKeyValue("dstSubresourceLayer", blitInfo.dstSubresourceLayer);
	td->AddKeyValue("offsetSrc", blitInfo.offsetSrc);
	td->AddKeyValue("extentsSrc", blitInfo.extentsSrc);
	td->AddKeyValue("offsetDst", blitInfo.offsetDst);
	td->AddKeyValue("extentsDst", blitInfo.extentsDst);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::ImageResolve &resolve)
{
	auto td = std::make_unique<StructDumpValue>("ImageResolve");
	td->AddKeyValue("src_subresource", resolve.src_subresource);
	td->AddKeyValue("src_offset", resolve.src_offset);
	td->AddKeyValue("dst_subresource", resolve.dst_subresource);
	td->AddKeyValue("dst_offset", resolve.dst_offset);
	td->AddKeyValue("extent", resolve.extent);
	return td;
}

std::unique_ptr<debug::BaseDumpValue> debug::dump(const prosper::util::RenderPassCreateInfo::AttachmentInfo &attInfo)
{
	auto td = std::make_unique<StructDumpValue>("AttachmentInfo");
	td->AddKeyValue("format", prosper::util::to_string(attInfo.format));
	td->AddKeyValue("sampleCount", attInfo.sampleCount);
	td->AddKeyValue("loadOp", attInfo.loadOp);
	td->AddKeyValue("storeOp", attInfo.storeOp);
	td->AddKeyValue("stencilLoadOp", attInfo.stencilLoadOp);
	td->AddKeyValue("stencilStoreOp", attInfo.stencilStoreOp);
	td->AddKeyValue("initialLayout", attInfo.initialLayout);
	td->AddKeyValue("finalLayout", attInfo.finalLayout);
	return td;
}

debug::ApiDumpRecorder::ApiDumpRecorder(prosper::ContextObject *contextObject, ApiDumpRecorder *parent) : m_contextObject {contextObject}, m_parent {parent} { m_recordSets.resize(2); }
debug::ApiDumpRecorder::~ApiDumpRecorder() {}

void debug::ApiDumpRecorder::Clear()
{
	m_recordMutex.lock();
	for(size_t i = 1; i < m_recordSets.size(); ++i)
		m_recordSets[i - 1] = std::move(m_recordSets[i]);
	m_recordSets.back().clear();
	m_recordMutex.unlock();
}

void debug::ApiDumpRecorder::Print(std::stringstream &ss) const
{
	int32_t idxRecord = -static_cast<int32_t>(m_recordSets.size()) + 1;
	for(auto &recordSet : m_recordSets) {
		ss << "Record set " << idxRecord++ << ":\n";
		size_t idx = 0;
		for(auto &record : recordSet) {
			ss << idx++ << ": ";
			record.Print(ss);
			ss << "\n";
		}
	}
}

void debug::ApiDumpRecorder::PrintCallTrace(uint64_t cmdIdx, std::stringstream &ss, int32_t recordSetIdx) const
{
	if(m_recordSets.empty())
		return;
	auto setIdx = (static_cast<int32_t>(m_recordSets.size()) - 1) - recordSetIdx;
	if(setIdx < 0 || setIdx >= m_recordSets.size())
		return;
	auto &recordSet = m_recordSets[setIdx];
	if(cmdIdx >= recordSet.size())
		return;
	auto &r = recordSet[cmdIdx];
	ss << r.callTrace;
}

debug::ApiDumpRecorder::Record::Record(const Record &other) { operator=(other); }
debug::ApiDumpRecorder::Record::~Record() {}

debug::DumpKeyValue::DumpKeyValue() {}
debug::DumpKeyValue::DumpKeyValue(const DumpKeyValue &other) : key(other.key), value(other.value->Clone()) {}
debug::DumpKeyValue::DumpKeyValue(const std::string &key, std::unique_ptr<BaseDumpValue> &&value) : key(key), value(std::move(value)) {}

debug::DumpKeyValue &debug::DumpKeyValue::operator=(const DumpKeyValue &other)
{
	key = other.key;
	value = other.value->Clone();
	return *this;
}

debug::ApiDumpRecorder::Record &debug::ApiDumpRecorder::Record::operator=(const Record &other)
{
	funcName = other.funcName;
	argNames = other.argNames;
	argValues.clear();
	for(auto &val : other.argValues)
		argValues.push_back(val->Clone());
	retType = other.retType;
	callTrace = other.callTrace;
	contextObjectName = other.contextObjectName;
	return *this;
}

void debug::ApiDumpRecorder::Record::Print(std::stringstream &ss) const
{
	if(!contextObjectName.empty())
		ss << contextObjectName << "::";
	ss << funcName << "(";
	for(auto i = 0u; i < argNames.size(); ++i) {
		if(i > 0)
			ss << ", ";
		ss << argNames[i];
	}
	ss << ") returns " << retType << ":\n";
	std::function<void(std::stringstream &, BaseDumpValue &, const std::string &)> print {};
	print = [&print](std::stringstream &ss, BaseDumpValue &val, const std::string &offset) {
		auto *dtVal = dynamic_cast<DataDumpValue *>(&val);
		if(dtVal)
			ss << offset << dtVal->value << "\n";
		else {
			auto *stVal = dynamic_cast<StructDumpValue *>(&val);
			if(stVal) {
				ss << offset << stVal->typeName << " {\n";
				for(auto &kv : stVal->keyvalues) {
					ss << offset << "  " << kv.key << ": ";
					print(ss, *kv.value, offset + "  ");
				}
				ss << offset << "}\n";
			}
		}
	};
	for(auto i = 0u; i < argNames.size(); ++i) {
		ss << "  " << argNames[i] << ": ";
		auto &val = argValues[i];
		if(val)
			print(ss, *val, "  ");
		else
			ss << "nullptr";
		ss << "\n";
	}
}
#endif
