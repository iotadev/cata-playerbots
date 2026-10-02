/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotSessionHooks.h"
#include "PlayerbotConfig.h"
#include "PlayerbotDevFixture.h"
#include "PlayerbotManagedRoster.h"
#include "PlayerbotAddonProtocol.h"
#include "RandomPlayerbotFactory.h"
#include "Config.h"
#include "Log.h"
#include "World.h"
#include <atomic>
namespace { std::atomic<bool> RestEnabled { false }; }
namespace { std::atomic<bool> MageArmorEnabled { false }; }
namespace { std::atomic<bool> LootPassEnabled { false }; }
namespace { std::atomic<bool> CorpseLootEnabled { false }; }
bool PlayerbotModuleCorpseLootEnabled() { return CorpseLootEnabled.load(std::memory_order_relaxed); }
bool PlayerbotModuleLootPassEnabled() { return LootPassEnabled.load(std::memory_order_relaxed); }
bool PlayerbotModuleMageArmorEnabled() { return MageArmorEnabled.load(std::memory_order_relaxed); }
bool PlayerbotModuleRestEnabled() { return RestEnabled.load(std::memory_order_relaxed); }
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

bool PlayerbotModuleEnginePartyBuffEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EnginePartyBuff", false);
}

bool PlayerbotModuleEnginePriestHealEnabled()
{
    return sConfigMgr->GetBoolDefault("Playerbots.Dev.EnginePriestHeal", false);
}

void LoadPlayerbotModuleSettings(World& world, bool moduleConfigsValid)
{
    PlayerbotDevFixture::SetEnabled(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Dev.Enabled", false) &&
        sConfigMgr->GetBoolDefault("Playerbots.Dev.Fixture20.Enabled", false));
    CorpseLootEnabled.store(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Loot.Corpses.Enabled", false),
        std::memory_order_relaxed);
    LootPassEnabled.store(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Loot.PassOnGroupLoot", false),
        std::memory_order_relaxed);
    MageArmorEnabled.store(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Mage.Armor.Enabled", false),
        std::memory_order_relaxed);
    RestEnabled.store(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Rest.Enabled", false),
        std::memory_order_relaxed);
    bool factoryEnabled = moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Factory.Enabled", false);
    RandomPlayerbotFactory::SetProvisioningEnabled(factoryEnabled);
    RandomPlayerbotFactory::SetInspectionEnabled(factoryEnabled || (moduleConfigsValid &&
        sConfigMgr->GetBoolDefault("Playerbots.Factory.InspectionEnabled", false)));
    SetPlayerbotAddonBridgeEnabled(moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.MultiBot.Enabled", false));
    world.setBoolConfig(CONFIG_PLAYERBOTS_DEV_ENABLED, moduleConfigsValid && sConfigMgr->GetBoolDefault("Playerbots.Dev.Enabled", false));
    world.setBoolConfig(CONFIG_PLAYERBOTS_DEV_GREETING_ENABLED, sConfigMgr->GetBoolDefault("Playerbots.Dev.GreetingOnJoin", true));
    bool managedRosterValid = PlayerbotManagedRoster::Configure(sConfigMgr->GetStringDefault("Playerbots.Managed.Characters", ""));
    if (!managedRosterValid)
        TC_LOG_ERROR("module.playerbots", "Playerbots.Managed.Characters has invalid or duplicate account:character bindings; managed admission disabled.");
    world.setBoolConfig(CONFIG_PLAYERBOTS_MANAGED_ENABLED, moduleConfigsValid && managedRosterValid &&
        sConfigMgr->GetBoolDefault("Playerbots.Managed.Enabled", false));
    bool accountLinksValid = PlayerbotManagedRoster::ConfigureAccountLinks(
        sConfigMgr->GetStringDefault("Playerbots.Managed.AccountLinks", ""));
    if (!accountLinksValid)
        TC_LOG_ERROR("module.playerbots", "Playerbots.Managed.AccountLinks is invalid; player lifecycle access disabled.");
    PlayerbotManagedRoster::SetPlayerControlEnabled(moduleConfigsValid && managedRosterValid && accountLinksValid &&
        sConfigMgr->GetBoolDefault("Playerbots.Managed.AllowPlayerControl", false));
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
