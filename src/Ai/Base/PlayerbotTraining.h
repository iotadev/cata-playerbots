/* TrainerAction donor intent / native Cata adaptation. GPL v2 or later. */
#ifndef PLAYERBOT_TRAINING_H
#define PLAYERBOT_TRAINING_H
#include "PlayerbotNpcRequest.h"
#include <charconv>
#include <limits>
#include <string_view>
class WorldSession;
namespace PlayerbotTraining
{
using PlayerbotNpcRequest::Mailbox;
constexpr uint32 Inspect = 0;
constexpr uint32 LearnAll = std::numeric_limits<uint32>::max();
constexpr uint32 MaxOffers = 128, MaxListed = 12, MaxPurchases = 16;
inline bool Recognizes(std::string_view command)
{
    return command == "train" || command.substr(0, 6) == "train " ||
        command == "trainer" || command.substr(0, 8) == "trainer ";
}
inline uint32 Parse(std::string_view command)
{
    if (command.substr(0, 6) != "train ") return 0;
    auto operand = command.substr(6);
    if (operand.empty() || operand.size() > 10) return 0;
    uint32 spell = 0;
    auto result = std::from_chars(operand.data(), operand.data() + operand.size(), spell);
    return result.ec == std::errc{} && result.ptr == operand.data() + operand.size() &&
        spell <= uint32(std::numeric_limits<int32>::max()) ? spell : 0;
}
// Reserved queue operands never become native spell IDs. Invalid syntax is not inspection.
inline std::optional<uint32> ParseRequest(std::string_view command)
{
    if (command == "trainer" || command == "train ?") return Inspect;
    if (command == "trainer learn" || command == "train all") return LearnAll;
    uint32 spell = Parse(command);
    return spell ? std::optional<uint32>(spell) : std::nullopt;
}
void Update(WorldSession& session, Mailbox& mailbox);
}
#endif
