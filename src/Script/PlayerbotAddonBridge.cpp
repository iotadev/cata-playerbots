/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotAddonBridge.h"
#include "PlayerbotAddonProtocol.h"
#include "PlayerbotAddonLifecycle.h"
#include "PlayerbotAddonRoster.h"
#include "../Bot/Cmd/PlayerbotAddonState.h"
#include "../Bot/Cmd/PlayerbotAddonMutation.h"
#include "../Bot/Cmd/PlayerbotStrategyControl.h"
#include "../Bot/Cmd/PlayerbotStrategyCompletions.h"
#include "../Bot/Cmd/PlayerbotStrategyPending.h"
#include "PlayerbotControl.h"
#include "PlayerbotConfig.h"
#include "Timer.h"
#include "PlayerbotManagedControl.h"
#include "PlayerbotManagedRoster.h"
#include "PlayerbotRoster.h"
#include "Chat.h"
#include "Player.h"
#include "Group.h"
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
    PlayerbotAddonProtocol::RosterQueryGuard StateGuard;
    uint64 LastUsed = 0;
};
// World-thread requests only. Account keys and timestamps, never Player/session
// pointers. Inactive entries expire after the replay retention has elapsed.
std::map<uint32, RequesterGuard> MutationGuards;
PlayerbotAddonProtocol::StrategyPending PendingStrategies; // World thread only; no native pointers.

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

Player* CurrentRequester(uint32 guid, PlayerbotStrategyBinding const& binding)
{
    WorldSession* session = sWorld->FindSession(binding.Account);
    Player* player = session ? session->GetPlayer() : nullptr;
    return session && !session->IsServerOrigin() && !session->isLogingOut() && player && player->IsInWorld() &&
        player->GetGUID().GetCounter() == guid && binding.SessionMatches(session->GetAccountId(), session->GetPlayerbotRequestIdentity()) ? player : nullptr;
}

char const* DispatchGroupStrategy(Player& sender, PlayerbotAddonProtocol::StrategyMutation const& mutation, std::string const& command)
{
    if (!PlayerbotModuleGroupStrategyMutationEnabled()) return "DISABLED_SCOPE";
    uint32 account = sender.GetSession()->GetAccountId();
    if (PendingStrategies.Busy(account)) return "BUSY";
    PlayerbotStrategyBinding binding;
    binding.Account = account;
    binding.Scope = mutation.Scope;
    binding.Session = sender.GetSession()->GetPlayerbotRequestIdentity();
    Group* group = sender.GetGroup();
    if (mutation.Scope != "ALL") binding.Group = group ? group->GetGUID().GetRawValue() : 0;
    // No group is a valid empty scope, not permission to fall back to ALL.
    if (mutation.Scope != "ALL" && (!group || (mutation.Scope == "RAID" && !group->isRaidGroup())))
        return "NO_MATCH";
    if (mutation.Scope != "ALL" && !group->IsMember(sender.GetGUID())) return "NO_MATCH";
    std::vector<uint32> bots;
    for (auto const& entry : PlayerbotRoster::ListActiveFor(sender))
    {
        WorldSession* session = sWorld->FindServerOriginPlayerbot(entry.Guid);
        Player* bot = session ? session->GetPlayer() : nullptr;
        if (!bot || (session->GetServerOriginFollowTargetGuidLow() != sender.GetGUID().GetCounter() &&
            session->GetServerOriginPartyControllerGuidLow() != sender.GetGUID().GetCounter())) continue;
        Group* botGroup = bot->GetGroup();
        if (mutation.Scope != "ALL" && (!botGroup || !botGroup->IsMember(entry.Guid))) continue;
        if (!binding.GroupMatches(group ? group->GetGUID().GetRawValue() : 0,
            botGroup ? botGroup->GetGUID().GetRawValue() : 0, group && group->isRaidGroup())) continue;
        if (bots.size() >= 128) return "BOT_LIMIT"; // Freeze/preflight before any post; no partial over-limit execution.
        bots.push_back(entry.Guid.GetCounter());
    }
    uint32 now = getMSTime();
    auto generation = PendingStrategies.Begin(sender.GetGUID().GetCounter(), binding, mutation, bots, now);
    if (!generation) return "BATCH_LIMIT";
    binding = PendingStrategies.Entries().at(*generation).Binding;
    for (uint32 bot : bots)
    {
        auto result = PlayerbotControl::DispatchStrategy(sender, ObjectGuid::Create<HighGuid::Player>(bot),
            command, mutation.Token, {}, *generation, binding);
        if (result != PlayerbotControlResult::Queued)
            PendingStrategies.Record({*generation, sender.GetGUID().GetCounter(), bot, mutation.Token, mutation.State,
                PlayerbotAddonProtocol::StrategyBatch::Stage::AdmissionRejected, false}, now);
    }
    return nullptr; // Only the world pump emits the single aggregate completion ACK.
}
}

void SetPlayerbotAddonBridgeEnabled(bool enabled)
{
    BridgeEnabled = enabled;
}

void UpdatePlayerbotAddonStrategyBatches()
{
    uint32 now = getMSTime();
    std::vector<uint64> abandoned;
    for (auto const& [id, entry] : PendingStrategies.Entries())
    {
        Player* requester = CurrentRequester(entry.Requester, entry.Binding);
        if (!requester || !BridgeEnabled) { abandoned.push_back(id); continue; }
        Group* group = requester->GetGroup();
        if (!PlayerbotModuleStrategyControlEnabled() || !PlayerbotModuleStrategyMutationEnabled() ||
            !PlayerbotModuleGroupStrategyMutationEnabled() || requester->IsBeingTeleported() ||
            !entry.Binding.GroupMatches(group ? group->GetGUID().GetRawValue() : 0,
                group ? group->GetGUID().GetRawValue() : 0, group && group->isRaidGroup()))
            PendingStrategies.Cancel(id); // Stop unexecuted work; executed work is not rolled back.
    }
    for (uint64 id : abandoned) PendingStrategies.Abandon(id);
    for (auto const& result : PlayerbotAddonProtocol::GroupStrategyCompletions().Drain()) PendingStrategies.Record(result, now);
    for (auto const& reply : PendingStrategies.Poll(now))
        if (Player* requester = CurrentRequester(reply.Requester, reply.Binding))
            if (BridgeEnabled && !requester->IsBeingTeleported()) Reply(*requester, reply.Message);
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
                    PlayerbotManagedRoster::IsPlayerControlEnabled(), PlayerbotModuleStrategyControlEnabled(), PlayerbotModuleStrategyMutationEnabled());
                Reply(sender, capabilities.empty() ? "CAPS" : "CAPS~" + capabilities);
            }
            break;
        case PlayerbotAddonProtocol::Request::Ping:
            Reply(sender, "PONG~" + request.Payload);
            break;
        case PlayerbotAddonProtocol::Request::Strategy:
        {
            auto mutation = PlayerbotAddonProtocol::ParseStrategyMutation(request.Payload);
            if (!mutation) { Reply(sender, "ERR~RUN~STRATEGY~~BAD_FIELDS"); break; }
            auto ack = [&](unsigned matched, char const* reason)
            { Reply(sender, PlayerbotAddonProtocol::StrategyAck(*mutation, matched, 0, matched, reason)); };
            // Validate the reply envelope before admitting a mutation. Never
            // replace a too-long target with an identity the client did not request.
            if (PlayerbotAddonProtocol::StrategyAck(*mutation, 1, 0, 1, "UNSUPPORTED_STRATEGY").empty())
            { Reply(sender, "ERR~RUN~STRATEGY~~ACK_TOO_LONG"); break; }
            if (!PlayerbotModuleStrategyMutationEnabled()) { ack(0, "DISABLED"); break; }
            if (char const* rejection = AdmitMutation(sender, mutation->Token)) { ack(0, rejection); break; }
            if (sender.IsBeingTeleported() || sender.GetSession()->isLogingOut()) { ack(0, "BUSY"); break; }
            std::string command = (mutation->State == "C" ? "co " : "nc ") + mutation->Changes;
            auto parsed = PlayerbotStrategyControl::Parse(command);
            if (mutation->Scope != "BOT")
            {
                if (!parsed || !parsed->Mutation) { ack(0, "UNSUPPORTED_STRATEGY"); break; }
                if (char const* reason = DispatchGroupStrategy(sender, *mutation, command)) ack(0, reason);
                break;
            }
            ObjectGuid guid;
            for (auto const& entry : PlayerbotRoster::ListActiveFor(sender))
                if (entry.Name == mutation->Target) { guid = entry.Guid; break; }
            if (guid.IsEmpty()) { ack(0, "NO_BOT"); break; }
            if (!parsed || !parsed->Mutation) { ack(1, "UNSUPPORTED_STRATEGY"); break; }
            auto result = PlayerbotControl::DispatchStrategy(sender, guid, command, mutation->Token, mutation->Target);
            switch (result)
            {
                case PlayerbotControlResult::Queued: break; // Completion ACK comes after map execution/snapshot publication.
                case PlayerbotControlResult::Unauthorized: ack(1, "FORBIDDEN"); break;
                case PlayerbotControlResult::NotFollowing: ack(1, "NOT_FOLLOWING"); break;
                case PlayerbotControlResult::Busy: ack(1, "BUSY"); break;
                case PlayerbotControlResult::BotUnavailable: ack(1, "NO_BOT"); break;
                case PlayerbotControlResult::InvalidCommand: ack(1, "UNSUPPORTED_STRATEGY"); break;
            }
            break;
        }
        case PlayerbotAddonProtocol::Request::State:
        case PlayerbotAddonProtocol::Request::States:
        {
            auto abort = [&](char const* reason) { Reply(sender, "STATE_ABORT~" + request.Payload + "~~" + reason); };
            if (!PlayerbotModuleStrategyControlEnabled()) { abort("DISABLED"); break; }
            RequesterGuard* guard = GetRequesterGuard(sender, NowMs());
            if (!guard || !guard->StateGuard.Admit(NowMs())) { abort("RATE_LIMIT"); break; }
            bool global = request.Kind == PlayerbotAddonProtocol::Request::States;
            std::vector<PlayerbotAddonProtocol::StateRow> rows;
            bool failed = false;
            for (auto const& entry : PlayerbotRoster::ListActiveFor(sender))
            {
                if (!global && entry.Name != request.BotName) continue;
                WorldSession* session = sWorld->FindServerOriginPlayerbot(entry.Guid);
                Player* bot = session ? session->GetPlayer() : nullptr;
                auto snapshot = session ? session->GetPlayerbotStrategySnapshot() : nullptr;
                // Roster reauthorizes this request. Snapshot identity and freshness
                // prevent exposing another controller's or an old session's state.
                if (!bot || bot->IsBeingTeleported() || !snapshot ||
                    !PlayerbotAddonProtocol::SnapshotFresh(*snapshot, entry.Guid.GetCounter(),
                        sender.CanBeGameMaster() ? snapshot->Controller : sender.GetGUID().GetCounter(), getMSTime()))
                { failed = true; break; }
                rows.push_back({entry.Name, snapshot->Combat, snapshot->NonCombat});
            }
            if (failed) { abort("STATE_NOT_READY"); break; }
            if (!global && rows.empty()) { abort("NO_BOT"); break; }
            for (auto const& frame : PlayerbotAddonProtocol::FrameStrategyStates(request.Payload, rows, global)) Reply(sender, frame);
            break;
        }
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
