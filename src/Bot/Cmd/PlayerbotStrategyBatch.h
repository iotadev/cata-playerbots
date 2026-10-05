/* Donor-shaped MultiBot group ACK policy; Cata asynchronous adaptation.
 * Bridge 1da05982e478cb00e0b6c87314afe7e0e9653ffb, RunStrategyMutationCommand.
 * GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_STRATEGY_BATCH_H
#define PLAYERBOT_STRATEGY_BATCH_H
#include "PlayerbotAddonMutation.h"
#include "PlayerbotStrategyControl.h"
#include <map>

namespace PlayerbotAddonProtocol
{
// Intended owner: world thread. Callers must separately authorize/freeze the roster,
// bound active batches, and deliver copied completions from map threads. No native
// pointers, permissions, dispatch, locking or cancellation are provided by this policy.
class StrategyBatch
{
public:
    // The pinned addon expires strategy requests at 5000ms. Close earlier so
    // its pending token can still accept the aggregate ACK; pending-entry
    // destruction revokes the weak execution lease on this deadline.
    static constexpr uint32_t LifetimeMs = 4000;
    enum class Stage { Queued, Executed, AdmissionRejected };
    struct Completion
    {
        uint64_t Batch;
        uint32_t Requester, Bot;
        std::string Token, State;
        Stage Phase;
        bool Succeeded;
    };
    struct Counts { unsigned Matched = 0, Succeeded = 0, Failed = 0, Unknown = 0; };

    static std::optional<StrategyBatch> Create(uint64_t batch, uint32_t requester,
        StrategyMutation const& request, std::vector<uint32_t> const& bots, uint32_t now)
    {
        if (!batch || !requester || request.Scope == "BOT" || !request.Target.empty() ||
            request.Changes.size() > 160 || bots.size() > 128)
            return {};
        // Same supported state/feature grammar as single-bot admission. No role/spec override.
        auto command = PlayerbotStrategyControl::Parse((request.State == "C" ? "co " : "nc ") + request.Changes);
        if (!command || !command->Mutation || StrategyAck(request, unsigned(bots.size()), 0, 0, "TIMEOUT").empty())
            return {};
        StrategyBatch result(batch, requester, request, now);
        for (uint32_t bot : bots)
            if (!bot || !result.outcomes.emplace(bot, Outcome::Pending).second) return {};
        return result;
    }
    bool Record(Completion const& completion, uint32_t now)
    {
        if (closed || uint32_t(now - created) >= LifetimeMs || completion.Batch != batch ||
            completion.Requester != requester || completion.Token != request.Token || completion.State != request.State ||
            (completion.Phase != Stage::Executed && completion.Phase != Stage::AdmissionRejected) ||
            (completion.Phase == Stage::AdmissionRejected && completion.Succeeded)) return false;
        auto found = outcomes.find(completion.Bot);
        if (found == outcomes.end() || found->second != Outcome::Pending) return false;
        found->second = completion.Succeeded ? Outcome::Succeeded : Outcome::Failed;
        return true;
    }
    Counts Snapshot() const
    {
        Counts result;
        result.Matched = unsigned(outcomes.size());
        for (auto const& [bot, outcome] : outcomes)
        {
            (void)bot;
            if (outcome == Outcome::Succeeded) ++result.Succeeded;
            else if (outcome == Outcome::Failed) ++result.Failed;
            else ++result.Unknown;
        }
        return result;
    }
    std::optional<std::string> TakeAck(uint32_t now)
    {
        if (closed) return {};
        Counts counts = Snapshot();
        if (counts.Unknown && uint32_t(now - created) < LifetimeMs) return {};
        char const* reason = counts.Unknown ? "TIMEOUT" : !counts.Matched ? "NO_MATCH" :
            counts.Failed ? (counts.Succeeded ? "PARTIAL" : "FAILED") : "OK";
        // Pending outcomes stay unknown on timeout, not falsely failed/rolled back.
        // Donor reader accepts succeeded+failed <= matched and exposes reason.
        std::string ack = StrategyAck(request, counts.Matched, counts.Succeeded, counts.Failed, reason);
        if (ack.empty()) return {}; // Creation preflights the largest planned frame.
        closed = true;
        return ack;
    }
private:
    enum class Outcome { Pending, Succeeded, Failed };
    StrategyBatch(uint64_t batch, uint32_t requester, StrategyMutation request, uint32_t now)
        : batch(batch), requester(requester), created(now), request(std::move(request)) { }
    uint64_t batch;
    uint32_t requester, created;
    StrategyMutation request;
    std::map<uint32_t, Outcome> outcomes;
    bool closed = false;
};
}
#endif
