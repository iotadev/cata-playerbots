/*
 * Adapted from AzerothCore mod-playerbots Action.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_ACTION_H
#define PLAYERBOTS_ACTION_H

#include "../WorldPacket/Event.h"
#include "NextAction.h"
#include "../PlayerbotAIAware.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class PlayerbotAI;

// Scheduling surface of the donor Action. Player-bound target lookup is added
// with AiObjectContext; this class does not own or dereference a bot.
class Action : public PlayerbotAIAware
{
public:
    enum class ActionThreatType { None = 0, Single = 1, Aoe = 2 };

    Action(PlayerbotAI* botAI, std::string name = "action")
        : PlayerbotAIAware(botAI), name(std::move(name)) { }
    virtual ~Action() = default;

    virtual bool Execute([[maybe_unused]] Event event) { return true; }
    virtual bool isUseful() { return true; }
    virtual bool isPossible() { return true; }
    virtual std::vector<NextAction> getPrerequisites() { return {}; }
    virtual std::vector<NextAction> getAlternatives() { return {}; }
    virtual std::vector<NextAction> getContinuers() { return {}; }
    virtual ActionThreatType getThreatType() { return ActionThreatType::None; }
    void Update() { }
    void Reset() { }
    void MakeVerbose() { verbose = true; }
    void setRelevance(float value) { relevance = value; }
    virtual float getRelevance() { return relevance; }
    std::string const getName() { return name; }

protected:
    bool verbose = false;
    float relevance = 0.0f;
    std::string const name;
};

// ActionNode owns no Action. The donor Engine/context owns those objects.
class ActionNode
{
public:
    ActionNode(std::string name, std::vector<NextAction> prerequisites = {},
               std::vector<NextAction> alternatives = {}, std::vector<NextAction> continuers = {})
        : name(std::move(name)), continuers(std::move(continuers)),
          alternatives(std::move(alternatives)), prerequisites(std::move(prerequisites)) { }
    virtual ~ActionNode() = default;

    Action* getAction() { return action; }
    void setAction(Action* value) { action = value; }
    std::string const getName() { return name; }
    std::vector<NextAction> getContinuers();
    std::vector<NextAction> getAlternatives();
    std::vector<NextAction> getPrerequisites();

private:
    std::string const name;
    Action* action = nullptr;
    std::vector<NextAction> continuers;
    std::vector<NextAction> alternatives;
    std::vector<NextAction> prerequisites;
};

class ActionBasket
{
public:
    ActionBasket(ActionNode* action, float relevance, bool skipPrerequisites, Event event);

    float getRelevance() const { return relevance; }
    ActionNode* getAction() const { return action; }
    Event getEvent() const { return event; }
    bool isSkipPrerequisites() const { return skipPrerequisites; }
    void AmendRelevance(float value) { relevance *= value; }
    void setRelevance(float value) { relevance = value; }
    bool isExpired(uint32_t msecs) const;

private:
    ActionNode* action;
    float relevance;
    bool skipPrerequisites;
    Event event;
    uint32_t created;
};

#endif
