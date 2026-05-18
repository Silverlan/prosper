// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.prosper:debug.object_register;

export import std.compat;

export namespace prosper {
	class ContextObject;
}
export namespace prosper::debug {
	enum class ObjectType : uint32_t {
		Image = 0u,
		ImageView,
		Sampler,
		Buffer,
		CommandBuffer,
		RenderPass,
		Framebuffer,
		DescriptorSet,
		Pipeline,
		Fence,
		Count,
	};

	class DLLPROSPER ObjectRegister {
	  public:
		static void register_context_object(ContextObject &o);
		static void unregister_context_object(ContextObject &o);
		static void initialize();

		void RegisterContextObject(ContextObject &obj);
		void UnregisterContextObject(ContextObject &obj);
		std::vector<ContextObject *> FindObjects(ObjectType type) const;
	  private:
		mutable std::mutex m_contextObjectMutex;
		std::unordered_set<ContextObject *> m_contextObjects = {};
	};
	DLLPROSPER std::vector<ContextObject *> find_registered_objects_by_type(ObjectType type);
}
