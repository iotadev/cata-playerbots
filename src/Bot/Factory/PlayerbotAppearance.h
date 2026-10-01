/*
 * Adapted from mod-playerbots RandomPlayerbotFactory::CreateRandomBot at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOT_APPEARANCE_H
#define CATA_PLAYERBOT_APPEARANCE_H

#include <cstdint>
#include <optional>
#include <vector>

struct PlayerbotAppearanceSection
{
    uint8_t Type;
    uint8_t Color;
};

struct PlayerbotAppearance
{
    uint8_t Skin;
    uint8_t Face;
    uint8_t HairStyle;
    uint8_t HairColor;
    uint8_t FacialHair;
};

// The caller supplies Cata data and the native appearance validator. No random
// indexing, database writes or cached success when the data is incomplete.
template<class Validator>
std::optional<PlayerbotAppearance> SelectPlayerbotAppearance(
    std::vector<PlayerbotAppearanceSection> const& faces,
    std::vector<PlayerbotAppearanceSection> const& hairs,
    std::vector<PlayerbotAppearanceSection> const& facialHair,
    bool excludesFacialHair, Validator validate)
{
    constexpr std::size_t MaxSections = 4096;
    constexpr std::size_t MaxAttempts = 65536;
    if (faces.empty() || hairs.empty() || (!excludesFacialHair && facialHair.empty()) ||
        faces.size() > MaxSections || hairs.size() > MaxSections || facialHair.size() > MaxSections)
        return std::nullopt;

    std::size_t attempts = 0;
    for (auto const& face : faces)
        for (auto const& hair : hairs)
        {
            PlayerbotAppearance candidate{face.Color, face.Type, hair.Type, hair.Color, 0};
            if (excludesFacialHair)
            {
                if (++attempts > MaxAttempts)
                    return std::nullopt;
                if (validate(candidate))
                    return candidate;
            }
            else
                for (auto const& facial : facialHair)
                {
                    if (++attempts > MaxAttempts)
                        return std::nullopt;
                    if (facial.Color != hair.Color)
                        continue;
                    candidate.FacialHair = facial.Type;
                    if (validate(candidate))
                        return candidate;
                }
        }
    return std::nullopt;
}

#endif
