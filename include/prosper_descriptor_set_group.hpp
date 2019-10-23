/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_DESCRIPTOR_SET_GROUP_HPP__
#define __PROSPER_DESCRIPTOR_SET_GROUP_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/descriptor_set_group.h>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class DescriptorSetBinding
	{
	public:
		enum class Type : uint8_t
		{
			StorageImage = 0,
			Texture,
			ArrayTexture,
			UniformBuffer,
			DynamicUniformBuffer,
			StorageBuffer
		};
		virtual ~DescriptorSetBinding()=default;
		uint32_t GetBindingIndex() const;
		DescriptorSet &GetDescriptorSet() const;
		virtual Type GetType() const=0;
	protected:
		DescriptorSetBinding(DescriptorSet &descSet,uint32_t bindingIdx);
		DescriptorSet &m_descriptorSet;
		uint32_t m_bindingIndex = 0u;
	};

	class Image;
	class DescriptorSetBindingStorageImage
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingStorageImage(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId={});
		virtual Type GetType() const override {return Type::StorageImage;}
		std::optional<uint32_t> GetLayerIndex() const;
		const std::shared_ptr<prosper::Texture> &GetTexture() const;
	private:
		std::shared_ptr<prosper::Texture> m_texture = nullptr;
		std::optional<uint32_t> m_layerId = {};
	};

	class DescriptorSetBindingTexture
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingTexture(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Texture &texture,std::optional<uint32_t> layerId={});
		virtual Type GetType() const override {return Type::Texture;}
		std::optional<uint32_t> GetLayerIndex() const;
		const std::shared_ptr<prosper::Texture> &GetTexture() const;
	private:
		std::shared_ptr<prosper::Texture> m_texture = nullptr;
		std::optional<uint32_t> m_layerId = {};
	};

	class DescriptorSetBindingArrayTexture
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingArrayTexture(DescriptorSet &descSet,uint32_t bindingIdx);
		virtual Type GetType() const override {return Type::ArrayTexture;}
		void SetArrayBinding(uint32_t arrayIndex,std::unique_ptr<DescriptorSetBindingTexture> bindingTexture);
	private:
		std::vector<std::unique_ptr<DescriptorSetBindingTexture>> m_arrayItems = {};
	};

	class Buffer;
	class DescriptorSetBindingUniformBuffer
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingUniformBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size);
		virtual Type GetType() const override {return Type::UniformBuffer;}
		uint64_t GetStartOffset() const;
		uint64_t GetSize() const;
		const std::shared_ptr<prosper::Buffer> &GetBuffer() const;
	private:
		std::shared_ptr<prosper::Buffer> m_buffer = nullptr;
		uint64_t m_startOffset = 0u;
		uint64_t m_size = 0u;
	};

	class DescriptorSetBindingDynamicUniformBuffer
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingDynamicUniformBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size);
		virtual Type GetType() const override {return Type::DynamicUniformBuffer;}
		uint64_t GetStartOffset() const;
		uint64_t GetSize() const;
		const std::shared_ptr<prosper::Buffer> &GetBuffer() const;
	private:
		std::shared_ptr<prosper::Buffer> m_buffer = nullptr;
		uint64_t m_startOffset = 0u;
		uint64_t m_size = 0u;
	};

	class DescriptorSetBindingStorageBuffer
		: public DescriptorSetBinding
	{
	public:
		DescriptorSetBindingStorageBuffer(DescriptorSet &descSet,uint32_t bindingIdx,prosper::Buffer &buffer,uint64_t startOffset,uint64_t size);
		virtual Type GetType() const override {return Type::StorageBuffer;}
		uint64_t GetStartOffset() const;
		uint64_t GetSize() const;
		const std::shared_ptr<prosper::Buffer> &GetBuffer() const;
	private:
		std::shared_ptr<prosper::Buffer> m_buffer = nullptr;
		uint64_t m_startOffset = 0u;
		uint64_t m_size = 0u;
	};

	class DescriptorSetGroup;
	class DLLPROSPER DescriptorSet
	{
	public:
		DescriptorSet(DescriptorSetGroup &dsg,Anvil::DescriptorSet &ds);
		DescriptorSet(const DescriptorSet&)=delete;
		DescriptorSet &operator=(const DescriptorSet&)=delete;

		DescriptorSetBinding *GetBinding(uint32_t bindingIndex);
		const DescriptorSetBinding *GetBinding(uint32_t bindingIndex) const;
		std::vector<std::unique_ptr<DescriptorSetBinding>> &GetBindings();
		const std::vector<std::unique_ptr<DescriptorSetBinding>> &GetBindings() const;
		DescriptorSetBinding &SetBinding(uint32_t bindingIndex,std::unique_ptr<DescriptorSetBinding> binding);

		prosper::Texture *GetBoundTexture(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex=nullptr);
		prosper::Image *GetBoundImage(uint32_t bindingIndex,std::optional<uint32_t> *optOutLayerIndex=nullptr);
		prosper::Buffer *GetBoundBuffer(uint32_t bindingIndex,uint64_t *outStartOffset=nullptr,uint64_t *outSize=nullptr);

		DescriptorSetGroup &GetDescriptorSetGroup() const;
		Anvil::DescriptorSet &GetAnvilDescriptorSet() const;
		Anvil::DescriptorSet &operator*();
		const Anvil::DescriptorSet &operator*() const;
		Anvil::DescriptorSet *operator->();
		const Anvil::DescriptorSet *operator->() const;
	private:
		DescriptorSetGroup &m_dsg;
		Anvil::DescriptorSet &m_descSet;
		std::vector<std::unique_ptr<DescriptorSetBinding>> m_bindings = {};
	};

	class DLLPROSPER DescriptorSetGroup
		: public ContextObject,
		public std::enable_shared_from_this<DescriptorSetGroup>
	{
	public:
		static std::shared_ptr<DescriptorSetGroup> Create(Context &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg,const std::function<void(DescriptorSetGroup&)> &onDestroyedCallback=nullptr);
		virtual ~DescriptorSetGroup() override;
		DescriptorSet *GetDescriptorSet(uint32_t index=0);
		const DescriptorSet *GetDescriptorSet(uint32_t index=0) const;

		Anvil::DescriptorSetGroup &GetAnvilDescriptorSetGroup() const;
		Anvil::DescriptorSetGroup &operator*();
		const Anvil::DescriptorSetGroup &operator*() const;
		Anvil::DescriptorSetGroup *operator->();
		const Anvil::DescriptorSetGroup *operator->() const;
	protected:
		DescriptorSetGroup(Context &context,std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> dsg);
		std::unique_ptr<Anvil::DescriptorSetGroup,std::function<void(Anvil::DescriptorSetGroup*)>> m_descriptorSetGroup = nullptr;
		std::vector<std::shared_ptr<DescriptorSet>> m_descriptorSets = {};
	};
};
#pragma warning(pop)

#endif
