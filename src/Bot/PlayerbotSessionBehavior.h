/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_SESSION_BEHAVIOR_H
#define PLAYERBOT_SESSION_BEHAVIOR_H
#include "PlayerbotSessionHooks.h"
#include "ObjectGuid.h"
#include "PlayerbotAI.h"
#include "Engine.h"
#include <atomic>
#include <memory>
class PlayerbotSessionBehavior final : public PlayerbotSessionHooks
{
public:
    explicit PlayerbotSessionBehavior(WorldSession& session);
    void RequestServerOriginFollow(uint32 characterGuidLow) override;
    void RequestServerOriginHold() override;
    void RequestPartyControllerFollow(uint32 characterGuidLow) override;
    void RequestPartyControllerHold() override;
    void RequestServerOriginAttack() override;
    void RequestServerOriginCease() override;
    void RequestServerOriginInstanceJoin(uint32 mapId) override;
    uint32 GetFollowTargetGuidLow() const override { return _serverOriginFollowTargetGuidLow.load(); }
    uint32 GetPartyControllerGuidLow() const override { return _serverOriginPartyControllerGuidLow.load(); }
    bool IsAttacking() const override { return _serverOriginAttacking.load(); }
    void UpdateMap(uint32 diff) override;
    void UpdateWorld() override;
private:
    void UpdateServerOriginMovement(uint32 diff);
    void UpdateServerOriginParty();
    void UpdateServerOriginResurrection();
    void UpdateServerOriginInstanceJoin();
    void UpdateServerOriginCombat(uint32 diff);
    WorldSession& _session;
    // The adapter exists before login. The context/engine are made after the
    // class is known and destroyed in reverse order, with no cached Player*.
    PlayerbotAI _ai;
    std::unique_ptr<AiObjectContext> _aiContext;
    std::unique_ptr<Engine> _engine;
    bool _warriorEngineBuffAnnounced = false;
    bool _warriorEngineCombatAnnounced = false;
    bool _mageEngineCombatAnnounced = false;
    bool _priestEngineHealAnnounced = false;
    // World-thread command mailbox; movement itself is changed only in the map update.
    std::atomic<uint64> _serverOriginMovementRequest { 0 }; // UINT64_MAX means hold
    std::atomic<uint32> _serverOriginFollowTargetGuidLow { 0 };
    uint32 _serverOriginLastFollowOwnerGuidLow = 0;
    uint32 _serverOriginRecoveryOwnerGuidLow = 0;
    // Invitation ownership is separate from the console-only follow path.
    std::atomic<uint32> _serverOriginPartyControllerGuidLow { 0 };
    uint32 _serverOriginFormationSignature = 0;
    uint32 _serverOriginPathRefreshMs = 0;
    bool _serverOriginPathCatchUpActive = false;
    uint8 _serverOriginMeleeStance = 0; // 0 unset, 1 behind, 2 tank/front
    uint32 _serverOriginHealCheckTimer = 0;
    uint32 _serverOriginResurrectionCheckTimer = 0;
    std::atomic<bool> _serverOriginAutoAssistEnabled { false };
    uint32 _serverOriginFollowTraceRemainingMs = 0;
    uint32 _serverOriginFollowTraceSampleTimerMs = 0;
    std::atomic<uint32> _serverOriginInstanceJoinRequest { 0 };
    uint32 _serverOriginPartyLeaderGuidLow = 0;
    uint32 _serverOriginInstanceJoinMapId = 0;
    uint8 _serverOriginInstanceAckBudget = 0;
    std::atomic<uint8> _serverOriginCombatRequest { 0 }; // 1 attack selected creature, 2 cease
    std::atomic<bool> _serverOriginAttacking { false };
    bool _serverOriginAutoAssistedAttack = false;
    ObjectGuid _serverOriginCombatTargetGuid;
    uint32 _serverOriginActionCheckTimer = 0;
    uint32 _serverOriginAssistCheckTimer = 0;
    uint32 _serverOriginBuffCheckTimer = 0;

};
#endif
