/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS.md
 * for copyright information; released under GNU GPL v2 or any later version.
 * Ported from 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.
 */
#ifndef PLAYERBOTS_PLAYERBOTAIAWARE_H
#define PLAYERBOTS_PLAYERBOTAIAWARE_H

class PlayerbotAI;

class PlayerbotAIAware
{
public:
    explicit PlayerbotAIAware(PlayerbotAI* botAI) : botAI(botAI) { }

protected:
    PlayerbotAI* botAI;
};

#endif
