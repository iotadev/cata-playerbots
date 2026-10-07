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
    else if (command == "stay")
        action = PlayerbotControlCommand::Stay;
    else if (command == "hold")
        action = PlayerbotControlCommand::Hold;
    else if (command == "attack" || command == "do attack my target")
        action = PlayerbotControlCommand::Attack;
    else if (command == "stop" || command == "cease")
        action = PlayerbotControlCommand::Cease;
    else if (command == "buff")
        action = PlayerbotControlCommand::Rebuff;
    else
        return false;
    return true;
}
inline bool ExtractPlayerbotRangeChat(std::string const& command, std::string& param)
{
    if (command == "range") { param.clear(); return true; }
    if (command.size() <= 5 || command.compare(0, 5, "range") != 0 ||
        (command[5] != ' ' && command[5] != '\t')) return false;
    param = command.substr(6);
    return true;
}
inline bool IsPlayerbotGearInspection(std::string const& command)
{
    return command == "gear" || command == "gear?" || command == "gear ?";
}
inline bool IsPlayerbotGearApply(std::string const& command) { return command == "gear apply"; }

// Group chat routing must not broaden /party into the rest of a raid.
inline bool PlayerbotControlChatReaches(bool raidChat, uint8 senderSubgroup, uint8 botSubgroup)
{
    return raidChat || senderSubgroup == botSubgroup;
}
#endif
