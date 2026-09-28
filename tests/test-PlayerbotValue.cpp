/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Value/Value.h"
#include <catch2/catch.hpp>

namespace
{
class CountValue final : public CalculatedValue<int>
{
public:
    explicit CountValue(uint32_t interval) : CalculatedValue<int>(nullptr, "count", interval) { }
    int calculations = 0;
protected:
    int Calculate() override { return ++calculations; }
};

class SingleCountValue final : public SingleCalculatedValue<int>
{
public:
    SingleCountValue() : SingleCalculatedValue<int>(nullptr, "single") { }
    int calculations = 0;
protected:
    int Calculate() override { return ++calculations; }
};
}

TEST_CASE("Playerbot calculated values cache, lazily read and reset", "[playerbot][engine][value]")
{
    CountValue cached(10000);
    REQUIRE(cached.LazyGet() == 1);
    REQUIRE(cached.Get() == 1);
    REQUIRE(cached.calculations == 1);
    REQUIRE(cached.RefGet() == 1);
    cached.Reset();
    REQUIRE(cached.Get() == 2);

    CountValue eager(1);
    REQUIRE(eager.Get() == 1);
    REQUIRE(eager.Get() == 2);
}

TEST_CASE("Playerbot single calculated values calculate once until reset", "[playerbot][engine][value]")
{
    SingleCountValue value;
    REQUIRE(value.LazyGet() == 1);
    REQUIRE(value.Get() == 1);
    REQUIRE(value.RefGet() == 1);
    REQUIRE(value.calculations == 1);
    value.Reset();
    REQUIRE(value.Get() == 2);
}

TEST_CASE("Playerbot manual values retain and restore their default", "[playerbot][engine][value]")
{
    ManualSetValue<int> value(nullptr, 7, "manual");
    REQUIRE(value.getName() == "manual");
    REQUIRE(value.Get() == 7);
    value.Set(12);
    REQUIRE(value.LazyGet() == 12);
    value.RefGet() = 14;
    REQUIRE(value.Get() == 14);
    value.Reset();
    REQUIRE(value.Get() == 7);
}
