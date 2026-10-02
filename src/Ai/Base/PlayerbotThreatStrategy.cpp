/* Adapted from donor ThreatStrategy.cpp at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See PORTING.md. Released under GNU GPL v2 or any later version. */
#include "PlayerbotThreatStrategy.h"
#include "../../Bot/Engine/Multiplier.h"
#include "../../Bot/PlayerbotAI.h"
#include "../../Bot/Engine/AiObjectContext.h"
#include "Player.h"

namespace
{
class ThreatMultiplier final : public Multiplier
{
public:
    explicit ThreatMultiplier(PlayerbotAI* ai) : Multiplier(ai, "threat") { }
    float GetValue(Action* action) override
    {
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        if (!context) return 1.0f;
        Value<bool>* neglect = context->GetValue<bool>("neglect threat");
        if (neglect && neglect->Get()) return 1.0f; // donor one-shot, consumed even for support
        if (!action || action->getThreatType() != Action::ActionThreatType::Single) return 1.0f;
        Player* bot = botAI->GetBot();
        if (!bot || !bot->GetGroup()) return 1.0f;
        Value<uint8>* threat = context->GetValue<uint8>("threat", "current target");
        return PlayerbotThreat::DamageMultiplier(action->getThreatType(), true, threat ? threat->Get() : 0);
    }
};
class ThreatStrategy final : public Strategy
{
public:
    explicit ThreatStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "threat"; }
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override
    {
        multipliers.push_back(new ThreatMultiplier(botAI));
    }
};
}
void PlayerbotThreat::AddContexts(SharedNamedObjectContextList<Strategy>& strategies)
{
    auto* factory = new NamedObjectContext<Strategy>();
    factory->creators["threat"] = [](PlayerbotAI* ai) { return new ThreatStrategy(ai); };
    strategies.Add(factory);
}
