/*
 * Adapted from AzerothCore mod-playerbots Engine.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "Engine.h"
#include "Timer.h"
#include <algorithm>
#include <unordered_map>

void Engine::AddActionExecutionListener(std::unique_ptr<ActionExecutionListener> listener)
{
    if (listener)
        listeners.push_back(std::move(listener));
}

bool Engine::ListenAndExecute(Action* action, Event event)
{
    bool before = true;
    for (auto const& listener : listeners)
        before &= listener->Before(action, event);
    bool executed = false;
    if (before)
    {
        bool allowed = true;
        for (auto const& listener : listeners)
            allowed &= listener->AllowExecution(action, event);
        executed = allowed ? action->Execute(event) : true;
    }
    for (auto const& listener : listeners)
        executed = listener->OverrideResult(action, executed, event);
    for (auto const& listener : listeners)
        listener->After(action, executed, event);
    return executed;
}

Engine::Engine(PlayerbotAI* botAI, AiObjectContext& context, uint32_t expiryMs)
    : PlayerbotAIAware(botAI), context(context), queue(expiryMs) { }

Engine::~Engine()
{
    Reset();
}

void Engine::Reset()
{
    queue.Clear();
    for (TriggerNode* trigger : triggers)
        delete trigger;
    triggers.clear();
    for (Multiplier* multiplier : multipliers)
        delete multiplier;
    multipliers.clear();
    actionNodeFactories.creators.clear();
    strategyTypeMask = 0;
    hasTargetExclusions = false;
}

void Engine::Init()
{
    Reset();
    for (auto const& [name, strategy] : strategies)
    {
        (void)name;
        strategyTypeMask |= strategy->GetType();
        hasTargetExclusions |= strategy->HasTargetExclusions();
        strategy->InitMultipliers(multipliers);
        strategy->InitTriggers(triggers);
        for (auto const& [actionName, creator] : strategy->actionNodeFactories.creators)
            actionNodeFactories.creators[actionName] = creator;
    }
}

void Engine::AddStrategy(std::string const& name)
{
    Strategy* strategy = context.GetStrategy(name);
    if (!strategy)
        return;
    for (std::string const& sibling : context.GetSiblingStrategy(name))
        strategies.erase(sibling);
    strategies[strategy->getName()] = strategy;
    Init();
}

bool Engine::RemoveStrategy(std::string const& name)
{
    if (!strategies.erase(name))
        return false;
    Init();
    return true;
}

bool Engine::HasStrategy(std::string const& name) const
{
    return strategies.find(name) != strategies.end();
}

bool Engine::ContainsStrategy(StrategyType type) const
{
    return (strategyTypeMask & type) != 0;
}

std::vector<std::string> Engine::GetStrategies() const
{
    std::vector<std::string> names;
    for (auto const& [name, strategy] : strategies)
    {
        (void)strategy;
        names.push_back(name);
    }
    return names;
}

ActionNode* Engine::CreateActionNode(std::string const& name)
{
    if (ActionNode* node = actionNodeFactories.GetContextObject(name, botAI))
        return node;
    return new ActionNode(name);
}

Action* Engine::InitializeAction(ActionNode* node)
{
    if (!node->getAction())
        node->setAction(context.GetAction(node->getName()));
    return node->getAction();
}

bool Engine::MultiplyAndPush(std::vector<NextAction> const& actions, float forcedRelevance,
                             bool skipPrerequisites, Event const& event)
{
    bool pushed = false;
    for (NextAction next : actions)
    {
        float relevance = forcedRelevance > 0 ? forcedRelevance : next.getRelevance();
        if (relevance <= 0)
            continue;
        ActionNode* node = CreateActionNode(next.getName());
        InitializeAction(node);
        queue.Push(new ActionBasket(node, relevance, skipPrerequisites, event));
        pushed = true;
    }
    return pushed;
}

void Engine::ProcessTriggers(bool minimal, bool forceRebuffPending, bool inCombat)
{
    std::unordered_map<Trigger*, Event> fired;
    uint32_t now = getMSTime();
    for (TriggerNode* node : triggers)
    {
        if (!node)
            continue;
        Trigger* trigger = node->getTrigger();
        if (!trigger)
        {
            trigger = context.GetTrigger(node->getName());
            node->setTrigger(trigger);
        }
        if (!trigger || fired.find(trigger) != fired.end())
            continue;
        if (minimal && node->getFirstRelevance() < 100)
            continue;
        if (!trigger->needCheck(now, forceRebuffPending, inCombat))
            continue;
        Event event = trigger->Check();
        if (!event)
            continue;
        fired.emplace(trigger, event);
    }
    for (TriggerNode* node : triggers)
    {
        if (node && node->getTrigger())
        {
            auto found = fired.find(node->getTrigger());
            if (found != fired.end())
                MultiplyAndPush(node->getHandlers(), 0.0f, false, found->second);
        }
    }
}

void Engine::PushDefaultActions()
{
    for (auto const& [name, strategy] : strategies)
    {
        (void)name;
        MultiplyAndPush(strategy->getDefaultActions(), 0.0f, false, Event());
    }
}

bool Engine::Tick(bool minimal, bool forceRebuffPending, bool inCombat, uint32_t iterationsPerTick)
{
    ProcessTriggers(minimal, forceRebuffPending, inCombat);
    if (!minimal)
        PushDefaultActions();

    uint32_t budget = queue.Size() * (minimal ? 2 : iterationsPerTick);
    for (uint32_t iteration = 0; iteration < budget; ++iteration)
    {
        ActionBasket* basket = queue.Peek();
        if (!basket || (minimal && basket->getRelevance() < 100))
            break;
        float relevance = basket->getRelevance();
        bool skipPrerequisites = basket->isSkipPrerequisites();
        Event event = basket->getEvent();
        std::unique_ptr<ActionNode> node(queue.Pop());
        Action* action = InitializeAction(node.get());
        if (!action || !action->isUseful())
            continue;

        for (Multiplier* multiplier : multipliers)
        {
            relevance *= multiplier->GetValue(action);
            if (relevance <= 0)
                break;
        }
        action->setRelevance(relevance);
        if (relevance > 0 && action->isPossible())
        {
            if (!skipPrerequisites && MultiplyAndPush(node->getPrerequisites(), relevance + 0.002f, false, event))
            {
                MultiplyAndPush({NextAction(node->getName(), relevance + 0.001f)},
                                relevance + 0.001f, true, event);
                continue;
            }
            if (ListenAndExecute(action, event))
            {
                lastAction = action->getName();
                MultiplyAndPush(node->getContinuers(), relevance, false, event);
                queue.RemoveExpired();
                return true;
            }
        }
        MultiplyAndPush(node->getAlternatives(), relevance + 0.003f, false, event);
    }
    queue.RemoveExpired();
    return false;
}

ActionResult Engine::ExecuteAction(std::string const& name, Event event)
{
    std::unique_ptr<ActionNode> node(CreateActionNode(name));
    Action* action = InitializeAction(node.get());
    if (!action)
        return ACTION_RESULT_UNKNOWN;
    if (!action->isUseful())
        return ACTION_RESULT_USELESS;
    if (!action->isPossible())
        return ACTION_RESULT_IMPOSSIBLE;
    action->MakeVerbose();
    bool executed = ListenAndExecute(action, event);
    if (executed)
        lastAction = action->getName();
    MultiplyAndPush(action->getContinuers(), 0.0f, false, event);
    return executed ? ACTION_RESULT_OK : ACTION_RESULT_FAILED;
}
