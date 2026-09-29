/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotSessionHooks.h"
#include "PlayerbotConfig.h"
#include "Config.h"
#include "World.h"
bool PlayerbotModuleEngineWarriorBuffEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EngineWarriorBuff", false);
}

bool PlayerbotModuleEngineWarriorCombatEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EngineWarriorCombat", false);
}

bool PlayerbotModuleEngineMageCombatEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EngineMageCombat", false);
}

bool PlayerbotModuleEnginePriestHealEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EnginePriestHeal", false);
}

void LoadPlayerbotModuleSettings(World& world, bool moduleConfigsValid)
{
    world.setBoolConfig(CONFIG_PLAYERBOTS_DEV_ENABLED, moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Dev.Enabled", false));
    world.setBoolConfig(CONFIG_PLAYERBOTS_DEV_GREETING_ENABLED, sConfigMgr->GetBoolDefault("Playerbots.Dev.GreetingOnJoin", true));
    int32 devPlayerbotAccountId = sConfigMgr->GetIntDefault("Playerbots.Dev.AccountId", 0);
    int32 devPlayerbotCharacterGuid = sConfigMgr->GetIntDefault("Playerbots.Dev.CharacterGuid", 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_ACCOUNT_ID, devPlayerbotAccountId > 0 ? uint32(devPlayerbotAccountId) : 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_CHARACTER_GUID, devPlayerbotCharacterGuid > 0 ? uint32(devPlayerbotCharacterGuid) : 0);
    int32 devPlayerbotAccountId2 = sConfigMgr->GetIntDefault("Playerbots.Dev.AccountId2", 0);
    int32 devPlayerbotCharacterGuid2 = sConfigMgr->GetIntDefault("Playerbots.Dev.CharacterGuid2", 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_ACCOUNT_ID_2, devPlayerbotAccountId2 > 0 ? uint32(devPlayerbotAccountId2) : 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_CHARACTER_GUID_2, devPlayerbotCharacterGuid2 > 0 ? uint32(devPlayerbotCharacterGuid2) : 0);
    int32 devPlayerbotAccountId3 = sConfigMgr->GetIntDefault("Playerbots.Dev.AccountId3", 0);
    int32 devPlayerbotCharacterGuid3 = sConfigMgr->GetIntDefault("Playerbots.Dev.CharacterGuid3", 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_ACCOUNT_ID_3, devPlayerbotAccountId3 > 0 ? uint32(devPlayerbotAccountId3) : 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_CHARACTER_GUID_3, devPlayerbotCharacterGuid3 > 0 ? uint32(devPlayerbotCharacterGuid3) : 0);
    int32 devPlayerbotAccountId4 = sConfigMgr->GetIntDefault("Playerbots.Dev.AccountId4", 0);
    int32 devPlayerbotCharacterGuid4 = sConfigMgr->GetIntDefault("Playerbots.Dev.CharacterGuid4", 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_ACCOUNT_ID_4, devPlayerbotAccountId4 > 0 ? uint32(devPlayerbotAccountId4) : 0);
    world.setIntConfig(CONFIG_PLAYERBOTS_DEV_CHARACTER_GUID_4, devPlayerbotCharacterGuid4 > 0 ? uint32(devPlayerbotCharacterGuid4) : 0);

}
