/* Cata map-to-world transport for donor-shaped group strategy completion.
 * GPL v2 or later. See PORTING.md. No native objects or packet delivery. */
#ifndef PLAYERBOT_STRATEGY_COMPLETIONS_H
#define PLAYERBOT_STRATEGY_COMPLETIONS_H
#include "PlayerbotStrategyBatch.h"
#include <mutex>
#include <vector>

namespace PlayerbotAddonProtocol
{
// Map producers submit only terminal copied results after snapshot publication.
// One world-thread consumer drains and correlates against its pending batches.
// Dropped/expired/canceled results stay unknown at the batch deadline; never retry
// a toggle or synthesize success. This queue alone does not enable group dispatch.
class StrategyCompletionInbox
{
public:
    static constexpr size_t Capacity = 4096; // Future table must admit at most 32 x 128 members.
    bool Submit(StrategyBatch::Completion completion)
    {
        if (!completion.Batch || !completion.Requester || !completion.Bot ||
            !ValidToken(completion.Token) || (completion.State != "C" && completion.State != "N") ||
            completion.Phase != StrategyBatch::Stage::Executed) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (pending.size() >= Capacity) return false;
        pending.push_back(std::move(completion));
        return true;
    }
    std::vector<StrategyBatch::Completion> Drain()
    {
        std::vector<StrategyBatch::Completion> result;
        std::lock_guard<std::mutex> lock(mutex);
        result.swap(pending);
        return result;
    }
private:
    std::mutex mutex;
    std::vector<StrategyBatch::Completion> pending;
};

inline StrategyCompletionInbox& GroupStrategyCompletions()
{
    static StrategyCompletionInbox inbox;
    return inbox;
}
}
#endif
