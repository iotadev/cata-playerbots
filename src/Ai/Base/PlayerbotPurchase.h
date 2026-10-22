/* BuyAction donor/Cata adaptation. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_PURCHASE_H
#define PLAYERBOT_PURCHASE_H
#include "PlayerbotNpcRequest.h"
#include <charconv>
#include <string_view>
#include <array>
class WorldSession;
namespace PlayerbotPurchase
{
using PlayerbotNpcRequest::Mailbox;
constexpr uint32 MaxVendorRows = 128;
constexpr uint32 MaxPurchasesPerType = 10; // Donor consumable iteration cap.
inline bool VendorCommand(std::string_view command) { return command == "buy vendor"; }
enum class Category : uint32 { Food, Drink, HealingPotion, ManaPotion, Bandage, Unsupported };
constexpr uint32 Categories = uint32(Category::Unsupported);
struct Offer { uint32 Item = 0, Slot = 0, Level = 0; Category Kind = Category::Food; };
using Choices = std::array<std::optional<Offer>, Categories>;
inline void Consider(std::array<std::optional<Offer>, Categories>& choices, Offer offer)
{
    if (!offer.Item || offer.Slot >= MaxVendorRows || uint32(offer.Kind) >= Categories) return;
    auto& current = choices[uint32(offer.Kind)];
    if (!current || offer.Level > current->Level ||
        (offer.Level == current->Level && (offer.Item < current->Item || (offer.Item == current->Item && offer.Slot < current->Slot))))
        current = offer;
}
inline bool Recognizes(std::string_view command) { return command == "buy" || command.substr(0, 4) == "buy "; }
inline uint32 Parse(std::string_view command)
{
    if (command.substr(0, 4) != "buy ") return 0;
    auto operand = command.substr(4);
    if (operand.empty() || operand.size() > 10) return 0;
    uint32 item = 0;
    auto result = std::from_chars(operand.data(), operand.data() + operand.size(), item);
    return result.ec == std::errc{} && result.ptr == operand.data() + operand.size() ? item : 0;
}
// Cata count is individual items, so one bundle is the native BuyCount.
inline bool BundleFits(uint32 bundle, uint32 stack, uint32 carried)
{
    return bundle && stack && bundle <= stack && uint64(carried) + bundle <= uint64(stack) * 2;
}
void Update(WorldSession& session, Mailbox& mailbox);
}
#endif
