/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_CONTROL_CHAT_H
#define CATA_PLAYERBOTS_CONTROL_CHAT_H

#include "PlayerbotControl.h"
#include <algorithm>
#include <cctype>
#include <string>

inline std::string NormalizePlayerbotControlChat(std::string const& text)
{
    if (text.size() > 256)
        return {};
    std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    std::size_t last = text.find_last_not_of(" \t\r\n");
    std::string command = text.substr(first, last - first + 1);
    std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return command;
}

inline bool ParsePlayerbotControlChat(std::string const& command, PlayerbotControlCommand& action)
{
    if (command == "follow")
        action = PlayerbotControlCommand::Follow;
    else if (command == "stay" || command == "hold")
        action = PlayerbotControlCommand::Hold;
    else if (command == "attack" || command == "do attack my target")
        action = PlayerbotControlCommand::Attack;
    else if (command == "stop" || command == "cease")
        action = PlayerbotControlCommand::Cease;
    else
        return false;
    return true;
}

// Group chat routing must not broaden /party into the rest of a raid.
inline bool PlayerbotControlChatReaches(bool raidChat, uint8 senderSubgroup, uint8 botSubgroup)
{
    return raidChat || senderSubgroup == botSubgroup;
}
#endif
