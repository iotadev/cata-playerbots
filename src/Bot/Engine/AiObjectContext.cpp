/*
 * Adapted from AzerothCore mod-playerbots AiObjectContext.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "AiObjectContext.h"
#include "../PlayerbotAI.h"

SharedNamedObjectContextList<Strategy> AiObjectContext::sharedStrategyContexts;
SharedNamedObjectContextList<Action> AiObjectContext::sharedActionContexts;
SharedNamedObjectContextList<Trigger> AiObjectContext::sharedTriggerContexts;
SharedNamedObjectContextList<UntypedValue> AiObjectContext::sharedValueContexts;

AiObjectContext::AiObjectContext(PlayerbotAI* botAI,
                                 SharedNamedObjectContextList<Strategy>& sharedStrategyContext,
                                 SharedNamedObjectContextList<Action>& sharedActionContext,
                                 SharedNamedObjectContextList<Trigger>& sharedTriggerContext,
                                 SharedNamedObjectContextList<UntypedValue>& sharedValueContext)
    : PlayerbotAIAware(botAI), strategyContexts(sharedStrategyContext),
      actionContexts(sharedActionContext), triggerContexts(sharedTriggerContext),
      valueContexts(sharedValueContext)
{
    if (botAI) botAI->SetAiObjectContext(this);
}
AiObjectContext::~AiObjectContext()
{
    if (botAI && botAI->GetAiObjectContext() == this)
        botAI->SetAiObjectContext(nullptr);
}

Strategy* AiObjectContext::GetStrategy(std::string const name)
{
    return strategyContexts.GetContextObject(name, botAI);
}

std::set<std::string> AiObjectContext::GetSiblingStrategy(std::string const name)
{
    return strategyContexts.GetSiblings(name);
}

Trigger* AiObjectContext::GetTrigger(std::string const name)
{
    return triggerContexts.GetContextObject(name, botAI);
}

Action* AiObjectContext::GetAction(std::string const name)
{
    return actionContexts.GetContextObject(name, botAI);
}

UntypedValue* AiObjectContext::GetUntypedValue(std::string const name)
{
    return valueContexts.GetContextObject(name, botAI);
}

std::set<std::string> AiObjectContext::GetValues()
{
    return valueContexts.GetCreated();
}

std::set<std::string> AiObjectContext::GetSupportedStrategies()
{
    return strategyContexts.supports();
}

std::set<std::string> AiObjectContext::GetSupportedActions()
{
    return actionContexts.supports();
}

std::string const AiObjectContext::FormatValues()
{
    std::ostringstream out;
    bool first = true;
    for (std::string const& name : valueContexts.GetCreated())
    {
        UntypedValue* value = GetUntypedValue(name);
        if (!value || value->Format() == "?")
            continue;
        if (!first)
            out << "|";
        out << "{" << name << "=" << value->Format() << "}";
        first = false;
    }
    return out.str();
}

std::vector<std::string> AiObjectContext::Save()
{
    std::vector<std::string> result;
    for (std::string const& name : valueContexts.GetCreated())
    {
        UntypedValue* value = GetUntypedValue(name);
        if (!value)
            continue;
        std::string data = value->Save();
        if (data != "?")
            result.push_back(name + ">" + data);
    }
    return result;
}

void AiObjectContext::Load(std::vector<std::string> const& data)
{
    for (std::string const& row : data)
    {
        size_t separator = row.find('>');
        if (separator == std::string::npos || row.find('>', separator + 1) != std::string::npos)
            continue;

        UntypedValue* value = GetUntypedValue(row.substr(0, separator));
        if (value)
            value->Load(row.substr(separator + 1));
    }
}
