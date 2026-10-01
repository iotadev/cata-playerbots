/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotAddonRoster.h"
#include <catch2/catch.hpp>

TEST_CASE("MultiBot managed roster preserves donor framing and encoded fields", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(Parse("GET~ALT_ROSTER").Kind == Request::AltRoster);
    REQUIRE(Parse("GET~ALT_ROSTER~other").Kind == Request::Invalid);
    auto empty = FrameManagedRoster({});
    REQUIRE(empty == std::vector<std::string>{ "ALT_ROSTER_BEGIN~0~0", "ALT_ROSTER_END~0~0" });
    auto frames = FrameManagedRoster({ { 5, "A%name", 8, 12, false }, { 6, "Bot", 5, 13, true } });
    REQUIRE(frames.size() == 4);
    REQUIRE(frames.front() == "ALT_ROSTER_BEGIN~2~0");
    REQUIRE(frames[1] == "ALT_ROSTER_ENTRY~5~A%25name~8~12~OFFLINE");
    REQUIRE(frames[2] == "ALT_ROSTER_ENTRY~6~Bot~5~13~ONLINE");
    REQUIRE(frames.back() == "ALT_ROSTER_END~2~0");
}

TEST_CASE("MultiBot managed roster truncates explicitly at donor limits", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    std::vector<ManagedRosterRow> rows;
    for (uint32_t i = 1; i <= 129; ++i)
        rows.push_back({ i, "Bot" + std::to_string(i), 1, 1, false });
    auto frames = FrameManagedRoster(rows);
    REQUIRE(frames.size() == 130);
    REQUIRE(frames.front() == "ALT_ROSTER_BEGIN~128~1");
    REQUIRE(frames.back() == "ALT_ROSTER_END~128~1");
    for (auto const& frame : frames)
        REQUIRE(frame.size() <= MaxMessageBytes);
    REQUIRE(FrameManagedRoster({ { 1, std::string(65, 'x'), 1, 1, false } }).front() == "ALT_ROSTER_BEGIN~0~1");
    REQUIRE(FrameManagedRoster({ { 0, "Bot", 1, 1, false } }).front() == "ALT_ROSTER_BEGIN~0~1");
}

TEST_CASE("MultiBot roster presence follows native loading and teardown", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    ServerOriginPlayerbotLifecycle receipt;
    REQUIRE_FALSE(RosterOnline(nullptr));
    REQUIRE_FALSE(RosterOnline(&receipt));
    receipt.LoginCompleted();
    REQUIRE(RosterOnline(&receipt));
    receipt.StopRequested();
    REQUIRE(RosterOnline(&receipt));
    receipt.SessionClosed(false);
    REQUIRE_FALSE(RosterOnline(&receipt));
}

TEST_CASE("MultiBot roster query limits bound reply amplification", "[playerbots][addon]")
{
    PlayerbotAddonProtocol::RosterQueryGuard guard;
    for (int i = 0; i < 4; ++i)
        REQUIRE(guard.Admit(0));
    REQUIRE_FALSE(guard.Admit(1999));
    REQUIRE(guard.Admit(2000));
}
