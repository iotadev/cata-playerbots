/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotControl.h"
#include "PlayerbotControlChat.h"
#include "PlayerbotRoster.h"
#include "Chat.h"
#include "Group.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"
#include "World.h"

namespace
{
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

        std::string command = NormalizePlayerbotControlChat(message);
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
        if (!ParsePlayerbotControlChat(command, action))
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

    void OnChat(Player* sender, uint32 type, uint32 lang, std::string& message, Group* group) override
    {
        bool partyChat = type == CHAT_MSG_PARTY || type == CHAT_MSG_PARTY_LEADER;
        bool raidChat = type == CHAT_MSG_RAID || type == CHAT_MSG_RAID_LEADER;
        if ((!partyChat && !raidChat) || lang == LANG_ADDON || !sender || !group ||
            group->isBGGroup() || !group->IsMember(sender->GetGUID()) || !sender->IsInWorld() ||
            !sender->GetSession() || sender->GetSession()->IsServerOrigin())
            return;

        std::string command = NormalizePlayerbotControlChat(message);
        PlayerbotControlCommand action;
        if (!ParsePlayerbotControlChat(command, action))
            return;
        uint32 queued = 0;
        uint32 rejected = 0;
        for (auto const& entry : PlayerbotRoster::ListActiveFor(*sender))
        {
            WorldSession* session = sWorld->FindServerOriginPlayerbot(entry.Guid);
            Player* bot = session ? session->GetPlayer() : nullptr;
            if (!bot || bot->GetGroup() != group ||
                !PlayerbotControlChatReaches(raidChat, group->GetMemberGroup(sender->GetGUID()),
                    group->GetMemberGroup(entry.Guid)))
                continue;
            // Dispatch independently rechecks native identity and full control.
            if (PlayerbotControl::Dispatch(*sender, entry.Guid, action) == PlayerbotControlResult::Queued)
                ++queued;
            else
                ++rejected;
        }
        if (queued || rejected)
            ChatHandler(sender->GetSession()).PSendSysMessage("Playerbots: %s requested for %u bot(s); %u rejected.",
                command.c_str(), queued, rejected);
        // Preserve normal chat delivery, including to human group members.
    }
};
}

void AddSC_playerbot_chat_commands()
{
    new PlayerbotChatCommands();
}
