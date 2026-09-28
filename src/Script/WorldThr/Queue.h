/*
 * Adapted from AzerothCore mod-playerbots Script/WorldThr/Queue.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_QUEUE_H
#define PLAYERBOTS_QUEUE_H

#include "../../Bot/Engine/Action/Action.h"
#include <cstdint>
#include <list>

// Owns queued baskets and their ActionNodes until Pop transfers a node to the
// caller. Action objects remain owned by the future Engine/context.
class Queue
{
public:
    explicit Queue(uint32_t expiryTime = 0) : expiryTime(expiryTime) { }
    ~Queue() { Clear(); }
    Queue(Queue const&) = delete;
    Queue& operator=(Queue const&) = delete;

    void Push(ActionBasket* action);
    ActionNode* Pop();
    ActionBasket* Peek() const;
    uint32_t Size() const { return uint32_t(actions.size()); }
    void SetExpiration(uint32_t value) { expiryTime = value; }
    void RemoveExpired();
    void Clear();

private:
    void updateExistingBasket(ActionBasket* existing, ActionBasket* newBasket);
    ActionBasket* findHighestRelevanceBasket() const;
    ActionNode* extractAndDeleteBasket(ActionBasket* basket);

    std::list<ActionBasket*> actions;
    uint32_t expiryTime;
};

#endif
