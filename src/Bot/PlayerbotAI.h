/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_PLAYERBOTAI_H
#define PLAYERBOTS_PLAYERBOTAI_H

#include "ObjectGuid.h"
#include "../Ai/Base/PlayerbotCorpseLoot.h"
#include "../Ai/Base/PlayerbotPartySupport.h"
#include "../Ai/Base/PlayerbotRestItem.h"
#include "ForceRebuff.h"

class Creature;
class Player;
class WorldSession;
class AiObjectContext;
class Engine;

// Session-bound Cata adapter for the donor engine's PlayerbotAI* interfaces.
// It never owns or retains a Player pointer. Accessors are map-thread only.
class PlayerbotAI
{
public:
    explicit PlayerbotAI(WorldSession& session) : session(session) { }
    Player* GetBot() const;
    Player* GetController() const;
    Creature* GetCurrentTarget() const;
    // Borrowed engine context; map-thread only, bound/cleared by its lifetime.
    AiObjectContext* GetAiObjectContext() const { return context; }
    void SetAiObjectContext(AiObjectContext* value) { context = value; }
    // Borrowed decision engine; map-thread only, bound/cleared by Engine lifetime.
    Engine const* GetDecisionEngine() const { return decisionEngine; }
    void SetDecisionEngine(Engine const* value) { decisionEngine = value; }
    void SetController(ObjectGuid guid)
    {
        if (controllerGuid != guid)
        {
            forceRebuff.End();
            lootCandidates.Clear();
            stayActive = false;
        }
        controllerGuid = guid;
    }
    void ClearController() { controllerGuid.Clear(); lootCandidates.Clear(); forceRebuff.End(); stayActive = false; }
    bool IsStaying() const { return stayActive; }
    void SetStaying(bool value) { stayActive = value; }
    ForceRebuffState& Rebuff() { return forceRebuff; }
    ForceRebuffState const& Rebuff() const { return forceRebuff; }
    void SetCurrentTarget(ObjectGuid guid) { targetGuid = guid; }
    void ClearCurrentTarget() { targetGuid.Clear(); }
    // Map-owned rest state contains a spell identity and recovery mode, no pointers.
    void BeginRest(uint32 spellId, bool drinking) { rest.Begin(spellId, drinking); }
    uint32 GetRestSpellId() const { return rest.Spell; }
    bool RestFinished(float health, float mana) const { return rest.Finished(health, mana); }
    void ClearRest() { rest.Clear(); }
    PlayerbotCorpseLoot::Mailbox& LootRequests() { return lootRequests; }
    PlayerbotCorpseLoot::AttemptHistory& LootAttempts() { return lootAttempts; }
    void RecordDefeatedCreature(ObjectGuid guid, uint32 now) { defeatedCreature = guid; lootCandidates.Add(guid, now); }
    ObjectGuid GetDefeatedCreature() const { return defeatedCreature; }
    PlayerbotCorpseLoot::Candidates& LootCandidates() { return lootCandidates; }
    PlayerbotCorpseLoot::Pursuit& LootPursuit() { return lootPursuit; }
    PlayerbotPartySupport::ReachRequest& SupportReachRequests() { return supportReachRequests; }
    void RequestSpellChaseRefresh() { spellChaseRefresh = true; }
    bool NeedsSpellChaseRefresh() const { return spellChaseRefresh; }
    void ClearSpellChaseRefresh() { spellChaseRefresh = false; }

private:
    WorldSession& session;
    ForceRebuffState forceRebuff;
    AiObjectContext* context = nullptr;
    Engine const* decisionEngine = nullptr;
    ObjectGuid controllerGuid;
    ObjectGuid targetGuid;
    PlayerbotRest::ActiveRest rest;
    bool spellChaseRefresh = false; // map-owned, rechecked against current native victim
    bool stayActive = false; // map-owned, never a cross-session movement authority
    PlayerbotCorpseLoot::Mailbox lootRequests;
    PlayerbotCorpseLoot::AttemptHistory lootAttempts; // map-owned
    ObjectGuid defeatedCreature; // map-owned, no retained creature pointer
    PlayerbotCorpseLoot::Candidates lootCandidates; // map-owned
    PlayerbotCorpseLoot::Pursuit lootPursuit; // map-owned
    PlayerbotPartySupport::ReachRequest supportReachRequests; // map-owned, consumed in the same update
};

#endif
