/* Adapted from TrainerAction at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. Native Cata training authority; see PORTING.md. */
#include "PlayerbotTraining.h"
#include "../../Script/PlayerbotConfig.h"
#include "PlayerbotSecurity.h"
#include "Chat.h"
#include "Creature.h"
#include "NPCPackets.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "Trainer.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>
namespace
{
struct Context { Player* Bot; Player* Human; Creature* Npc; Trainer::Trainer const* Trainer; uint32 TrainerId; };
std::optional<Context> Resolve(WorldSession& session, PlayerbotNpcRequest::Request const& request)
{
    Player* bot = session.GetPlayer();
    Player* human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request.Requester));
    bool attached = session.GetServerOriginFollowTargetGuidLow() == request.Requester ||
        session.GetServerOriginPartyControllerGuidLow() == request.Requester;
    if (!PlayerbotModuleTrainingEnabled() || !PlayerbotNpcRequest::Fresh(request, getMSTime()) ||
        !session.IsServerOrigin() || session.PlayerLoading() || session.isLogingOut() ||
        !bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported() ||
        bot->IsInCombat() || bot->IsInFlight() || bot->IsNonMeleeSpellCast(false) ||
        !attached || !human || !human->GetSession() || human->GetSession()->IsServerOrigin() ||
        !human->IsInWorld() || human->IsBeingTeleported() ||
        bot->GetMapId() != request.Map || bot->GetInstanceId() != request.Instance ||
        human->GetMap() != bot->GetMap() ||
        !PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *human)) return std::nullopt;
    ObjectGuid guid(request.Npc);
    Creature* npc = guid.IsCreature() ? bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER) : nullptr;
    if (!npc || !human->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER)) return std::nullopt;
    uint32 id = sObjectMgr->GetCreatureDefaultTrainer(npc->GetEntry());
    Trainer::Trainer const* trainer = id ? sObjectMgr->GetTrainer(id) : nullptr;
    if (!trainer || id > uint32(std::numeric_limits<int32>::max()) ||
        trainer->GetTrainerType() != Trainer::Type::Class || !trainer->IsTrainerValidForPlayer(bot)) return std::nullopt;
    return Context{bot, human, npc, trainer, id};
}
std::optional<uint32> Cost(Context const& context, Trainer::Spell const& offer)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(offer.SpellId);
    if (!offer.SpellId || offer.SpellId > uint32(std::numeric_limits<int32>::max()) || !spell ||
        spell->IsPrimaryProfessionFirstRank()) return std::nullopt;
    for (auto const& effect : spell->Effects)
        if (effect.Effect == SPELL_EFFECT_LEARN_SPELL)
        {
            SpellInfo const* learned = sSpellMgr->GetSpellInfo(effect.TriggerSpell);
            if (!learned || learned->IsPrimaryProfessionFirstRank()) return std::nullopt;
        }
    if (!context.Trainer->CanTeachSpell(context.Bot, &offer)) return std::nullopt;
    // Match native float multiplication and truncation without an out-of-range signed cast.
    float cost = offer.MoneyCost * context.Bot->GetReputationPriceDiscount(context.Npc);
    if (!std::isfinite(cost) || cost < 0 || double(cost) > std::numeric_limits<int32>::max()) return std::nullopt;
    return uint32(cost);
}
std::set<uint32> Unknown(Player const& bot, uint32 id)
{
    std::set<uint32> unknown;
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(id);
    if (!spell) return unknown;
    if (!spell->HasEffect(SPELL_EFFECT_LEARN_SPELL))
    { if (!bot.HasSpell(id)) unknown.insert(id); }
    else
        for (auto const& effect : spell->Effects)
            if (effect.Effect == SPELL_EFFECT_LEARN_SPELL && effect.TriggerSpell &&
                sSpellMgr->GetSpellInfo(effect.TriggerSpell) && !bot.HasSpell(effect.TriggerSpell))
                unknown.insert(effect.TriggerSpell);
    return unknown;
}
}
void PlayerbotTraining::Update(WorldSession& session, Mailbox& mailbox)
{
    auto request = mailbox.Take();
    if (!request) return;
    auto context = Resolve(session, *request);
    bool validOperand = request->Item == Inspect || request->Item == LearnAll ||
        request->Item <= uint32(std::numeric_limits<int32>::max());
    if (!context || !validOperand)
    {
        Player* human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
        if (human && human->GetSession())
            ChatHandler(human->GetSession()).SendSysMessage("Playerbot training rejected: invalid request, control/state change or nearby default class trainer not eligible.");
        mailbox.Finish(request->Serial);
        return;
    }
    struct Offer { uint32 Id, Cost; };
    std::vector<Offer> offers;
    std::set<uint32> seen;
    uint32 scanned = 0;
    bool truncated = context->Trainer->GetSpells().size() > MaxOffers;
    for (auto const& offer : context->Trainer->GetSpells())
    {
        if (scanned++ >= MaxOffers) break;
        if (!seen.insert(offer.SpellId).second) continue; // Native GetSpell uses the first matching row.
        if (request->Item != Inspect && request->Item != LearnAll && request->Item != offer.SpellId) continue;
        if (auto cost = Cost(*context, offer)) offers.push_back({offer.SpellId, *cost});
    }
    std::sort(offers.begin(), offers.end(), [](Offer a, Offer b) { return a.Cost != b.Cost ? a.Cost < b.Cost : a.Id < b.Id; });
    offers.erase(std::unique(offers.begin(), offers.end(), [](Offer a, Offer b) { return a.Id == b.Id; }), offers.end());
    if (request->Item == Inspect)
    {
        ChatHandler reply(context->Human->GetSession());
        reply.PSendSysMessage("Playerbot %s: %u currently learnable class offers in the bounded scan, showing at most %u. %s",
            context->Bot->GetName().c_str(), uint32(offers.size()), MaxListed, truncated ? "Catalog scan capped; remaining offers unresolved." : "");
        for (uint32 i = 0; i < offers.size() && i < MaxListed; ++i)
            reply.PSendSysMessage("Trainer spell %u: %u copper (%s).", offers[i].Id, offers[i].Cost,
                context->Bot->GetMoney() >= offers[i].Cost ? "affordable now" : "insufficient funds");
        mailbox.Finish(request->Serial);
        return;
    }
    uint32 attempted = 0, learned = 0;
    std::string botName = context->Bot->GetName();
    uint64 spent = 0;
    bool stopped = false;
    // Only initially available copied IDs, cheapest first. Newly unlocked offers wait for the next command.
    for (Offer candidate : offers)
    {
        if (attempted >= MaxPurchases) { stopped = true; break; }
        context = Resolve(session, *request);
        if (!context) { stopped = true; break; }
        Trainer::Spell const* current = nullptr;
        uint32 rows = 0;
        for (auto const& offer : context->Trainer->GetSpells())
        {
            if (rows++ >= MaxOffers) break;
            if (offer.SpellId == candidate.Id) { current = &offer; break; }
        }
        auto cost = current ? Cost(*context, *current) : std::nullopt;
        if (!cost) continue;
        if (context->Bot->GetMoney() < *cost) { stopped = true; break; }
        auto unknown = Unknown(*context->Bot, candidate.Id);
        uint64 money = context->Bot->GetMoney();
        context->Bot->SetFacingToObject(context->Npc);
        WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TRAINER_LIST)};
        hello.Unit = ObjectGuid(request->Npc);
        session.HandleTrainerListOpcode(hello);
        WorldPackets::NPC::TrainerBuySpell buy{WorldPacket(CMSG_TRAINER_BUY_SPELL)};
        buy.TrainerGUID = hello.Unit; buy.TrainerID = int32(context->TrainerId); buy.SpellID = int32(candidate.Id);
        session.HandleTrainerBuySpellOpcode(buy);
        ++attempted;
        uint32 observed = 0;
        for (uint32 id : unknown) if (context->Bot->HasSpell(id)) ++observed;
        learned += observed;
        spent += money >= context->Bot->GetMoney() ? money - context->Bot->GetMoney() : 0;
        if (!observed) { stopped = true; break; } // No automatic retry of an unknown/asynchronous outcome.
    }
    Player* human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
    if (human && human->GetSession() && !human->GetSession()->IsServerOrigin())
        ChatHandler(human->GetSession()).PSendSysMessage(
            "Playerbot %s training: attempted %u native purchase(s), observed %u newly learned spell(s), spent %llu copper. %s%sZero observation is not learning confirmation; no rollback/retry.",
            botName.c_str(), attempted, learned, static_cast<unsigned long long>(spent),
            stopped ? "Stopped on cap/funds/state or unconfirmed outcome. " : "",
            truncated ? "Catalog scan capped. " : "");
    mailbox.Finish(request->Serial);
}
