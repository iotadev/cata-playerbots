/*
 * Adapted from AzerothCore mod-playerbots Multiplier.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_MULTIPLIER_H
#define PLAYERBOTS_MULTIPLIER_H

#include "PlayerbotAIAware.h"
#include <string>
#include <utility>

class Action;

class Multiplier : public PlayerbotAIAware
{
public:
    Multiplier(PlayerbotAI* ai, std::string name) : PlayerbotAIAware(ai), name(std::move(name)) { }
    virtual ~Multiplier() = default;
    virtual float GetValue([[maybe_unused]] Action* action) { return 1.0f; }
    std::string const getName() const { return name; }

private:
    std::string name;
};

#endif
