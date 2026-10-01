/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotAddonBridge.h"
#include "PlayerbotAddonProtocol.h"
#include "PlayerbotAddonLifecycle.h"
#include "PlayerbotAddonRoster.h"
#include "PlayerbotManagedControl.h"
#include "PlayerbotManagedRoster.h"
#include "PlayerbotRoster.h"
#include "Chat.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <sstream>
#include <chrono>
#include <map>
#include <algorithm>

namespace
{
bool BridgeEnabled = false;
struct RequesterGuard
{
    PlayerbotAddonProtocol::MutationGuard Guard;
    PlayerbotAddonProtocol::RosterQueryGuard RosterGuard;
    uint64 LastUsed = 0;
};
// World-thread requests only. Account keys and timestamps, never Player/session
// pointers. Inactive entries expire after the replay retention has elapsed.
std::map<uint32, RequesterGuard> MutationGuards;

uint64 NowMs()
{
    return uint64(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

RequesterGuard* GetRequesterGuard(Player& requester, uint64 now)
{
    for (auto it = MutationGuards.begin(); it != MutationGuards.end();)
        if (now - it->second.LastUsed >= 600000)
            it = MutationGuards.erase(it);
        else
            ++it;
    uint32 accountId = requester.GetSession()->GetAccountId();
    auto it = MutationGuards.find(accountId);
    if (it == MutationGuards.end())
    {
        if (MutationGuards.size() >= 256)
            return nullptr;
        it = MutationGuards.emplace(accountId, RequesterGuard{}).first;
    }
    it->second.LastUsed = now;
    return &it->second;
}

char const* AdmitMutation(Player& requester, std::string const& token)
{
    uint64 now = NowMs();
    RequesterGuard* guard = GetRequesterGuard(requester, now);
    return guard ? guard->Guard.Admit(token, now) : "REQUESTER_LIMIT";
}

void Reply(Player& requester, std::string const& message)
{
    if (message.size() > PlayerbotAddonProtocol::MaxMessageBytes)
        return;
    WorldPacket packet;
    ChatHandler::BuildChatPacket(packet, CHAT_MSG_WHISPER, LANG_ADDON, &requester, &requester,
        message, 0U, "", DEFAULT_LOCALE, "MBOT");
    requester.SendDirectMessage(&packet);
}

uint32 Percent(uint32 value, uint32 maximum)
{
    return maximum ? uint32(uint64(value) * 100 / maximum) : 0;
}
}

void SetPlayerbotAddonBridgeEnabled(bool enabled)
{
    BridgeEnabled = enabled;
}

bool HandlePlayerbotAddonMessage(Player& sender, std::string const& prefix, std::string const& message)
{
    if (!BridgeEnabled || prefix != "MBOT" || !sender.IsInWorld() ||
        !sender.GetSession() || sender.GetSession()->IsServerOrigin())
        return false;

    auto request = PlayerbotAddonProtocol::Parse(message);
    switch (request.Kind)
    {
        case PlayerbotAddonProtocol::Request::Hello:
            Reply(sender, "HELLO_ACK~1~cata-playerbots");
            // Capability is service availability, not permission for this
            // requester. Every roster/mutation/poll still rechecks access.
            {
                std::string capabilities = PlayerbotAddonProtocol::ManagedCapabilities(
                    PlayerbotManagedRoster::IsPlayerControlEnabled());
                Reply(sender, capabilities.empty() ? "CAPS" : "CAPS~" + capabilities);
            }
            break;
        case PlayerbotAddonProtocol::Request::Ping:
            Reply(sender, "PONG~" + request.Payload);
            break;
        case PlayerbotAddonProtocol::Request::AltRoster:
        {
            uint64 now = NowMs();
            RequesterGuard* guard = GetRequesterGuard(sender, now);
            if (!guard || !guard->RosterGuard.Admit(now))
            {
                Reply(sender, "ERR~GET~ALT_ROSTER~~RATE_LIMIT");
                break;
            }
            // Use configured identities and trusted links, not the donor's
            // same-account SQL scan: Cata has one native session per account.
            auto entries = PlayerbotManagedControl::ListFor(sender);
            std::sort(entries.begin(), entries.end(), [](auto const& a, auto const& b)
            {
                return a.Guid.GetCounter() < b.Guid.GetCounter();
            });
            std::vector<PlayerbotAddonProtocol::ManagedRosterRow> rows;
            rows.reserve(entries.size());
            for (auto const& entry : entries)
                rows.push_back({ entry.Guid.GetCounter(), entry.Name, entry.Class, entry.Level,
                    PlayerbotAddonProtocol::RosterOnline(entry.Receipt.get()) });
            for (auto const& message : PlayerbotAddonProtocol::FrameManagedRoster(rows))
                Reply(sender, message);
            break;
        }
        case PlayerbotAddonProtocol::Request::Connect:
        case PlayerbotAddonProtocol::Request::Disconnect:
        case PlayerbotAddonProtocol::Request::LifecycleState:
        {
            using Request = PlayerbotAddonProtocol::Request;
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(request.GuidLow);
            std::string identity = request.Payload + "~" + std::to_string(request.GuidLow) + "~";
            auto entries = PlayerbotManagedControl::ListFor(sender);
            PlayerbotManagedRosterEntry const* visible = nullptr;
            for (auto const& entry : entries)
                if (entry.Guid == guid)
                {
                    visible = &entry;
                    break;
                }
            if (request.Kind == Request::LifecycleState)
            {
                // Reauthorize every poll. Do not expose names or receipt state
                // after link revocation or another party taking control.
                if (!visible)
                    Reply(sender, "BOT_LIFECYCLE_STATE~" + identity + "~OFFLINE~FORBIDDEN");
                else
                {
                    auto view = PlayerbotAddonProtocol::View(visible->Receipt.get());
                    Reply(sender, "BOT_LIFECYCLE_STATE~" + identity + PlayerbotAddonProtocol::EncodeField(visible->Name) +
                        "~" + view.State + "~" + view.Reason);
                }
                break;
            }
            std::string action = request.Kind == Request::Connect ? "CONNECT" : "DISCONNECT";
            if (char const* rejection = AdmitMutation(sender, request.Payload))
            {
                Reply(sender, "BOT_LIFECYCLE~" + identity + "~" + action + "~ERR~" + rejection);
                break;
            }
            // All admission/ownership/native lifecycle decisions remain in
            // the shared service, never in client-supplied fields.
            auto result = request.Kind == Request::Connect ? PlayerbotManagedControl::Start(sender, guid) :
                PlayerbotManagedControl::Stop(sender, guid);
            char const* status = "ERR";
            char const* reason = "REJECTED";
            switch (result.Result)
            {
                case PlayerbotManagedResult::Accepted: status = "PENDING"; reason = "STARTED"; break;
                case PlayerbotManagedResult::AlreadyOffline: status = "OK"; reason = "ALREADY_OFFLINE"; break;
                case PlayerbotManagedResult::Disabled: reason = "DISABLED"; break;
                case PlayerbotManagedResult::NotConfigured: reason = "NOT_CONFIGURED"; break;
                case PlayerbotManagedResult::Unauthorized: reason = "FORBIDDEN"; break;
                case PlayerbotManagedResult::AdmissionRejected: reason = "ADMISSION_REJECTED"; break;
            }
            Reply(sender, "BOT_LIFECYCLE~" + identity +
                (visible ? PlayerbotAddonProtocol::EncodeField(visible->Name) : "") +
                "~" + action + "~" + status + "~" + reason);
            break;
        }
        case PlayerbotAddonProtocol::Request::Roster:
        {
            std::ostringstream payload;
            bool first = true;
            for (auto const& entry : PlayerbotRoster::ListActiveFor(sender))
            {
                WorldSession* session = sWorld->FindServerOriginPlayerbot(entry.Guid);
                Player* bot = session ? session->GetPlayer() : nullptr;
                if (!bot || !bot->IsInWorld())
                    continue;
                if (!first)
                    payload << ';';
                first = false;
                payload << entry.Name << ',' << uint32(entry.Class) << ',' << uint32(entry.Level)
                    << ',' << bot->GetMapId() << ',' << (bot->IsAlive() ? '1' : '0') << ','
                    << Percent(bot->GetHealth(), bot->GetMaxHealth()) << ','
                    << Percent(bot->GetPower(POWER_MANA), bot->GetMaxPower(POWER_MANA));
                if (payload.tellp() > std::streamoff(PlayerbotAddonProtocol::MaxMessageBytes - 7))
                {
                    // Do not let a truncated roster look like a complete one.
                    Reply(sender, "ERR~GET~ROSTER~~ROSTER_TOO_LARGE");
                    return true;
                }
            }
            std::string rows = payload.str();
            Reply(sender, rows.empty() ? "ROSTER" : "ROSTER~" + rows);
            break;
        }
        case PlayerbotAddonProtocol::Request::Invalid:
            Reply(sender, std::string("ERR~~~~") + request.Error);
            break;
    }
    return true;
}
