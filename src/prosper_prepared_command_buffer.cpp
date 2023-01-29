/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "prosper_prepared_command_buffer.hpp"
#include <shader/prosper_shader.hpp>
#include <sharedutils/util_string.h>

using namespace prosper;

prosper::util::PreparedCommand::Argument::Argument(Argument &&other) { operator=(std::move(other)); }
prosper::util::PreparedCommand::Argument::~Argument() { Clear(); }
prosper::util::PreparedCommand::Argument::Argument(const std::string &dynamicValue) : Argument {} { SetDynamicValue(dynamicValue); }
prosper::util::PreparedCommand::Argument &prosper::util::PreparedCommand::Argument::operator=(Argument &&other)
{
	Clear();
	type = other.type;
	value = other.value;

	other.type = Type::Invalid;
	other.value = nullptr;
	return *this;
}
void prosper::util::PreparedCommand::Argument::Clear()
{
	switch(type) {
	case Type::DynamicValue:
		delete static_cast<StringHash *>(value);
		break;
	case Type::StaticValue:
		delete static_cast<udm::Property *>(value);
		break;
	}
	type = Type::Invalid;
	value = nullptr;
}
void prosper::util::PreparedCommand::Argument::SetDynamicValue(const std::string &name)
{
	type = Type::DynamicValue;
	value = new StringHash {ustring::string_switch::hash(name)};
}

////////////

prosper::util::PreparedCommand::PreparedCommand(PreparedCommandFunction &&function, std::vector<Argument> &&args) : function {std::move(function)}, args {std::move(args)} {}

////////////

prosper::util::PreparedCommandArgumentMap::~PreparedCommandArgumentMap()
{
	for(auto &pair : arguments)
		delete pair.second;
}

////////////

prosper::util::PreparedCommandBufferRecordState::PreparedCommandBufferRecordState(const PreparedCommandBuffer &prepCommandBuffer, prosper::ICommandBuffer &commandBuffer, const PreparedCommandArgumentMap &drawArguments, const PreparedCommandBufferUserData &userData)
    : prepCommandBuffer {prepCommandBuffer}, commandBuffer {commandBuffer}, drawArguments {drawArguments}, userData {userData}
{
}
prosper::util::PreparedCommandBufferRecordState::~PreparedCommandBufferRecordState() {}

////////////

prosper::util::PreparedCommandBuffer::PreparedCommandBuffer() {}
void prosper::util::PreparedCommandBuffer::PushCommand(PreparedCommandFunction &&cmd, std::vector<PreparedCommand::Argument> &&args) { commands.emplace_back(std::move(cmd), std::move(args)); }
bool prosper::util::PreparedCommandBuffer::RecordCommands(prosper::ICommandBuffer &cmdBuf, const PreparedCommandArgumentMap &drawArguments, const PreparedCommandBufferUserData &userData) const
{
	PreparedCommandBufferRecordState recordState {*this, cmdBuf, drawArguments, userData};
	for(auto &cmd : commands) {
		recordState.curCommand = &cmd;
		if(!cmd.function(recordState))
			return false;
	}
	return true;
}
