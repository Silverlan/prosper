/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_COMMAND_BUFFER_HPP__
#define __PROSPER_COMMAND_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <wrappers/command_buffer.h>

#undef max

#pragma warning(push)
#pragma warning(disable : 4251)
namespace prosper
{
	class Framebuffer;
	class RenderPass;
	class DLLPROSPER CommandBuffer
		: public ContextObject,
		public std::enable_shared_from_this<CommandBuffer>
	{
	public:
		virtual ~CommandBuffer() override;
		Anvil::CommandBufferBase &GetAnvilCommandBuffer() const;
		Anvil::CommandBufferBase &operator*();
		const Anvil::CommandBufferBase &operator*() const;
		Anvil::CommandBufferBase *operator->();
		const Anvil::CommandBufferBase *operator->() const;

		virtual bool IsPrimary() const;
		virtual bool IsSecondary() const;
		Anvil::QueueFamilyType GetQueueFamilyType() const;
		bool Reset(bool shouldReleaseResources) const;

		bool StopRecording() const;
	protected:
		CommandBuffer(Context &context,const std::shared_ptr<Anvil::CommandBufferBase> &cmdBuffer,Anvil::QueueFamilyType queueFamilyType);
		std::shared_ptr<Anvil::CommandBufferBase> m_cmdBuffer = nullptr;
		Anvil::QueueFamilyType m_queueFamilyType = Anvil::QueueFamilyType::FIRST;
	};

	///////////////////

	class DLLPROSPER PrimaryCommandBuffer
		: public CommandBuffer
	{
	public:
		static std::shared_ptr<PrimaryCommandBuffer> Create(Context &context,std::unique_ptr<Anvil::PrimaryCommandBuffer,std::function<void(Anvil::PrimaryCommandBuffer*)>> cmdBuffer,Anvil::QueueFamilyType queueFamilyType,const std::function<void(CommandBuffer&)> &onDestroyedCallback=nullptr);
		virtual bool IsPrimary() const override;

		Anvil::PrimaryCommandBuffer &GetAnvilCommandBuffer() const;
		Anvil::PrimaryCommandBuffer &operator*();
		const Anvil::PrimaryCommandBuffer &operator*() const;
		Anvil::PrimaryCommandBuffer *operator->();
		const Anvil::PrimaryCommandBuffer *operator->() const;

		bool StartRecording(bool oneTimeSubmit=true,bool simultaneousUseAllowed=false) const;
	protected:
		PrimaryCommandBuffer(Context &context,std::unique_ptr<Anvil::PrimaryCommandBuffer,std::function<void(Anvil::PrimaryCommandBuffer*)>> cmdBuffer,Anvil::QueueFamilyType queueFamilyType);
	};

	///////////////////

	class DLLPROSPER SecondaryCommandBuffer
		: public CommandBuffer
	{
	public:
		static std::shared_ptr<SecondaryCommandBuffer> Create(Context &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,Anvil::QueueFamilyType queueFamilyType,const std::function<void(CommandBuffer&)> &onDestroyedCallback=nullptr);
		virtual bool IsSecondary() const override;

		Anvil::SecondaryCommandBuffer &GetAnvilCommandBuffer() const;
		Anvil::SecondaryCommandBuffer &operator*();
		const Anvil::SecondaryCommandBuffer &operator*() const;
		Anvil::SecondaryCommandBuffer *operator->();
		const Anvil::SecondaryCommandBuffer *operator->() const;

		bool StartRecording(
			bool oneTimeSubmit,bool simultaneousUseAllowed,bool renderPassUsageOnly,
			const Framebuffer &framebuffer,const RenderPass &rp,Anvil::SubPassID subPassId,
			Anvil::OcclusionQuerySupportScope occlusionQuerySupportScope,bool occlusionQueryUsedByPrimaryCommandBuffer,
			Anvil::QueryPipelineStatisticFlags statisticsFlags
		) const;
	protected:
		SecondaryCommandBuffer(Context &context,std::unique_ptr<Anvil::SecondaryCommandBuffer,std::function<void(Anvil::SecondaryCommandBuffer*)>> cmdBuffer,Anvil::QueueFamilyType queueFamilyType);
	};
};
#pragma warning(pop)

#endif
