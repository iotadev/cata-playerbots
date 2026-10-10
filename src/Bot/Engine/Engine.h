/*
 * Adapted from AzerothCore mod-playerbots Engine.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_ENGINE_H
#define PLAYERBOTS_ENGINE_H

#include "AiObjectContext.h"
#include "Multiplier.h"
#include "ActionTrace.h"
#include "Strategy/Strategy.h"
#include "../../Script/WorldThr/Queue.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum ActionResult
{
    ACTION_RESULT_UNKNOWN,
    ACTION_RESULT_OK,
    ACTION_RESULT_IMPOSSIBLE,
    ACTION_RESULT_USELESS,
    ACTION_RESULT_FAILED
};

class ActionExecutionListener
{
public:
    virtual ~ActionExecutionListener() = default;
    virtual bool Before(Action* action, Event event) = 0;
    virtual bool AllowExecution(Action* action, Event event) = 0;
    virtual void After(Action* action, bool executed, Event event) = 0;
    virtual bool OverrideResult(Action* action, bool executed, Event event) = 0;
};

// The donor's decision engine, isolated from PlayerbotAI's live player/session
// methods. The session bridge will supply state and call Tick on the map thread.
class Engine : public PlayerbotAIAware
{
public:
    enum class StrategyChangeStatus { Rejected, Unchanged, Changed };
    struct StrategyChangeResult
    {
        StrategyChangeStatus Status = StrategyChangeStatus::Rejected;
        bool Query = false;
    };
    Engine(PlayerbotAI* botAI, AiObjectContext& context, uint32_t expiryMs = 0);
    ~Engine();
    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;

    void Init();
    void Activate();
    void CancelPendingActions() { queue.Clear(); lastAction.clear(); }
    void AddStrategy(std::string const& name);
    bool RemoveStrategy(std::string const& name);
    bool HasStrategy(std::string const& name) const;
    bool ContainsStrategy(StrategyType type) const;
    std::vector<std::string> GetStrategies() const;
    // Map-thread-only donor operator layer. The caller supplies a state-specific
    // mutable allowlist; registration alone does not authorize changing a strategy.
    // No chat transport, persistence, reset, or session-role override is implied.
    StrategyChangeResult ChangeStrategies(std::string const& command,
                                         std::set<std::string> const& mutableStrategies);
    uint32_t GetStrategyTypeMask() const { return strategyTypeMask; }
    bool HasTargetExclusions() const { return hasTargetExclusions; }
    GuidSet GatherTargetExclusions(TargetValueExclusionType type) const;

    bool Tick(bool minimal = false, bool forceRebuffPending = false, bool inCombat = false,
              uint32_t iterationsPerTick = 10);
    ActionResult ExecuteAction(std::string const& name, Event event = Event());
    // Engine owns registered listeners, as in the donor. Register only before
    // starting map-thread ticks; registration is not thread-safe.
    void AddActionExecutionListener(std::unique_ptr<ActionExecutionListener> listener);
    std::string const& GetLastAction() const { return lastAction; }
    uint32_t QueuedCount() const { return queue.Size(); }
    void SetTraceObserver(std::function<void(ActionTraceRecord)> observer) { traceObserver = std::move(observer); }

private:
    void Reset();
    void ProcessTriggers(bool minimal, bool forceRebuffPending, bool inCombat);
    void PushDefaultActions();
    bool MultiplyAndPush(std::vector<NextAction> const& actions, float forcedRelevance,
                         bool skipPrerequisites, Event const& event);
    ActionNode* CreateActionNode(std::string const& name);
    Action* InitializeAction(ActionNode* node);
    bool ListenAndExecute(Action* action, Event event);
    void Trace(std::string const& action, char const* kind, char const* reason, bool called = false,
        std::optional<bool> actionReturn = {}, std::optional<bool> engineResult = {}) noexcept;

    AiObjectContext& context;
    Queue queue;
    std::map<std::string, Strategy*> strategies; // borrowed from context
    std::vector<TriggerNode*> triggers;          // owned here
    std::vector<Multiplier*> multipliers;         // owned here
    NamedObjectFactoryList<ActionNode> actionNodeFactories;
    std::vector<std::unique_ptr<ActionExecutionListener>> listeners;
    uint32_t strategyTypeMask = 0;
    bool hasTargetExclusions = false;
    std::string lastAction;
    std::function<void(ActionTraceRecord)> traceObserver;
};

#endif
