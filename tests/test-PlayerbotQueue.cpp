/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Action/Action.h"
#include "../src/Script/WorldThr/Queue.h"
#include <catch2/catch.hpp>
#include <memory>

namespace
{
class HandlerAction final : public Action
{
public:
    HandlerAction() : Action(nullptr, "handler") { }
    std::vector<NextAction> getPrerequisites() override { return { NextAction("from handler", 3.0f) }; }
    std::vector<NextAction> getAlternatives() override { return { NextAction("fallback", 2.0f) }; }
    std::vector<NextAction> getContinuers() override { return { NextAction("continue", 1.0f) }; }
};

class CountedNode final : public ActionNode
{
public:
    CountedNode(std::string name, int& destroyed) : ActionNode(std::move(name)), destroyed(destroyed) { }
    ~CountedNode() override { ++destroyed; }
private:
    int& destroyed;
};
}

TEST_CASE("Playerbot action nodes preserve donor order and duplicate handlers", "[playerbot][engine][queue]")
{
    HandlerAction action;
    ActionNode node("spell", { NextAction("from node", 4.0f), NextAction("from handler", 5.0f) },
                    { NextAction("another", 6.0f) }, { NextAction("first", 7.0f) });
    node.setAction(&action);

    auto prerequisites = node.getPrerequisites();
    REQUIRE(prerequisites.size() == 3);
    REQUIRE(prerequisites[0].getName() == "from node");
    REQUIRE(prerequisites[1].getName() == "from handler");
    REQUIRE(prerequisites[2].getName() == "from handler");
    REQUIRE(prerequisites[2].getRelevance() == 3.0f);
    REQUIRE(node.getAlternatives().size() == 2);
    REQUIRE(node.getContinuers().size() == 2);
}

TEST_CASE("Playerbot queue pops highest relevance and transfers node ownership", "[playerbot][engine][queue]")
{
    int destroyed = 0;
    Queue queue;
    queue.Push(new ActionBasket(new CountedNode("low", destroyed), 2.0f, false, Event("low")));
    queue.Push(new ActionBasket(new CountedNode("high", destroyed), 8.0f, true, Event("high")));
    REQUIRE(queue.Size() == 2);
    REQUIRE(queue.Peek()->getEvent().GetSource() == "high");
    REQUIRE(queue.Peek()->isSkipPrerequisites());

    std::unique_ptr<ActionNode> highest(queue.Pop());
    REQUIRE(highest->getName() == "high");
    REQUIRE(queue.Size() == 1);
    REQUIRE(destroyed == 0);
    highest.reset();
    REQUIRE(destroyed == 1);
    queue.Clear();
    REQUIRE(destroyed == 2);
}

TEST_CASE("Playerbot queue merges duplicate names without replacing the queued event", "[playerbot][engine][queue]")
{
    int destroyed = 0;
    Queue queue;
    queue.Push(new ActionBasket(new CountedNode("spell", destroyed), 2.0f, false, Event("first")));
    queue.Push(new ActionBasket(new CountedNode("spell", destroyed), 9.0f, false, Event("second")));
    REQUIRE(queue.Size() == 1);
    REQUIRE(destroyed == 1);
    REQUIRE(queue.Peek()->getRelevance() == 9.0f);
    REQUIRE(queue.Peek()->getEvent().GetSource() == "first");
    queue.Clear();
    REQUIRE(destroyed == 2);
}

TEST_CASE("Playerbot queue retains negative relevance and clears pending nodes", "[playerbot][engine][queue]")
{
    int destroyed = 0;
    Queue queue;
    queue.Push(new ActionBasket(new CountedNode("negative", destroyed), -2.0f, false, Event("test")));
    REQUIRE(queue.Peek() != nullptr);
    std::unique_ptr<ActionNode> popped(queue.Pop());
    REQUIRE(popped->getName() == "negative");
    // Pop transfers ownership even when relevance is negative.
    REQUIRE(destroyed == 0);
    popped.reset();
    REQUIRE(destroyed == 1);
}
