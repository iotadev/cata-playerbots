/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotSessionBehavior.h"
#include "WorldSession.h"
#include "Creature.h"
#include "Group.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PartyPackets.h"
#include "Player.h"
#include "PlayerbotGroupStrategy.h"
#include "PlayerbotSecurity.h"
#include "MageAiObjectContext.h"
#include "PlayerbotMageStrategy.h"
#include "PriestAiObjectContext.h"
#include "PlayerbotPriestStrategy.h"
#include "../Script/PlayerbotConfig.h"
#include "PlayerbotWarriorStrategy.h"
#include "World.h"
#include "WorldPacket.h"
#include "WarriorAiObjectContext.h"
PlayerbotSessionBehavior::PlayerbotSessionBehavior(WorldSession& session)
    : _session(session), _ai(session) { }

std::unique_ptr<PlayerbotSessionHooks> CreatePlayerbotSessionHooks(WorldSession& session)
{
    return std::make_unique<PlayerbotSessionBehavior>(session);
}
bool PlayerbotModuleSupportsClass(uint8 playerClass)
{
    return playerClass == CLASS_WARRIOR || playerClass == CLASS_MAGE || playerClass == CLASS_PRIEST;
}
void PlayerbotSessionBehavior::UpdateMap(uint32 diff)
{
    // Construct class objects only after login yields the actual Player.
    if (!_engine)
        if (Player* bot = _ai.GetBot())
            if (bot->getClass() == CLASS_WARRIOR)
            {
                _aiContext = std::make_unique<WarriorAiObjectContext>(&_ai);
                _engine = std::make_unique<Engine>(&_ai, *_aiContext);
                _engine->AddStrategy("nc");
                _engine->AddStrategy(bot->GetPrimaryTalentTree(bot->GetActiveSpec()) == TALENT_TREE_WARRIOR_PROTECTION ? "tank" : "warrior");
            }
            else if (bot->getClass() == CLASS_MAGE)
            {
                _aiContext = std::make_unique<MageAiObjectContext>(&_ai);
                _engine = std::make_unique<Engine>(&_ai, *_aiContext);
                _engine->AddStrategy(bot->GetPrimaryTalentTree(bot->GetActiveSpec()) == TALENT_TREE_MAGE_FROST ? "frost" : "mage");
            }
            else if (bot->getClass() == CLASS_PRIEST)
            {
                _aiContext = std::make_unique<PriestAiObjectContext>(&_ai);
                _engine = std::make_unique<Engine>(&_ai, *_aiContext);
                _engine->AddStrategy("heal");
                _engine->AddStrategy("nc");
            }

    UpdateServerOriginParty();
    if (_serverOriginResurrectionCheckTimer > diff)
        _serverOriginResurrectionCheckTimer -= diff;
    else
    {
        _serverOriginResurrectionCheckTimer = 2000;
        UpdateServerOriginResurrection();
    }
    UpdateServerOriginMovement(diff);
    UpdateServerOriginInstanceJoin();
    UpdateServerOriginCombat(diff);
}
void PlayerbotSessionBehavior::UpdateWorld()
{
    Player* _player = _session.GetPlayer();
    if (_session.IsServerOrigin() && _player && _serverOriginInstanceAckBudget && _player->IsBeingTeleportedFar())
    {
        --_serverOriginInstanceAckBudget;
        _session.HandleMoveWorldportAck();
        _player = _session.GetPlayer();
        if (_player && _player->IsInWorld())
        {
            if (_player->GetMapId() == _serverOriginInstanceJoinMapId)
            {
                TC_LOG_INFO("server", "PB-PARTY: %s entered dungeon map %u instance %u", _player->GetName().c_str(),
                    _player->GetMapId(), _player->GetInstanceId());
                uint32 controllerGuidLow = _serverOriginPartyLeaderGuidLow;
                Group* group = _player->GetGroup();
                if (controllerGuidLow && group && group->IsMember(ObjectGuid::Create<HighGuid::Player>(controllerGuidLow)))
                {
                    RequestServerOriginFollow(controllerGuidLow);
                    _serverOriginPartyControllerGuidLow.store(controllerGuidLow);
                }
                else
                    RequestServerOriginHold();
            }
            else
                TC_LOG_INFO("server", "PB-PARTY: %s dungeon transfer failed; arrived on map %u", _player->GetName().c_str(), _player->GetMapId());
            _serverOriginInstanceAckBudget = 0;
        }
    }

}
void PlayerbotSessionBehavior::RequestServerOriginFollow(uint32 characterGuidLow)
{
    ASSERT(_session.IsServerOrigin() && characterGuidLow);
    RequestServerOriginCease();
    _serverOriginPartyControllerGuidLow.store(0);
    _serverOriginMovementRequest.store(characterGuidLow);
}

void PlayerbotSessionBehavior::RequestServerOriginHold()
{
    ASSERT(_session.IsServerOrigin());
    RequestServerOriginCease();
    _serverOriginPartyControllerGuidLow.store(0);
    _serverOriginMovementRequest.store(uint64(-1));
}

void PlayerbotSessionBehavior::RequestPartyControllerFollow(uint32 characterGuidLow)
{
    ASSERT(_session.IsServerOrigin() && characterGuidLow);
    if (_serverOriginPartyControllerGuidLow.load() != characterGuidLow)
        return;
    RequestServerOriginCease();
    _serverOriginMovementRequest.store(characterGuidLow);
}

void PlayerbotSessionBehavior::RequestPartyControllerHold()
{
    ASSERT(_session.IsServerOrigin());
    if (!_serverOriginPartyControllerGuidLow.load())
        return;
    RequestServerOriginCease();
    _serverOriginMovementRequest.store(uint64(-1));
}

void PlayerbotSessionBehavior::RequestServerOriginAttack()
{
    ASSERT(_session.IsServerOrigin());
    _serverOriginCombatRequest.store(1);
}

void PlayerbotSessionBehavior::RequestServerOriginCease()
{
    ASSERT(_session.IsServerOrigin());
    _serverOriginAutoAssistEnabled.store(false);
    _serverOriginCombatRequest.store(2);
}

void PlayerbotSessionBehavior::RequestServerOriginInstanceJoin(uint32 mapId)
{
    ASSERT(_session.IsServerOrigin() && mapId);
    _serverOriginInstanceJoinRequest.store(mapId);
}

void PlayerbotSessionBehavior::UpdateServerOriginMovement(uint32 diff)
{
    Player* _player = _session.GetPlayer();
    if (!_session.IsServerOrigin() || !_player || !_player->IsInWorld())
        return;

    auto hold = [this, _player](bool retainOwner)
    {
        _serverOriginFollowTargetGuidLow.store(0);
        if (!retainOwner)
        {
            _ai.ClearController();
            _serverOriginLastFollowOwnerGuidLow = 0;
            _serverOriginRecoveryOwnerGuidLow = 0;
        }
        _serverOriginAutoAssistEnabled.store(false);
        _serverOriginFormationSignature = 0;
        _serverOriginPathRefreshMs = 0;
        _serverOriginPathCatchUpActive = false;
        _serverOriginHealCheckTimer = 0;
        _serverOriginFollowTraceRemainingMs = 0;
        _serverOriginFollowTraceSampleTimerMs = 0;
        _player->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
        _player->GetMotionMaster()->Clear(MOTION_SLOT_IDLE);
        _player->GetMotionMaster()->MoveIdle();
        _player->StopMoving();
    };

    uint64 request = _serverOriginMovementRequest.exchange(0);
    if (request == uint64(-1))
        hold(false);
    else if (request)
    {
        ObjectGuid ownerGuid = ObjectGuid::Create<HighGuid::Player>(uint32(request));
        Player* owner = _player->GetMap()->GetPlayer(ownerGuid);
        if (owner && owner != _player && !owner->GetSession()->IsServerOrigin() && owner->IsAlive() && _player->IsAlive())
        {
            PlayerbotGroup::FollowPosition position = PlayerbotGroup::PositionFor(*_player, *owner);
            _player->GetMotionMaster()->MoveFollow(owner, position.Distance, position.Angle);
            _serverOriginFormationSignature = position.Signature;
            _serverOriginPathRefreshMs = 0;
            _serverOriginPathCatchUpActive = false;
            _serverOriginHealCheckTimer = 0;
            _serverOriginFollowTargetGuidLow.store(uint32(request));
            _ai.SetController(ownerGuid);
            _serverOriginLastFollowOwnerGuidLow = uint32(request);
            _serverOriginRecoveryOwnerGuidLow = 0;
            _serverOriginAutoAssistEnabled.store(true);
            _serverOriginFollowTraceRemainingMs = 60000;
            _serverOriginFollowTraceSampleTimerMs = 0;
            TC_LOG_INFO("server", "PB-01: %s following %s", _player->GetName().c_str(), owner->GetName().c_str());
            if (_player->getClass() == CLASS_PRIEST)
                PlayerbotPriest::LogKnownAbilities(*_player);
        }
        else
        {
            hold(false);
            TC_LOG_INFO("server", "PB-01: follow target %u unavailable on bot map; holding", uint32(request));
        }
    }

    uint32 activeOwner = _serverOriginFollowTargetGuidLow.load();
    if (activeOwner)
    {
        Player* owner = _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(activeOwner));
        if (!owner || !owner->IsAlive() || !_player->IsAlive())
        {
            bool died = (owner && !owner->IsAlive()) || !_player->IsAlive();
            hold(died || (!owner && _player->IsAlive()));
            if (died)
                _serverOriginRecoveryOwnerGuidLow = activeOwner;
            if (!_player->IsAlive())
                TC_LOG_INFO("server", "PB-DEATH: bot %s died while following; holding", _player->GetName().c_str());
            else if (owner && !owner->IsAlive())
                TC_LOG_INFO("server", "PB-DEATH: human owner %s died; bot %s holding", owner->GetName().c_str(), _player->GetName().c_str());
            else
                TC_LOG_INFO("server", "PB-01: follow target left bot map; holding");
        }
        else if (_serverOriginFollowTraceRemainingMs)
        {
            if (_serverOriginFollowTraceSampleTimerMs <= diff)
            {
                TC_LOG_INFO("server", "PB-01 trace: t=%us owner=(%.2f,%.2f,%.2f) bot=(%.2f,%.2f,%.2f) distance=%.1f ownerFlags=%u botMotion=%u",
                    (60000 - _serverOriginFollowTraceRemainingMs) / 1000,
                    owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(),
                    _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(),
                    _player->GetDistance(owner), owner->GetUnitMovementFlags(),
                    uint32(_player->GetMotionMaster()->GetCurrentMovementGeneratorType()));
                _serverOriginFollowTraceSampleTimerMs = 1000;
            }
            else
                _serverOriginFollowTraceSampleTimerMs -= diff;

            _serverOriginFollowTraceRemainingMs = _serverOriginFollowTraceRemainingMs > diff ? _serverOriginFollowTraceRemainingMs - diff : 0;
        }
        if (owner && owner->IsAlive() && _player->IsAlive() && !_serverOriginAttacking.load() && !_player->IsInCombat())
        {
            PlayerbotGroup::FollowPosition position = PlayerbotGroup::PositionFor(*_player, *owner);
            bool usePath = PlayerbotGroup::ShouldPathCatchUp(_serverOriginPathCatchUpActive, _player->GetExactDist2d(owner));
            if (usePath)
            {
                if (_serverOriginPathRefreshMs <= diff)
                {
                    // Playerbots' far-follow action uses a path to close long gaps.
                    // Let the core pathfinder handle this leg, then resume formation.
                    _player->GetMotionMaster()->MovePoint(0, owner->GetPosition(), true);
                    _serverOriginPathRefreshMs = 3000;
                }
                else
                    _serverOriginPathRefreshMs -= diff;
                _serverOriginPathCatchUpActive = true;
            }
            else if (_serverOriginPathCatchUpActive || position.Signature != _serverOriginFormationSignature)
            {
                if (_serverOriginPathCatchUpActive)
                    _player->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
                _player->GetMotionMaster()->MoveFollow(owner, position.Distance, position.Angle);
                _serverOriginFormationSignature = position.Signature;
                _serverOriginPathCatchUpActive = false;
                _serverOriginPathRefreshMs = 0;
            }
        }
    }
    else if (_serverOriginRecoveryOwnerGuidLow)
    {
        Player* owner = _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(_serverOriginRecoveryOwnerGuidLow));
        if (owner && PlayerbotGroup::CanResumeAfterDeath(*_player, *owner))
        {
            uint32 ownerGuidLow = _serverOriginRecoveryOwnerGuidLow;
            bool partyControlled = _serverOriginPartyControllerGuidLow.load() == ownerGuidLow;
            _serverOriginRecoveryOwnerGuidLow = 0;
            RequestServerOriginFollow(ownerGuidLow);
            if (partyControlled)
                _serverOriginPartyControllerGuidLow.store(ownerGuidLow);
            TC_LOG_INFO("server", "PB-RECOVERY: %s and owner %s are alive nearby; resuming follow", _player->GetName().c_str(), owner->GetName().c_str());
        }
    }
}

void PlayerbotSessionBehavior::UpdateServerOriginParty()
{
    Player* _player = _session.GetPlayer();
    if (!_session.IsServerOrigin() || !_player || !_player->IsInWorld())
        return;

    if (uint32 controllerGuidLow = _serverOriginPartyControllerGuidLow.load())
    {
        Group* group = _player->GetGroup();
        if (!group || !group->IsMember(ObjectGuid::Create<HighGuid::Player>(controllerGuidLow)))
        {
            RequestServerOriginHold();
            _serverOriginPartyLeaderGuidLow = 0;
            TC_LOG_INFO("server", "PB-PARTY: %s lost invitation-adopted controller GUID %u; holding", _player->GetName().c_str(), controllerGuidLow);
        }
    }

    if (_player->GetGroup())
        return;

    Group* invitation = _player->GetGroupInvite();
    if (!invitation)
        return;

    // Adapt mod-playerbots' AcceptInvitationAction: resolve the inviting
    // leader, use the core accept flow, then adopt that human as controller.
    // This default-off dev roster keeps the donor invite/control distinction:
    // an ungrouped bot may accept an eligible human invite, while full control
    // belongs to the adopted party controller. The core also checks the invite.
    Player* owner = ObjectAccessor::FindPlayer(invitation->GetLeaderGUID());
    WorldPackets::Party::PartyInviteResponse response(WorldPacket(CMSG_PARTY_INVITE_RESPONSE, 0));
    if (!owner || !PlayerbotSecurity(*_player).CheckLevelFor(PLAYERBOT_SECURITY_INVITE, *owner))
    {
        response.Accept = false;
        _session.HandlePartyInviteResponseOpcode(response);
        TC_LOG_INFO("server", "PB-PARTY: %s declined invitation without an available human leader", _player->GetName().c_str());
        return;
    }

    uint32 ownerGuidLow = owner->GetGUID().GetCounter();
    response.Accept = true;
    _session.HandlePartyInviteResponseOpcode(response);
    if (_player->GetGroup() && _player->GetGroup()->IsMember(owner->GetGUID()))
    {
        _serverOriginPartyLeaderGuidLow = ownerGuidLow;
        RequestServerOriginCease();
        if (owner->GetMap() == _player->GetMap() && owner->IsAlive() && _player->IsAlive())
            RequestServerOriginFollow(ownerGuidLow);
        else
        {
            RequestServerOriginHold();
        }
        _serverOriginPartyControllerGuidLow.store(ownerGuidLow);
        TC_LOG_INFO("server", "PB-PARTY: %s joined followed leader GUID %u's party; controller adopted from invitation", _player->GetName().c_str(), ownerGuidLow);
        PlayerbotGroup::GreetOnJoin(*_player, *owner);
    }
}

void PlayerbotSessionBehavior::UpdateServerOriginResurrection()
{
    Player* bot = _ai.GetBot();
    if (!bot || bot->IsAlive() || !bot->IsResurrectRequested() ||
        !PlayerbotModuleEnginePriestHealEnabled())
        return;

    Group* group = bot->GetGroup();
    if (!group)
        return;

    // Adapt donor AcceptResurrectAction: answer only a request from a current
    // party member and let Cata's normal handler apply raid and request rules.
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member->GetMap() != bot->GetMap() ||
            !bot->IsResurrectRequestedBy(member->GetGUID()))
            continue;

        WorldPacket response(CMSG_RESURRECT_RESPONSE, 9);
        response << member->GetGUID() << uint8(1);
        _session.HandleResurrectResponseOpcode(response);
        if (bot->IsAlive())
            TC_LOG_INFO("server", "PB-RECOVERY: %s accepted resurrection from %s", bot->GetName().c_str(), member->GetName().c_str());
        return;
    }
}

void PlayerbotSessionBehavior::UpdateServerOriginInstanceJoin()
{
    Player* _player = _session.GetPlayer();
    if (!_session.IsServerOrigin() || !_player || !_player->IsInWorld())
        return;

    uint32 mapId = _serverOriginInstanceJoinRequest.exchange(0);
    if (!mapId)
        return;

    Group* group = _player->GetGroup();
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
    AreaTriggerStruct const* entrance = mapEntry && mapEntry->IsDungeon() && !mapEntry->IsRaid() ?
        sObjectMgr->GetMapEntranceTrigger(mapId) : nullptr;
    if (!group || !_serverOriginPartyLeaderGuidLow ||
        group->GetLeaderGUID() != ObjectGuid::Create<HighGuid::Player>(_serverOriginPartyLeaderGuidLow) ||
        !_player->IsAlive() || _player->IsInCombat() || _player->IsBeingTeleportedFar() ||
        !entrance || !group->GetBoundInstance(mapEntry))
    {
        TC_LOG_INFO("server", "PB-PARTY: dungeon join rejected for %s map %u; requires a living, idle bot in its followed leader's bound party dungeon", _player->GetName().c_str(), mapId);
        return;
    }

    if (!_player->TeleportTo(mapId, entrance->target_X, entrance->target_Y, entrance->target_Z, entrance->target_Orientation))
    {
        TC_LOG_INFO("server", "PB-PARTY: dungeon join denied by core entry checks for %s map %u", _player->GetName().c_str(), mapId);
        return;
    }

    _serverOriginInstanceJoinMapId = mapId;
    _serverOriginInstanceAckBudget = 2; // one target transfer, at most one homebind fallback
    TC_LOG_INFO("server", "PB-PARTY: %s began dungeon transfer to map %u", _player->GetName().c_str(), mapId);
}

void PlayerbotSessionBehavior::UpdateServerOriginCombat(uint32 diff)
{
    Player* _player = _session.GetPlayer();
    if (!_session.IsServerOrigin() || !_player || !_player->IsInWorld())
        return;

    if (_player->getClass() == CLASS_PRIEST &&
        (_serverOriginFollowTargetGuidLow.load() || _serverOriginRecoveryOwnerGuidLow))
    {
        if (_serverOriginHealCheckTimer > diff)
            _serverOriginHealCheckTimer -= diff;
        else
        {
            _serverOriginHealCheckTimer = 750;
            if (_engine && PlayerbotModuleEnginePriestHealEnabled())
            {
                if (!_priestEngineHealAnnounced)
                {
                    TC_LOG_INFO("server", "PB-ENGINE: %s Priest healing routed through engine", _player->GetName().c_str());
                    _priestEngineHealAnnounced = true;
                }
                _engine->Tick(false, false, _player->IsInCombat());
            }
            else
                PlayerbotPriest::HealParty(*_player, _ai.GetController());
        }
    }

    auto cease = [this, _player]()
    {
        _ai.ClearCurrentTarget();
        if (_serverOriginAttacking.exchange(false))
            TC_LOG_INFO("server", "PB-02: %s ceased attack", _player->GetName().c_str());
        _serverOriginCombatTargetGuid.Clear();
        _serverOriginAutoAssistedAttack = false;
        _serverOriginActionCheckTimer = 0;
        _serverOriginMeleeStance = 0;
        _player->AttackStop();
        _player->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
        _player->StopMoving();
    };

    auto validTarget = [_player](Player* owner, Creature* target)
    {
        return owner && owner->IsAlive() && target && !target->IsControlledByPlayer() && target->IsAlive() && _player->IsAlive() &&
            _player->IsValidAttackTarget(target) && _player->IsWithinDistInMap(target, 25.0f) &&
            owner->IsWithinDistInMap(target, 25.0f) && _player->IsWithinLOSInMap(target);
    };

    auto beginAttack = [this, _player, &cease](Creature* target, bool autoAssisted)
    {
        bool ranged = _player->getClass() == CLASS_MAGE;
        if (!ranged && _player->getClass() != CLASS_WARRIOR)
            return false;
        cease();
        if (!_player->Attack(target, !ranged))
            return false;

        _serverOriginCombatTargetGuid = target->GetGUID();
        _ai.SetCurrentTarget(_serverOriginCombatTargetGuid);
        _serverOriginAttacking.store(true);
        _serverOriginAutoAssistedAttack = autoAssisted;
        _serverOriginActionCheckTimer = 0;
        _serverOriginPathCatchUpActive = false;
        if (ranged)
            _player->GetMotionMaster()->MoveChase(target, 20.0f);
        else
        {
            bool tanking = target->GetVictim() == _player;
            _serverOriginMeleeStance = tanking ? 2 : 1;
            _player->GetMotionMaster()->MoveChase(target, std::nullopt, ChaseAngle(PlayerbotGroup::MeleeChaseAngle(tanking)));
        }
        TC_LOG_INFO("server", "PB-02: %s %s %s", _player->GetName().c_str(), autoAssisted ? "auto-assisting" : "attacking", target->GetName().c_str());
        if (ranged)
            PlayerbotMage::LogKnownAbilities(*_player);
        else
            PlayerbotWarrior::LogKnownAbilities(*_player);
        return true;
    };

    uint8 request = _serverOriginCombatRequest.exchange(0);
    if (request == 2)
        cease();
    else if (request == 1)
    {
        uint32 ownerGuidLow = _serverOriginFollowTargetGuidLow.load();
        Player* owner = ownerGuidLow ? _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(ownerGuidLow)) : nullptr;
        Unit* selected = owner ? owner->GetSelectedUnit() : nullptr;
        Creature* target = selected ? selected->ToCreature() : nullptr;
        if (!validTarget(owner, target))
        {
            TC_LOG_INFO("server", "PB-02: attack rejected; selected=%s ownerDist=%.1f botTargetDist=%.1f ownerTargetDist=%.1f targetAlive=%u validTarget=%u lineOfSight=%u",
                selected ? selected->GetName().c_str() : "none", owner ? _player->GetDistance(owner) : -1.0f,
                target ? _player->GetDistance(target) : -1.0f, owner && target ? owner->GetDistance(target) : -1.0f,
                target && target->IsAlive(), target && _player->IsValidAttackTarget(target), target && _player->IsWithinLOSInMap(target));
            cease();
        }
        else
        {
            if (!beginAttack(target, false))
                TC_LOG_INFO("server", "PB-02: attack rejected by core combat rules");
        }
    }

    if (!request && !_serverOriginAttacking.load() && _serverOriginAutoAssistEnabled.load())
    {
        if (_serverOriginAssistCheckTimer > diff)
            _serverOriginAssistCheckTimer -= diff;
        else
        {
            _serverOriginAssistCheckTimer = 500;
            uint32 ownerGuidLow = _serverOriginFollowTargetGuidLow.load();
            Player* owner = ownerGuidLow ? _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(ownerGuidLow)) : nullptr;
            Unit* selected = owner ? owner->GetSelectedUnit() : nullptr;
            Creature* target = selected ? selected->ToCreature() : nullptr;
            if (_player->getClass() != CLASS_PRIEST && validTarget(owner, target) && owner->IsInCombatWith(target))
                beginAttack(target, true);
        }
    }

    if (!_serverOriginAttacking.load() && _serverOriginFollowTargetGuidLow.load() && !_player->IsInCombat())
    {
        if (_serverOriginBuffCheckTimer > diff)
            _serverOriginBuffCheckTimer -= diff;
        else
        {
            _serverOriginBuffCheckTimer = 2000;
            if (_player->getClass() == CLASS_WARRIOR && _engine &&
                PlayerbotModuleEngineWarriorBuffEnabled())
            {
                if (!_warriorEngineBuffAnnounced)
                {
                    TC_LOG_INFO("server", "PB-ENGINE: %s Warrior buff routed through engine", _player->GetName().c_str());
                    _warriorEngineBuffAnnounced = true;
                }
                _engine->Tick();
            }
            else
                PlayerbotWarrior::MaintainBuff(*_player);
            if (Player* owner = _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(_serverOriginFollowTargetGuidLow.load())))
            {
                PlayerbotMage::MaintainBuff(*_player, *owner);
                PlayerbotPriest::MaintainBuff(*_player, *owner);
            }
        }
    }

    if (_serverOriginAttacking.load())
    {
        uint32 ownerGuidLow = _serverOriginFollowTargetGuidLow.load();
        Player* owner = ownerGuidLow ? _player->GetMap()->GetPlayer(ObjectGuid::Create<HighGuid::Player>(ownerGuidLow)) : nullptr;
        Creature* target = ObjectAccessor::GetCreature(*_player, _serverOriginCombatTargetGuid);
        bool ownerBeyondLeash = owner && !_player->IsWithinDistInMap(owner, 35.0f);
        bool targetBeyondLeash = target && !_player->IsWithinDistInMap(target, 35.0f);
        if (!owner || !owner->IsAlive() || !target || target->IsControlledByPlayer() || !target->IsAlive() || !_player->IsAlive() ||
            _player->GetVictim() != target || !_player->IsValidAttackTarget(target) ||
            (_serverOriginAutoAssistedAttack && !owner->IsInCombatWith(target)) ||
            ownerBeyondLeash || targetBeyondLeash)
        {
            if (target && !target->IsAlive())
                TC_LOG_INFO("server", "PB-02: %s target died; returning to follow", _player->GetName().c_str());
            else if (ownerBeyondLeash || targetBeyondLeash)
                TC_LOG_INFO("server", "PB-02: %s exceeded combat leash; returning to follow", _player->GetName().c_str());
            cease();
            return;
        }

        if (_serverOriginActionCheckTimer > diff)
        {
            _serverOriginActionCheckTimer -= diff;
            return;
        }
        _serverOriginActionCheckTimer = 1000;

        if (_player->getClass() == CLASS_MAGE)
        {
            if (!_player->IsNonMeleeSpellCast(false))
                _player->SetFacingToObject(target);
            if (_engine && PlayerbotModuleEngineMageCombatEnabled())
            {
                bool frost = _player->GetPrimaryTalentTree(_player->GetActiveSpec()) == TALENT_TREE_MAGE_FROST;
                char const* strategy = frost ? "frost" : "mage";
                if (!_engine->HasStrategy(strategy))
                {
                    _engine->RemoveStrategy(frost ? "mage" : "frost");
                    _engine->AddStrategy(strategy);
                }
                if (!_mageEngineCombatAnnounced)
                {
                    TC_LOG_INFO("server", "PB-ENGINE: %s Mage combat routed through %s strategy", _player->GetName().c_str(), strategy);
                    _mageEngineCombatAnnounced = true;
                }
                _engine->Tick(false, false, true);
            }
            else
                PlayerbotMage::Execute(*_player, *target);
            return;
        }

        bool tanking = target->GetVictim() == _player;
        uint8 stance = tanking ? 2 : 1;
        if (stance != _serverOriginMeleeStance)
        {
            _player->GetMotionMaster()->MoveChase(target, std::nullopt, ChaseAngle(PlayerbotGroup::MeleeChaseAngle(tanking)));
            _serverOriginMeleeStance = stance;
        }

        if (_engine && PlayerbotModuleEngineWarriorCombatEnabled())
        {
            bool protection = _player->GetPrimaryTalentTree(_player->GetActiveSpec()) == TALENT_TREE_WARRIOR_PROTECTION;
            char const* strategy = protection ? "tank" : "warrior";
            if (!_engine->HasStrategy(strategy))
            {
                _engine->RemoveStrategy(protection ? "warrior" : "tank");
                _engine->AddStrategy(strategy);
            }
            if (!_warriorEngineCombatAnnounced)
            {
                TC_LOG_INFO("server", "PB-ENGINE: %s Warrior combat routed through %s strategy", _player->GetName().c_str(), strategy);
                _warriorEngineCombatAnnounced = true;
            }
            _engine->Tick(false, false, true);
        }
        else
            PlayerbotWarrior::Execute(*_player, *target);
    }
}
