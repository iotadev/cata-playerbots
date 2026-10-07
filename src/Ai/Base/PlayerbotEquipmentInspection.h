/* GPL v2 or later. Read-only inspection adaptation; see PORTING.md. */
#ifndef PLAYERBOT_EQUIPMENT_INSPECTION_H
#define PLAYERBOT_EQUIPMENT_INSPECTION_H
#include "PlayerbotEquipment.h"
#include <iomanip>
#include <locale>
#include <sstream>
namespace PlayerbotEquipmentInspection
{
inline bool Due(bool enabled, bool priorEnabled, bool sameIdentity, uint32 now, uint32 built)
{
    return enabled && (!priorEnabled || !sameIdentity || uint32(now - built) >= 2000);
}
inline bool Fresh(bool enabled, bool publishedEnabled, uint32 now, uint32 built)
{
    return enabled && publishedEnabled && uint32(now - built) < 5000;
}
inline char const* Label(PlayerbotEquipment::Decision decision)
{
    using PlayerbotEquipment::Decision;
    switch (decision)
    {
        case Decision::Keep: return "keep";
        case Decision::Upgrade: return "upgrade";
        case Decision::FillSlot: return "empty-slot candidate";
        case Decision::ReplaceBroken: return "healthy replacement for broken gear";
        case Decision::NeedsRepair: return "candidate needs repair";
        default: return "unknown/incomplete";
    }
}
inline std::vector<std::string> Rows(PlayerbotEquipment::Survey const& survey)
{
    std::vector<std::string> rows;
    if (!survey.Available) return rows;
    for (auto const& item : survey.Items)
    {
        if (rows.size() == 6) break;
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << "slot " << unsigned(item.Input.Slot) << ": item " << item.Input.Entry << " vs " << item.Input.ExistingEntry
            << " - " << Label(item.Result) << " (" << std::fixed << std::setprecision(2);
        if (item.CandidateScore) text << *item.CandidateScore; else text << "?";
        text << "/";
        if (item.ExistingScore) text << *item.ExistingScore; else text << "?";
        text << ")";
        rows.push_back(text.str());
    }
    return rows;
}
}
#endif
