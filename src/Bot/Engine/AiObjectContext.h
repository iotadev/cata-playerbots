/*
 * Adapted from AzerothCore mod-playerbots AiObjectContext.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_AIOBJECTCONTEXT_H
#define PLAYERBOTS_AIOBJECTCONTEXT_H

#include "Action/Action.h"
#include "NamedObjectContext.h"
#include "PlayerbotAIAware.h"
#include "Strategy/Strategy.h"
#include "Trigger/Trigger.h"
#include "Value/Value.h"
#include <set>
#include <sstream>
#include <string>
#include <vector>

class AiObjectContext : public PlayerbotAIAware
{
public:
    AiObjectContext(PlayerbotAI* botAI,
                    SharedNamedObjectContextList<Strategy>& sharedStrategyContext = sharedStrategyContexts,
                    SharedNamedObjectContextList<Action>& sharedActionContext = sharedActionContexts,
                    SharedNamedObjectContextList<Trigger>& sharedTriggerContext = sharedTriggerContexts,
                    SharedNamedObjectContextList<UntypedValue>& sharedValueContext = sharedValueContexts);
    virtual ~AiObjectContext();

    virtual Strategy* GetStrategy(std::string const name);
    virtual std::set<std::string> GetSiblingStrategy(std::string const name);
    virtual Trigger* GetTrigger(std::string const name);
    virtual Action* GetAction(std::string const name);
    virtual UntypedValue* GetUntypedValue(std::string const name);

    template <class T>
    Value<T>* GetValue(std::string const name)
    {
        return dynamic_cast<Value<T>*>(GetUntypedValue(name));
    }

    template <class T>
    Value<T>* GetValue(std::string const name, std::string const param)
    {
        return GetValue<T>(name + "::" + param);
    }

    template <class T>
    Value<T>* GetValue(std::string const name, int32_t param)
    {
        return GetValue<T>(name, std::to_string(param));
    }

    std::set<std::string> GetValues();
    std::set<std::string> GetSupportedStrategies();
    std::set<std::string> GetSupportedActions();
    std::string const FormatValues();
    std::vector<std::string> Save();
    void Load(std::vector<std::string> const& data);

    std::vector<std::string> performanceStack;

protected:
    NamedObjectContextList<Strategy> strategyContexts;
    NamedObjectContextList<Action> actionContexts;
    NamedObjectContextList<Trigger> triggerContexts;
    NamedObjectContextList<UntypedValue> valueContexts;

private:
    static SharedNamedObjectContextList<Strategy> sharedStrategyContexts;
    static SharedNamedObjectContextList<Action> sharedActionContexts;
    static SharedNamedObjectContextList<Trigger> sharedTriggerContexts;
    static SharedNamedObjectContextList<UntypedValue> sharedValueContexts;
};

#endif
