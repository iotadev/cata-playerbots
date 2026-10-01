/*
 * Adapted from mod-playerbots PlayerbotMgr account-link and PlayerbotSecurity
 * master relationships at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or later.
 */
#ifndef CATA_PLAYERBOTS_MANAGED_SECURITY_H
#define CATA_PLAYERBOTS_MANAGED_SECURITY_H

// Server-derived facts for a single request. Never populate these from addon fields.
struct PlayerbotManagedAccess
{
    bool Enabled = false;
    bool ValidIdentity = false;
    bool HumanInWorld = false;
    bool GameMaster = false;
    bool LinkedAccount = false;
    bool SameTeam = false;
    bool Grouped = false;
    bool FullPartyControl = false;

    bool MayStart() const
    {
        return Enabled && ValidIdentity && HumanInWorld && (GameMaster || (LinkedAccount && SameTeam));
    }

    bool MayStopOrList() const
    {
        return MayStart() && (GameMaster || !Grouped || FullPartyControl);
    }
};
#endif
