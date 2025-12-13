// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.prosper;

import :prepared_command_buffer;
import :shader_system.shader;

using namespace prosper;

util::PreparedCommand::Argument::Argument(Argument &&other) { operator=(std::move(other)); }
util::PreparedCommand::Argument::~Argument() { Clear(); }
util::PreparedCommand::Argument::Argument(const std::string &dynamicValue) : Argument {} { SetDynamicValue(dynamicValue); }
util::PreparedCommand::Argument &util::PreparedCommand::Argument::operator=(Argument &&other)
{
	Clear();
	type = other.type;
	value = other.value;

	other.type = Type::Invalid;
	other.value = nullptr;
	return *this;
}
void util::PreparedCommand::Argument::Clear()
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
void util::PreparedCommand::Argument::SetDynamicValue(const std::string &name)
{
	type = Type::DynamicValue;
	value = new StringHash {pragma::string::string_switch::hash(name)};
}

////////////

util::PreparedCommand::PreparedCommand(PreparedCommandFunction &&function, std::vector<Argument> &&args) : function {std::move(function)}, args {std::move(args)} {}

////////////

util::PreparedCommandArgumentMap::~PreparedCommandArgumentMap()
{
	for(auto &pair : arguments)
		delete pair.second;
}

////////////

util::PreparedCommandBufferRecordState::PreparedCommandBufferRecordState(const PreparedCommandBuffer &prepCommandBuffer, ICommandBuffer &commandBuffer, const PreparedCommandArgumentMap &drawArguments, const PreparedCommandBufferUserData &userData)
    : prepCommandBuffer {prepCommandBuffer}, commandBuffer {commandBuffer}, drawArguments {drawArguments}, userData {userData}
{
}
util::PreparedCommandBufferRecordState::~PreparedCommandBufferRecordState() {}

////////////

util::PreparedCommandBuffer::PreparedCommandBuffer() {}
void util::PreparedCommandBuffer::PushCommand(PreparedCommandFunction &&cmd, std::vector<PreparedCommand::Argument> &&args) { commands.emplace_back(std::move(cmd), std::move(args)); }
bool util::PreparedCommandBuffer::RecordCommands(ICommandBuffer &cmdBuf, const PreparedCommandArgumentMap &drawArguments, const PreparedCommandBufferUserData &userData) const
{
	PreparedCommandBufferRecordState recordState {*this, cmdBuf, drawArguments, userData};
	for(auto &cmd : commands) {
		recordState.curCommand = &cmd;
		if(!cmd.function(recordState))
			return false;
	}
	return true;
}
void util::PreparedCommandBuffer::Reset()
{
	commands.clear();
	dynamicArguments = {};
	enableDrawArgs = true;
}
