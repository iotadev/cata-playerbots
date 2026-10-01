/*
 * Adapted from mod-playerbots RandomPlayerbotFactory::CreateRandomBot at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "RandomPlayerbotFactory.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include <limits>

std::optional<PlayerbotAppearance> RandomPlayerbotFactory::PreviewAppearance(
    uint8_t race, uint8_t classId, uint8_t gender, std::string& failure)
{
    failure.clear();
    auto reject = [&failure](char const* reason) -> std::optional<PlayerbotAppearance>
    {
        failure = reason;
        return std::nullopt;
    };
    if (!race || race > 32 || !classId || classId > 32 || gender > GENDER_FEMALE)
        return reject("invalid race, class or gender");
    auto raceEntry = sChrRacesStore.LookupEntry(race);
    auto classEntry = sChrClassesStore.LookupEntry(classId);
    if (!raceEntry || !classEntry || !sObjectMgr->GetPlayerInfo(race, classId))
        return reject("race/class pair has no native Cata player creation data");
    uint32 expansion = sWorld->getIntConfig(CONFIG_EXPANSION);
    if (raceEntry->Race_related > expansion || classEntry->Required_expansion > expansion)
        return reject("race/class requires a disabled expansion");
    if ((sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK) & (uint32(1) << (race - 1))) ||
        (sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK) & (uint32(1) << (classId - 1))))
        return reject("race/class creation is disabled");
    uint32 teamBit = Player::TeamForRace(race) == ALLIANCE ? 1u : 2u;
    if (sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED) & teamBit)
        return reject("faction creation is disabled");

    std::vector<PlayerbotAppearanceSection> faces, hairs, facialHair;
    for (CharSectionsEntry const* section : sCharSectionsStore)
    {
        if (section->RaceID != race || section->SexID != gender || !(section->Flags & SECTION_FLAG_PLAYER) ||
            ((section->Flags & SECTION_FLAG_DEATH_KNIGHT) && classId != CLASS_DEATH_KNIGHT) ||
            section->VariationIndex > std::numeric_limits<uint8_t>::max() ||
            section->ColorIndex > std::numeric_limits<uint8_t>::max())
            continue;
        PlayerbotAppearanceSection choice{uint8_t(section->VariationIndex), uint8_t(section->ColorIndex)};
        switch (section->BaseSection)
        {
            case SECTION_TYPE_FACE: faces.push_back(choice); break;
            case SECTION_TYPE_HAIR: hairs.push_back(choice); break;
            case SECTION_TYPE_FACIAL_HAIR: facialHair.push_back(choice); break;
            default: break;
        }
    }
    // Native Player::ValidateAppearance uses the same no-facial-hair exceptions.
    bool excludesFacialHair = race == RACE_TAUREN || race == RACE_DRAENEI ||
        (gender == GENDER_FEMALE && race != RACE_NIGHTELF && race != RACE_UNDEAD_PLAYER);
    auto appearance = SelectPlayerbotAppearance(faces, hairs, facialHair, excludesFacialHair,
        [race, classId, gender](PlayerbotAppearance const& value)
        {
            return Player::ValidateAppearance(race, classId, gender, value.HairStyle,
                value.HairColor, value.Face, value.FacialHair, value.Skin, true);
        });
    if (!appearance)
        return reject("no appearance passed native Cata creation validation within the bounded search");
    return appearance;
}
