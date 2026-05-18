// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :debug.object_register;

using namespace prosper;

static std::optional<debug::ObjectType> get_object_type(const ContextObject &obj)
{
	if(dynamic_cast<const IImage *>(&obj))
		return debug::ObjectType::Image;
	if(dynamic_cast<const IImageView *>(&obj))
		return debug::ObjectType::ImageView;
	if(dynamic_cast<const ISampler *>(&obj))
		return debug::ObjectType::Sampler;
	if(dynamic_cast<const IBuffer *>(&obj))
		return debug::ObjectType::Buffer;
	if(dynamic_cast<const ICommandBuffer *>(&obj))
		return debug::ObjectType::CommandBuffer;
	if(dynamic_cast<const IRenderPass *>(&obj))
		return debug::ObjectType::RenderPass;
	if(dynamic_cast<const IFramebuffer *>(&obj))
		return debug::ObjectType::Framebuffer;
	if(dynamic_cast<const IDescriptorSet *>(&obj))
		return debug::ObjectType::DescriptorSet;
	if(dynamic_cast<const IFence *>(&obj))
		return debug::ObjectType::Fence;
	return {};
}

static std::unique_ptr<debug::ObjectRegister> g_objectRegister;

void debug::ObjectRegister::register_context_object(ContextObject &o)
{
	if(!g_objectRegister)
		return;
	g_objectRegister->RegisterContextObject(o);
}
void debug::ObjectRegister::unregister_context_object(ContextObject &o)
{
	if(!g_objectRegister)
		return;
	g_objectRegister->UnregisterContextObject(o);
}
void debug::ObjectRegister::initialize() { g_objectRegister = std::make_unique<ObjectRegister>(); }

void debug::ObjectRegister::RegisterContextObject(ContextObject &obj)
{
	std::scoped_lock lock {m_contextObjectMutex};
	m_contextObjects.insert(&obj);
}
void debug::ObjectRegister::UnregisterContextObject(ContextObject &obj)
{
	std::scoped_lock lock {m_contextObjectMutex};
	auto it = m_contextObjects.find(&obj);
	if(it == m_contextObjects.end())
		return;
	m_contextObjects.erase(it);
}
std::vector<ContextObject *> debug::ObjectRegister::FindObjects(ObjectType type) const
{
	std::scoped_lock lock {m_contextObjectMutex};
	std::vector<ContextObject *> objects;
	for(auto *o : m_contextObjects) {
		if(get_object_type(*o) != type)
			continue;
		objects.push_back(o);
	}
	return objects;
}

std::vector<ContextObject *> debug::find_registered_objects_by_type(ObjectType type)
{
	std::vector<ContextObject *> objects;
	if(g_objectRegister == nullptr)
		return {};
	return g_objectRegister->FindObjects(type);
}
