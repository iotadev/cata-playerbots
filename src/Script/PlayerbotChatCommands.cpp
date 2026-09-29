/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotControl.h"
#include "PlayerbotRoster.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include <algorithm>
#include <cctype>

namespace
{
std::string NormalizeCommand(std::string const& text)
{
    std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    std::size_t last = text.find_last_not_of(" \t\r\n");
    std::string command = text.substr(first, last - first + 1);
    std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return command;
}

class PlayerbotChatCommands final : public PlayerScript
{
public:
    PlayerbotChatCommands() : PlayerScript("PlayerbotChatCommands") { }

    void OnChat(Player* sender, uint32 type, uint32 lang, std::string& message, Player* receiver) override
    {
        if (type != CHAT_MSG_WHISPER || lang == LANG_ADDON || !sender || !receiver ||
            !sender->GetSession() || sender->GetSession()->IsServerOrigin() ||
            !receiver->GetSession() || !receiver->GetSession()->IsServerOrigin())
            return;

        std::string command = NormalizeCommand(message);
        ChatHandler reply(sender->GetSession());
        if (command == "list")
        {
            std::vector<PlayerbotRosterEntry> roster = PlayerbotRoster::ListActiveFor(*sender);
            if (roster.empty())
                reply.SendSysMessage("No controllable Playerbots are currently online.");
            for (PlayerbotRosterEntry const& entry : roster)
                reply.PSendSysMessage("Playerbot: %s (level %u, class %u).", entry.Name.c_str(),
                    uint32(entry.Level), uint32(entry.Class));
            return;
        }

        PlayerbotControlCommand action;
        if (command == "follow")
            action = PlayerbotControlCommand::Follow;
        else if (command == "stay" || command == "hold")
            action = PlayerbotControlCommand::Hold;
        else if (command == "attack")
            action = PlayerbotControlCommand::Attack;
        else if (command == "stop" || command == "cease")
            action = PlayerbotControlCommand::Cease;
        else
            return;

        switch (PlayerbotControl::Dispatch(*sender, receiver->GetGUID(), action))
        {
            case PlayerbotControlResult::Queued:
                reply.PSendSysMessage("Playerbot %s: %s requested.", receiver->GetName().c_str(), command.c_str());
                break;
            case PlayerbotControlResult::Unauthorized:
                reply.SendSysMessage("You do not control that Playerbot.");
                break;
            case PlayerbotControlResult::NotFollowing:
                reply.SendSysMessage("That Playerbot must follow you before it can attack your target.");
                break;
            case PlayerbotControlResult::BotUnavailable:
                reply.SendSysMessage("That Playerbot is unavailable.");
                break;
        }
    }
};
}

void AddSC_playerbot_chat_commands()
{
    new PlayerbotChatCommands();
}
