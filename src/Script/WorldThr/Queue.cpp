/*
 * Adapted from AzerothCore mod-playerbots Script/WorldThr/Queue.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "Queue.h"

void Queue::Push(ActionBasket* basket)
{
    if (!basket)
        return;
    if (!basket->getAction())
    {
        delete basket;
        return;
    }

    for (ActionBasket* existing : actions)
        if (existing->getAction()->getName() == basket->getAction()->getName())
        {
            updateExistingBasket(existing, basket);
            return;
        }

    actions.push_back(basket);
}

ActionNode* Queue::Pop()
{
    ActionBasket* basket = findHighestRelevanceBasket();
    return basket ? extractAndDeleteBasket(basket) : nullptr;
}

ActionBasket* Queue::Peek() const
{
    return findHighestRelevanceBasket();
}

void Queue::RemoveExpired()
{
    if (!expiryTime)
        return;

    for (auto it = actions.begin(); it != actions.end();)
    {
        ActionBasket* basket = *it;
        if (!basket->isExpired(expiryTime))
        {
            ++it;
            continue;
        }

        delete basket->getAction();
        delete basket;
        it = actions.erase(it);
    }
}

void Queue::Clear()
{
    for (ActionBasket* basket : actions)
    {
        delete basket->getAction();
        delete basket;
    }
    actions.clear();
}

void Queue::updateExistingBasket(ActionBasket* existing, ActionBasket* newBasket)
{
    if (existing->getRelevance() < newBasket->getRelevance())
        existing->setRelevance(newBasket->getRelevance());

    delete newBasket->getAction();
    delete newBasket;
}

ActionBasket* Queue::findHighestRelevanceBasket() const
{
    ActionBasket* selection = nullptr;
    for (ActionBasket* basket : actions)
        if (!selection || basket->getRelevance() > selection->getRelevance())
            selection = basket;
    return selection;
}

ActionNode* Queue::extractAndDeleteBasket(ActionBasket* basket)
{
    ActionNode* node = basket->getAction();
    actions.remove(basket);
    delete basket;
    return node;
}
