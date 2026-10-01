/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "NativeCharacterCreationReceipt.h"
#include <catch2/catch.hpp>

TEST_CASE("Native character creation distinguishes pending rejection and committed identity", "[playerbots][factory]")
{
    using State = NativeCharacterCreationReceipt::State;
    NativeCharacterCreationReceipt success;
    REQUIRE(success.GetState() == State::Pending);
    REQUIRE_FALSE(success.Complete(1, true));
    REQUIRE(success.GetState() == State::Pending);
    REQUIRE(success.Complete(1, true, 42));
    REQUIRE(success.GetState() == State::Succeeded);
    REQUIRE(success.GetCharacterGuidLow() == 42);
    REQUIRE(success.GetNativeResult() == 1);
    REQUIRE_FALSE(success.Complete(2, false));
    success.Abandon();
    REQUIRE(success.GetState() == State::Succeeded);

    NativeCharacterCreationReceipt failure;
    REQUIRE(failure.Complete(17, false, 42));
    REQUIRE(failure.GetState() == State::Rejected);
    REQUIRE(failure.GetNativeResult() == 17);
    REQUIRE(failure.GetCharacterGuidLow() == 0);
}

TEST_CASE("Abandoned character creation does not certify rollback or later completion", "[playerbots][factory]")
{
    NativeCharacterCreationReceipt receipt;
    receipt.Abandon();
    REQUIRE(receipt.GetState() == NativeCharacterCreationReceipt::State::Abandoned);
    REQUIRE(receipt.GetCharacterGuidLow() == 0);
    REQUIRE_FALSE(receipt.Complete(1, true, 42));
}

TEST_CASE("Provisioning readiness waits for confirmed realm accounting", "[playerbots][factory]")
{
    NativeCharacterCreationReceipt receipt(true);
    REQUIRE_FALSE(receipt.AccountingCompleted(true));
    REQUIRE(receipt.Complete(1, true, 42));
    REQUIRE(receipt.GetState() == NativeCharacterCreationReceipt::State::Reconciling);
    REQUIRE(receipt.GetCharacterGuidLow() == 0);
    REQUIRE(receipt.GetCommittedCharacterGuidLow() == 42);
    REQUIRE_FALSE(receipt.Complete(2, false));
    REQUIRE(receipt.AccountingCompleted(true));
    REQUIRE(receipt.GetState() == NativeCharacterCreationReceipt::State::Succeeded);
    REQUIRE(receipt.GetCharacterGuidLow() == 42);
    REQUIRE_FALSE(receipt.AccountingCompleted(false));
}

TEST_CASE("Provisioning accounting failure retains committed recovery evidence", "[playerbots][factory]")
{
    NativeCharacterCreationReceipt receipt(true);
    REQUIRE(receipt.Complete(1, true, 42));
    REQUIRE(receipt.AccountingCompleted(false));
    REQUIRE(receipt.GetState() == NativeCharacterCreationReceipt::State::AccountingFailed);
    REQUIRE(receipt.GetCharacterGuidLow() == 0);
    REQUIRE(receipt.GetCommittedCharacterGuidLow() == 42);
    receipt.Abandon();
    REQUIRE(receipt.GetState() == NativeCharacterCreationReceipt::State::AccountingFailed);
}

TEST_CASE("Provisioning rejection and abandonment remain honest while accounting is pending", "[playerbots][factory]")
{
    NativeCharacterCreationReceipt rejected(true);
    REQUIRE(rejected.Complete(17, false));
    REQUIRE(rejected.GetState() == NativeCharacterCreationReceipt::State::Reconciling);
    REQUIRE(rejected.AccountingCompleted(true));
    REQUIRE(rejected.GetState() == NativeCharacterCreationReceipt::State::Rejected);
    REQUIRE(rejected.GetNativeResult() == 17);

    NativeCharacterCreationReceipt abandoned(true);
    REQUIRE(abandoned.Complete(1, true, 42));
    abandoned.Abandon();
    REQUIRE(abandoned.GetState() == NativeCharacterCreationReceipt::State::Abandoned);
    REQUIRE(abandoned.GetCharacterGuidLow() == 0);
    REQUIRE(abandoned.GetCommittedCharacterGuidLow() == 42);
    REQUIRE_FALSE(abandoned.AccountingCompleted(true));
}
