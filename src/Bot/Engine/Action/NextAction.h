/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */
// Ported from mod-playerbots 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.

// Extracted unchanged from donor Engine/Action/Action.h to break umbrella dependencies.
#ifndef PLAYERBOTS_NEXTACTION_H
#define PLAYERBOTS_NEXTACTION_H

#include <string>
#include <vector>

class NextAction
{
public:
    NextAction(std::string const name, float relevance = 0.0f)
        : relevance(relevance), name(name) {}

    std::string const getName() { return name; }
    float getRelevance() { return relevance; }

    static std::vector<NextAction> merge(std::vector<NextAction> const& what, std::vector<NextAction> const& with)
    {
        std::vector<NextAction> result = {};

        for (NextAction const& action : what)
            result.push_back(action);

        for (NextAction const& action : with)
            result.push_back(action);

        return result;
    };

private:
    float relevance;
    std::string name;
};

#endif
