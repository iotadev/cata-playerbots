/*
 * Adapted from mod-playerbots PlayerbotAI::ChangeEngine and AiFactory's
 * separate combat/noncombat/dead engines at 037c01418b5d01506917a3db9b44fd56ac5f965c.
 * See AUTHORS.md and PORTING.md. GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_STATE_ENGINES_H
#define PLAYERBOTS_STATE_ENGINES_H
#include "Engine.h"
#include <array>

// Map-thread only. Context objects are shared, strategy registrations and
// pending actions are not. The session owns native death/resurrection/transfer.
class StateEngines
{
public:
    enum class State : size_t { NonCombat, Combat, Dead };
    StateEngines(PlayerbotAI* ai, AiObjectContext& context)
    {
        for (auto& engine : engines)
            engine = std::make_unique<Engine>(ai, context);
        Active().Activate();
    }
    Engine& Get(State state) { return *engines[static_cast<size_t>(state)]; }
    Engine& Active() { return Get(state); }
    State Current() const { return state; }
    bool Select(State next)
    {
        if (state == next) return false;
        CancelPendingActions();
        state = next;
        Active().Activate();
        return true;
    }
    void CancelPendingActions()
    {
        for (auto& engine : engines) engine->CancelPendingActions();
    }
private:
    std::array<std::unique_ptr<Engine>, 3> engines;
    State state = State::NonCombat;
};
#endif
