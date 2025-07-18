// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PROSPER_DESCRIPTOR_SET_GROUP_HPP__
#define __PROSPER_DESCRIPTOR_SET_GROUP_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include "prosper_structs.hpp"
#include <sharedutils/functioncallback.h>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper {
	class Texture;
	class IDescriptorSet;
	class IBuffer;
	class DLLPROSPER DescriptorSetBinding {
	  public:
		enum class Type : uint8_t {
			StorageImage = 0,
			Texture,
			ArrayTexture,
			UniformBuffer,
			DynamicUniformBuffer,
			StorageBuffer,
			DynamicStorageBuffer,

			Count
		};
		DescriptorSetBinding(const DescriptorSetBinding &) = delete;
		DescriptorSetBinding &operator=(const DescriptorSetBinding &) = delete;
		virtual ~DescriptorSetBinding() = default;
		uint32_t GetBindingIndex() const;
		IDescriptorSet &GetDescriptorSet() const;
		virtual Type GetType() const = 0;
	  protected:
		DescriptorSetBinding(IDescriptorSet &descSet, uint32_t bindingIdx);
		IDescriptorSet &m_descriptorSet;
		uint32_t m_bindingIndex = 0u;
	};

	class DLLPROSPER DescriptorSetBindingStorageImage : public DescriptorSetBinding {
	  public:
		DescriptorSetBindingStorageImage(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::Texture &texture, std::optional<uint32_t> layerId = {});
		virtual Type GetType() const override { return Type::StorageImage; }
		std::optional<uint32_t> GetLayerIndex() const;
		const std::weak_ptr<prosper::Texture> &GetTexture() const;
	  private:
		std::weak_ptr<prosper::Texture> m_texture {};
		std::optional<uint32_t> m_layerId = {};
	};

	class DLLPROSPER DescriptorSetBindingTexture : public DescriptorSetBinding {
	  public:
		DescriptorSetBindingTexture(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::Texture &texture, std::optional<uint32_t> layerId = {});
		virtual Type GetType() const override { return Type::Texture; }
		std::optional<uint32_t> GetLayerIndex() const;
		const std::weak_ptr<prosper::Texture> &GetTexture() const;
	  private:
		std::weak_ptr<prosper::Texture> m_texture {};
		std::optional<uint32_t> m_layerId = {};
	};

	class DLLPROSPER DescriptorSetBindingArrayTexture : public DescriptorSetBinding {
	  public:
		DescriptorSetBindingArrayTexture(IDescriptorSet &descSet, uint32_t bindingIdx);
		virtual Type GetType() const override { return Type::ArrayTexture; }
		void SetArrayBinding(uint32_t arrayIndex, std::unique_ptr<DescriptorSetBindingTexture> bindingTexture);
		uint32_t GetArrayCount() const;
		DescriptorSetBindingTexture *GetArrayItem(uint32_t idx);
	  private:
		std::vector<std::unique_ptr<DescriptorSetBindingTexture>> m_arrayItems = {};
	};

	class DLLPROSPER DescriptorSetBindingBaseBuffer : public DescriptorSetBinding {
	  public:
		DescriptorSetBindingBaseBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size);
		virtual ~DescriptorSetBindingBaseBuffer() override;
		uint64_t GetStartOffset() const;
		uint64_t GetSize() const;
		const std::weak_ptr<prosper::IBuffer> &GetBuffer() const;
	  private:
		CallbackHandle m_cbBufferReallocation {};
		std::weak_ptr<prosper::IBuffer> m_buffer {};
		uint64_t m_startOffset = 0u;
		uint64_t m_size = 0u;
	};

	class DLLPROSPER DescriptorSetBindingUniformBuffer : public DescriptorSetBindingBaseBuffer {
	  public:
		DescriptorSetBindingUniformBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size);
		virtual Type GetType() const override { return Type::UniformBuffer; }
	};

	class DLLPROSPER DescriptorSetBindingDynamicUniformBuffer : public DescriptorSetBindingBaseBuffer {
	  public:
		DescriptorSetBindingDynamicUniformBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size);
		virtual Type GetType() const override { return Type::DynamicUniformBuffer; }
	};

	class DLLPROSPER DescriptorSetBindingStorageBuffer : public DescriptorSetBindingBaseBuffer {
	  public:
		DescriptorSetBindingStorageBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size);
		virtual Type GetType() const override { return Type::StorageBuffer; }
	};

	class DLLPROSPER DescriptorSetBindingDynamicStorageBuffer : public DescriptorSetBindingBaseBuffer {
	  public:
		DescriptorSetBindingDynamicStorageBuffer(IDescriptorSet &descSet, uint32_t bindingIdx, prosper::IBuffer &buffer, uint64_t startOffset, uint64_t size);
		virtual Type GetType() const override { return Type::DynamicStorageBuffer; }
	};

	class IDescriptorSetGroup;
	class DLLPROSPER IDescriptorSet {
	  public:
		IDescriptorSet(IDescriptorSetGroup &dsg);
		IDescriptorSet(const IDescriptorSet &) = delete;
		IDescriptorSet &operator=(const IDescriptorSet &) = delete;
		virtual ~IDescriptorSet() = default;

		uint32_t GetBindingCount() const;
		DescriptorSetBinding *GetBinding(uint32_t bindingIndex);
		const DescriptorSetBinding *GetBinding(uint32_t bindingIndex) const;
		std::vector<std::unique_ptr<DescriptorSetBinding>> &GetBindings();
		const std::vector<std::unique_ptr<DescriptorSetBinding>> &GetBindings() const;
		DescriptorSetBinding &SetBinding(uint32_t bindingIndex, std::unique_ptr<DescriptorSetBinding> binding);
		virtual bool Update() = 0;

		prosper::Texture *GetBoundArrayTexture(uint32_t bindingIndex, uint32_t index);
		uint32_t GetBoundArrayTextureCount(uint32_t bindingIndex) const;
		prosper::Texture *GetBoundTexture(uint32_t bindingIndex, std::optional<uint32_t> *optOutLayerIndex = nullptr);
		prosper::IImage *GetBoundImage(uint32_t bindingIndex, std::optional<uint32_t> *optOutLayerIndex = nullptr);
		prosper::IBuffer *GetBoundBuffer(uint32_t bindingIndex, uint64_t *outStartOffset = nullptr, uint64_t *outSize = nullptr);

		bool SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId);
		bool SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx);
		bool SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId);
		bool SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx);
		bool SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex, uint32_t layerId);
		bool SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex);
		bool SetBindingUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		bool SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		bool SetBindingStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		bool SetBindingDynamicStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());

		IDescriptorSetGroup &GetDescriptorSetGroup() const;

		template<class T, typename = std::enable_if_t<std::is_base_of_v<IDescriptorSet, T>>>
		T &GetAPITypeRef()
		{
			return *static_cast<T *>(m_apiTypePtr);
		}
		template<class T, typename = std::enable_if_t<std::is_base_of_v<IDescriptorSet, T>>>
		const T &GetAPITypeRef() const
		{
			return const_cast<IDescriptorSet *>(this)->GetAPITypeRef<T>();
		}

		void ReloadBinding(uint32_t bindingIdx);
	  protected:
		virtual bool DoSetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx, const std::optional<uint32_t> &layerId) = 0;
		virtual bool DoSetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx, const std::optional<uint32_t> &layerId) = 0;
		virtual bool DoSetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex, const std::optional<uint32_t> &layerId) = 0;
		virtual bool DoSetBindingUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size) = 0;
		virtual bool DoSetBindingDynamicUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size) = 0;
		virtual bool DoSetBindingStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size) = 0;
		virtual bool DoSetBindingDynamicStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset, uint64_t size) = 0;
		void *m_apiTypePtr = nullptr;
	  private:
		IDescriptorSetGroup &m_dsg;
		std::vector<std::unique_ptr<DescriptorSetBinding>> m_bindings = {};
	};

	class DLLPROSPER IDescriptorSetGroup : public ContextObject, public std::enable_shared_from_this<IDescriptorSetGroup> {
	  public:
		IDescriptorSetGroup(const IDescriptorSetGroup &) = delete;
		IDescriptorSetGroup &operator=(const IDescriptorSetGroup &) = delete;
		virtual ~IDescriptorSetGroup() override;
		IDescriptorSet *GetDescriptorSet(uint32_t index = 0);
		const IDescriptorSet *GetDescriptorSet(uint32_t index = 0) const;
		uint32_t GetBindingCount() const;

		const DescriptorSetCreateInfo &GetDescriptorSetCreateInfo() const;
	  protected:
		IDescriptorSetGroup(IPrContext &context, const DescriptorSetCreateInfo &createInfo);
		std::vector<std::shared_ptr<IDescriptorSet>> m_descriptorSets = {};
		DescriptorSetCreateInfo m_createInfo;
	};

	class SwapBuffer;
	struct DescriptorSetInfo;
	class DLLPROSPER SwapDescriptorSet : public std::enable_shared_from_this<SwapDescriptorSet> {
	  public:
		using SubDescriptorSetIndex = uint32_t;
		static std::shared_ptr<SwapDescriptorSet> Create(Window &window, std::vector<std::shared_ptr<IDescriptorSetGroup>> &&dsgs);
		static std::shared_ptr<SwapDescriptorSet> Create(Window &window, const DescriptorSetInfo &descSetInfo);
		IDescriptorSet &GetDescriptorSet(SubDescriptorSetIndex idx);
		const IDescriptorSet &GetDescriptorSet(SubDescriptorSetIndex idx) const { return const_cast<SwapDescriptorSet *>(this)->GetDescriptorSet(idx); }
		IDescriptorSet &GetDescriptorSet();
		const IDescriptorSet &GetDescriptorSet() const { return const_cast<SwapDescriptorSet *>(this)->GetDescriptorSet(); }

		void SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId);
		void SetBindingStorageImage(prosper::Texture &texture, uint32_t bindingIdx);
		void SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t layerId);
		void SetBindingTexture(prosper::Texture &texture, uint32_t bindingIdx);
		void SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex, uint32_t layerId);
		void SetBindingArrayTexture(prosper::Texture &texture, uint32_t bindingIdx, uint32_t arrayIndex);
		void SetBindingUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		void SetBindingDynamicUniformBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		void SetBindingStorageBuffer(prosper::IBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());

		void SetBindingUniformBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		void SetBindingDynamicUniformBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());
		void SetBindingStorageBuffer(prosper::SwapBuffer &buffer, uint32_t bindingIdx, uint64_t startOffset = 0ull, uint64_t size = std::numeric_limits<uint64_t>::max());

		void Update();

		IDescriptorSet *operator->();
		const IDescriptorSet *operator->() const { return const_cast<SwapDescriptorSet *>(this)->operator->(); }
		IDescriptorSet &operator*();
		const IDescriptorSet &operator*() const { return const_cast<SwapDescriptorSet *>(this)->operator*(); }
	  private:
		SwapDescriptorSet(Window &window, std::vector<std::shared_ptr<IDescriptorSetGroup>> &&dsgs);
		std::vector<std::shared_ptr<IDescriptorSetGroup>> m_dsgs;
		std::weak_ptr<Window> m_window {};
		Window *m_windowPtr = nullptr;
	};
};
#pragma warning(pop)

#endif
