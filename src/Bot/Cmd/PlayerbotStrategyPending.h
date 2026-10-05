/* GPL v2 or later. World-owned Cata asynchronous adaptation of donor group ACKs. */
#ifndef PLAYERBOT_STRATEGY_PENDING_H
#define PLAYERBOT_STRATEGY_PENDING_H
#include "PlayerbotStrategyBatch.h"
#include "PlayerbotStrategyBinding.h"
#include <limits>

namespace PlayerbotAddonProtocol
{
class StrategyPending
{
public:
    static constexpr size_t Capacity = 32;
    struct Entry
    {
        uint32_t Requester;
        PlayerbotStrategyBinding Binding;
        std::shared_ptr<void const> Lease;
        StrategyBatch Batch;
    };
    struct Reply { uint32_t Requester; PlayerbotStrategyBinding Binding; std::string Message; };
    bool Busy(uint32_t account) const
    { for (auto const& entry : pending) if (entry.second.Binding.Account == account) return true; return false; }
    std::optional<uint64_t> Begin(uint32_t requester, PlayerbotStrategyBinding binding,
        StrategyMutation const& mutation, std::vector<uint32_t> const& bots, uint32_t now)
    {
        if (pending.size() >= Capacity || Busy(binding.Account) || !binding.Account || binding.Session.expired() ||
            binding.Scope != mutation.Scope || generation == std::numeric_limits<uint64_t>::max()) return {};
        auto lease = std::shared_ptr<void const>(std::make_shared<uint8_t>(0));
        binding.Lease = lease;
        if (!binding.Valid()) return {};
        auto batch = StrategyBatch::Create(generation + 1, requester, mutation, bots, now);
        if (!batch) return {};
        ++generation; // Never reuse during this process, even after timeout/cancellation.
        pending.emplace(generation, Entry{requester, binding, std::move(lease), std::move(*batch)});
        return generation;
    }
    std::map<uint64_t, Entry> const& Entries() const { return pending; }
    void Abandon(uint64_t id) { pending.erase(id); } // Lost login: no ACK to a replacement session.
    void Cancel(uint64_t id) { auto entry = pending.find(id); if (entry != pending.end()) entry->second.Lease.reset(); }
    bool Record(StrategyBatch::Completion const& result, uint32_t now)
    { auto entry = pending.find(result.Batch); return entry != pending.end() && entry->second.Batch.Record(result, now); }
    std::vector<Reply> Poll(uint32_t now)
    {
        std::vector<Reply> replies;
        for (auto it = pending.begin(); it != pending.end();)
            if (auto ack = it->second.Batch.TakeAck(now))
            {
                replies.push_back({it->second.Requester, it->second.Binding, std::move(*ack)});
                it = pending.erase(it);
            }
            else ++it;
        return replies;
    }
private:
    uint64_t generation = 0;
    std::map<uint64_t, Entry> pending;
};
}
#endif
