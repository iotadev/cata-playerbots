/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "ScriptMgr.h"
#include "Log.h"
#include "World.h"

namespace
{
class PlayerbotsModuleWorldScript final : public WorldScript
{
public:
    PlayerbotsModuleWorldScript() : WorldScript("PlayerbotsModuleWorldScript") { }
    void OnStartup() override
    {
        TC_LOG_INFO("server.loading", "Optional mod-playerbots loaded; development admission %s",
            sWorld->getBoolConfig(CONFIG_PLAYERBOTS_DEV_ENABLED) ? "enabled" : "disabled");
    }
};
}

void AddSC_playerbots_module()
{
    new PlayerbotsModuleWorldScript();
}
