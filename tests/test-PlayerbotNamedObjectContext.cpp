/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/NamedObjectContext.h"
#include <catch2/catch.hpp>
#include <memory>
#include <type_traits>

namespace
{
class RegistryObject : public Qualified
{
public:
    explicit RegistryObject(int& alive, int kind = 0) : _alive(alive), kind(kind) { ++_alive; }
    virtual ~RegistryObject() { --_alive; }
    int kind;
private:
    int& _alive;
};
}

static_assert(!std::is_copy_constructible<NamedObjectContext<RegistryObject>>::value);
static_assert(!std::is_copy_constructible<SharedNamedObjectContextList<RegistryObject>>::value);
static_assert(!std::is_copy_constructible<NamedObjectContextList<RegistryObject>>::value);
static_assert(!std::is_copy_constructible<NamedObjectFactoryList<RegistryObject>>::value);

TEST_CASE("Playerbot registry preserves qualified names and independent factory objects", "[playerbot][engine]")
{
    int alive = 0;
    NamedObjectFactory<RegistryObject> factory;
    factory.creators["target"] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
    std::unique_ptr<RegistryObject> first(factory.create("target::party::tank", nullptr));
    std::unique_ptr<RegistryObject> second(factory.create("target::party::tank", nullptr));
    REQUIRE(first);
    REQUIRE(first->getQualifier() == "party::tank");
    REQUIRE(second.get() != first.get());
    REQUIRE(factory.create("missing", nullptr) == nullptr);
    REQUIRE(factory.supports() == std::set<std::string>{"target"});
    REQUIRE(alive == 2);
}

TEST_CASE("Playerbot registry caches objects per bot and full qualifier", "[playerbot][engine]")
{
    int alive = 0;
    {
        SharedNamedObjectContextList<RegistryObject> shared;
        auto* factory = new NamedObjectContext<RegistryObject>();
        factory->creators["value"] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
        shared.Add(factory);
        {
            NamedObjectContextList<RegistryObject> first(shared);
            NamedObjectContextList<RegistryObject> second(shared);
            auto* firstValue = first.GetContextObject("value::1", nullptr);
            REQUIRE(firstValue == first.GetContextObject("value::1", nullptr));
            REQUIRE(firstValue != first.GetContextObject("value::2", nullptr));
            REQUIRE(firstValue != second.GetContextObject("value::1", nullptr));
            REQUIRE(firstValue->getQualifier() == "1");
            REQUIRE(alive == 3);
            REQUIRE(first.GetContextObject("missing", nullptr) == nullptr);
            REQUIRE(first.GetContextObject("missing", nullptr) == nullptr);
        }
        REQUIRE(alive == 0);
    }
    REQUIRE(alive == 0);
}

TEST_CASE("Playerbot registry preserves sibling strategy groups", "[playerbot][engine]")
{
    int alive = 0;
    SharedNamedObjectContextList<RegistryObject> shared;
    auto* movement = new NamedObjectContext<RegistryObject>(false, true);
    for (std::string const name : {"follow", "stay", "guard"})
        movement->creators[name] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
    shared.Add(movement);
    auto* unrelated = new NamedObjectContext<RegistryObject>();
    unrelated->creators["buff"] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
    shared.Add(unrelated);
    NamedObjectContextList<RegistryObject> context(shared);
    REQUIRE(context.GetSiblings("follow") == std::set<std::string>{"guard", "stay"});
    REQUIRE(context.GetSiblings("buff").empty());
    REQUIRE(context.GetSiblings("unknown").empty());
    REQUIRE(alive == 0);
}

TEST_CASE("Playerbot later registrations override creators without sharing bot objects", "[playerbot][engine]")
{
    int alive = 0;
    SharedNamedObjectContextList<RegistryObject> shared;
    auto* base = new NamedObjectContext<RegistryObject>();
    base->creators["cast"] = [&](PlayerbotAI*) { return new RegistryObject(alive, 1); };
    shared.Add(base);
    auto* specific = new NamedObjectContext<RegistryObject>();
    specific->creators["cast"] = [&](PlayerbotAI*) { return new RegistryObject(alive, 2); };
    shared.Add(specific);
    NamedObjectContextList<RegistryObject> context(shared);
    REQUIRE(context.GetContextObject("cast", nullptr)->kind == 2);
    REQUIRE(context.supports() == std::set<std::string>{"cast"});
    REQUIRE(alive == 1);
}

TEST_CASE("Playerbot local context clear destroys cached instances exactly once", "[playerbot][engine]")
{
    int alive = 0;
    {
        NamedObjectContext<RegistryObject> context;
        context.creators["action"] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
        auto* object = context.create("action", nullptr);
        REQUIRE(context.create("action", nullptr) == object);
        REQUIRE(alive == 1);
        context.Clear();
        REQUIRE(alive == 0);
        REQUIRE(context.GetCreated().empty());
        context.Clear();
        REQUIRE(context.create("action", nullptr));
        REQUIRE(alive == 1);
    }
    REQUIRE(alive == 0);
}

TEST_CASE("Playerbot action node factory list returns caller-owned uncached objects", "[playerbot][engine]")
{
    int alive = 0;
    {
        NamedObjectFactoryList<RegistryObject> factories;
        auto* factory = new NamedObjectFactory<RegistryObject>();
        factory->creators["node"] = [&](PlayerbotAI*) { return new RegistryObject(alive); };
        factories.Add(factory);
        std::unique_ptr<RegistryObject> first(factories.GetContextObject("node::spell", nullptr));
        std::unique_ptr<RegistryObject> second(factories.GetContextObject("node::spell", nullptr));
        REQUIRE(first);
        REQUIRE(first.get() != second.get());
        REQUIRE(first->getQualifier() == "spell");
        REQUIRE(factories.GetContextObject("missing", nullptr) == nullptr);
    }
    REQUIRE(alive == 0);
}

TEST_CASE("Playerbot qualifier formatting handles more than 255 components", "[playerbot][engine]")
{
    Qualified number(42);
    REQUIRE(number.getQualifier() == "42");
    REQUIRE(Qualified::MultiQualify({"a", "b"}, ",") == "{a,b}");
    REQUIRE(Qualified::MultiQualify({"a", "b"}, ",", "") == "a,b");
    REQUIRE(Qualified::getMultiQualifier("10 20 30", 1) == 20);
    std::vector<std::string> components(300, "x");
    auto const text = Qualified::MultiQualify(components, ",", "");
    REQUIRE(text.size() == 599);
    REQUIRE(text.front() == 'x');
    REQUIRE(text.back() == 'x');
}
