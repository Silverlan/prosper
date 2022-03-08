/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PROSPER_PREPARED_COMMAND_BUFFER_HPP__
#define __PROSPER_PREPARED_COMMAND_BUFFER_HPP__

#include "prosper_definitions.hpp"
#include "prosper_includes.hpp"
#include "prosper_context_object.hpp"
#include <sharedutils/util_string.h>
#include <udm.hpp>

namespace prosper {struct ShaderBindState;};
namespace prosper::util
{
	class PreparedCommandBuffer;
	struct PreparedCommand;
	struct PreparedCommandArgumentMap;
	struct DLLPROSPER PreparedCommandBufferUserData
	{
		using StringHash = uint32_t;
		template<typename T>
			void Set(StringHash nameHash,T &value)
		{
			userData[nameHash] = &value;
		}
		template<typename T>
			T &Get(StringHash nameHash) const
		{
			auto it = userData.find(nameHash);
			if(it == userData.end())
				throw std::runtime_error{"User data has not been set!"};
			return *static_cast<T*>(it->second);
		}
	private:
		mutable std::unordered_map<StringHash,void*> userData;
	};
	struct DLLPROSPER PreparedCommandBufferRecordState
	{
		PreparedCommandBufferRecordState(
			const PreparedCommandBuffer &prepCommandBuffer,
			prosper::ICommandBuffer &commandBuffer,
			const PreparedCommandArgumentMap &drawArguments,
			const PreparedCommandBufferUserData &userData
		);
		~PreparedCommandBufferRecordState();
		const PreparedCommandBuffer &prepCommandBuffer;
		prosper::ICommandBuffer &commandBuffer;
		const PreparedCommandArgumentMap &drawArguments;
		const PreparedCommandBufferUserData &userData;
		const PreparedCommand *curCommand;
		mutable std::unique_ptr<prosper::ShaderBindState> shaderBindState;
		template<typename T>
			T GetArgument(uint32_t idx) const;
	};
	using PreparedCommandFunction = std::function<bool(const PreparedCommandBufferRecordState&)>;
	struct DLLPROSPER PreparedCommand
	{
		struct DLLPROSPER Argument
		{
			using StringHash = uint32_t;
			enum class Type : uint8_t
			{
				StaticValue = 0,
				DynamicValue,
				Invalid = std::numeric_limits<uint8_t>::max()
			};
			Argument()=default;
			Argument(const std::string &dynamicValue);
			Argument(const Argument&)=delete;
			Argument(Argument &&other);
			~Argument();
			Argument &operator=(const Argument&)=delete;
			Argument &operator=(Argument &&other);
			void Clear();
			template<typename T>
				void GetStaticValue(T &outValue) const;
			template<typename T>
				void SetStaticValue(T &&arg);
			void SetDynamicValue(const std::string &name);
			Type type = Type::Invalid;
			void *value = nullptr;
		};
		PreparedCommand(PreparedCommandFunction &&function,std::vector<Argument> &&args);
		PreparedCommand(const PreparedCommand&)=delete;
		PreparedCommand(PreparedCommand&&)=default;
		PreparedCommand &operator=(const PreparedCommand&)=delete;
		PreparedCommand &operator=(PreparedCommand &&)=default;
		PreparedCommandFunction function;
		std::vector<Argument> args;
	};
	struct DLLPROSPER PreparedCommandArgumentMap
	{
		PreparedCommandArgumentMap()=default;
		~PreparedCommandArgumentMap();
		std::unordered_map<PreparedCommand::Argument::StringHash,udm::Property*> arguments;
		template<typename T>
			void SetArgumentValue(const std::string &name,T &&value)
		{
			auto hash = ustring::string_switch::hash(name);
			SetArgumentValue<T>(hash,std::move(value));
		}
		template<typename T>
			void SetArgumentValue(PreparedCommand::Argument::StringHash hash,T &&value)
		{
			auto it = arguments.find(hash);
			if(it == arguments.end())
			{
				auto *prop = new udm::Property{};
				udm::Property::Construct<T>(*prop);
				it = arguments.insert(std::make_pair(hash,prop)).first;
			}
			udm::Property::Construct<T>(*it->second,std::forward<T>(value));
		}
		template<typename T>
			bool GetArgumentValue(const std::string &name,T &outValue) const
		{
			return GetArgumentValue<T>(name,std::move(outValue));
		}
		template<typename T>
			bool GetArgumentValue(PreparedCommand::Argument::StringHash hash,T &outValue) const
		{
			auto it = arguments.find(hash);
			if(it == arguments.end())
				return false;
			auto val = it->second->ToValue<T>();
			if(!val.has_value())
				throw std::runtime_error{"Dynamic argument value is set, but uses incompatible type!"};
			outValue = *val;
			return true;
		}
	};
	class DLLPROSPER PreparedCommandBuffer
	{
	public:
		PreparedCommandBuffer();
		PreparedCommandBuffer(PreparedCommandBuffer&&)=delete;
		PreparedCommandBuffer(const PreparedCommandBuffer&)=delete;
		PreparedCommandBuffer &operator=(PreparedCommandBuffer&&)=delete;
		PreparedCommandBuffer &operator=(const PreparedCommandBuffer&)=delete;
		template<typename T>
			void GetArgumentValue(const PreparedCommand::Argument &arg,const PreparedCommandArgumentMap &drawArguments,T &outValue) const
		{
			if(arg.type == PreparedCommand::Argument::Type::DynamicValue)
			{
				auto &nameHash = *static_cast<PreparedCommand::Argument::StringHash*>(arg.value);
				auto res = drawArguments.GetArgumentValue<T>(nameHash,outValue);
				if(res)
					return;
				dynamicArguments.GetArgumentValue<T>(nameHash,outValue);
				return;
			}
			arg.GetStaticValue<T>(outValue);
		}
		template<typename T>
			void SetDynamicArgument(const std::string &name,T &&value)
		{
			dynamicArguments.SetArgumentValue<T>(name,std::move(value));
		}
		void PushCommand(PreparedCommandFunction &&cmd,std::vector<PreparedCommand::Argument> &&args={});
		bool RecordCommands(prosper::ICommandBuffer &cmdBuf,const PreparedCommandArgumentMap &drawArguments,const PreparedCommandBufferUserData &userData) const;
		std::vector<PreparedCommand> commands;
		PreparedCommandArgumentMap dynamicArguments;
		bool enableDrawArgs = true;
	};

	template<typename T>
		void PreparedCommand::Argument::GetStaticValue(T &outValue) const
	{
		switch(type)
		{
		case Type::StaticValue:
		{
			auto val = static_cast<udm::Property*>(value)->ToValue<T>();
			if(!val.has_value())
				throw std::runtime_error{"Static argument value is set, but uses incompatible type!"};
			outValue = *val;
			return;
		}
		}
		throw std::runtime_error{"No static argument value has been specified!"};
	}
	template<typename T>
		void PreparedCommand::Argument::SetStaticValue(T &&arg)
	{
		type = Type::StaticValue;
		auto *prop = new udm::Property{};
		udm::Property::Construct(*prop,udm::type_to_enum<T>());
		*prop = arg;
		value = prop;
	}

	template<typename T>
		T PreparedCommandBufferRecordState::GetArgument(uint32_t idx) const
	{
		T value;
		prepCommandBuffer.GetArgumentValue(curCommand->args[idx],drawArguments,value);
		return value;
	}
};

#endif
