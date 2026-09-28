/*
 * Adapted from AzerothCore mod-playerbots Action.h/Action.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "Action.h"
#include "Timer.h"

std::vector<NextAction> ActionNode::getContinuers()
{
    return action ? NextAction::merge(continuers, action->getContinuers()) : continuers;
}

std::vector<NextAction> ActionNode::getAlternatives()
{
    return action ? NextAction::merge(alternatives, action->getAlternatives()) : alternatives;
}

std::vector<NextAction> ActionNode::getPrerequisites()
{
    return action ? NextAction::merge(prerequisites, action->getPrerequisites()) : prerequisites;
}

ActionBasket::ActionBasket(ActionNode* action, float relevance, bool skipPrerequisites, Event event)
    : action(action), relevance(relevance), skipPrerequisites(skipPrerequisites),
      event(std::move(event)), created(getMSTime()) { }

bool ActionBasket::isExpired(uint32_t msecs) const
{
    return getMSTime() - created >= msecs;
}
