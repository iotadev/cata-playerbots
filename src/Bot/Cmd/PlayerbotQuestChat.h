/* Donor quest/item-link command intent; native Cata hyperlink framing.
 * GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_QUEST_CHAT_H
#define PLAYERBOT_QUEST_CHAT_H
#include "../../Ai/Base/PlayerbotQuestAccept.h"
#include "ChatCommandHyperlinks.h"
#include <algorithm>
#include <string>
namespace PlayerbotQuestChat
{
template<bool IsQuest> struct LinkTag
{
    using value_type = uint32;
    static char const* tag() { return IsQuest ? "quest" : "item"; }
    static bool StoreTo(uint32& value, char const* data, size_t size)
    {
        std::string_view fields(data, size);
        value = PlayerbotQuestAccept::ParseQuestNumber(fields.substr(0, fields.find(':')));
        return value != 0; // Item zero is explicit numeric intent, never a fake link.
    }
};
inline bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
inline void Skip(std::string_view& input) { while (!input.empty() && Space(input.front())) input.remove_prefix(1); }
template<bool IsQuest> std::optional<uint32> Operand(std::string_view& input)
{
    Skip(input);
    if (input.empty()) return {};
    if (input.front() != '|')
    {
        size_t end = 0; while (end < input.size() && !Space(input[end])) ++end;
        auto token = input.substr(0, end);
        uint32 value = PlayerbotQuestAccept::ParseQuestNumber(token);
        if (!value && (IsQuest || token != "0")) return {};
        input.remove_prefix(end); return value;
    }
    size_t end = input.find("|h|r");
    if (end == std::string_view::npos || end + 4 < 12) return {};
    size_t consumed = end + 4;
    if (consumed < input.size() && !Space(input[consumed])) return {};
    if (input.substr(0, 2) != "|c") return {};
    for (size_t i = 2; i < 10; ++i)
        if (!((input[i] >= '0' && input[i] <= '9') || (input[i] >= 'a' && input[i] <= 'f') || (input[i] >= 'A' && input[i] <= 'F'))) return {};
    // Native framing gets a complete bounded NUL-terminated operand. We only
    // supply strict ID storage, avoiding legacy truncating numeric conversion.
    std::string link(input.substr(0, consumed));
    Trinity::ChatCommands::Hyperlink<LinkTag<IsQuest>> parser;
    char const* tail = parser.TryConsume(link.c_str());
    if (!tail || *tail) return {};
    uint32 value = *parser;
    input.remove_prefix(consumed); return value;
}
struct Command
{
    PlayerbotQuestAccept::Operation Action = PlayerbotQuestAccept::Operation::Accept;
    uint32 Quest = 0, Item = 0;
};
inline std::optional<Command> Parse(std::string_view text)
{
    if (text.size() > 1024 || text.find('\0') != std::string_view::npos) return {};
    Skip(text);
    size_t end = 0; while (end < text.size() && !Space(text[end])) ++end;
    std::string verb(text.substr(0, end));
    std::transform(verb.begin(), verb.end(), verb.begin(), [](unsigned char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : char(c); });
    Command result;
    using PlayerbotQuestAccept::Operation;
    if (verb == "accept") result.Action = Operation::Accept;
    else if (verb == "reward") result.Action = Operation::Reward;
    else if (verb == "share") result.Action = Operation::Share;
    else if (verb == "drop") result.Action = Operation::Abandon;
    else return {};
    text.remove_prefix(end);
    Skip(text);
    if ((result.Action == Operation::Accept || result.Action == Operation::Reward) && !text.empty() && text.front() == '*')
    {
        text.remove_prefix(1); Skip(text);
        if (!text.empty()) return {};
        result.Action = result.Action == Operation::Accept ? Operation::AcceptAll : Operation::RewardAll; return result;
    }
    auto quest = Operand<true>(text);
    if (!quest) return {};
    result.Quest = *quest;
    if (result.Action == Operation::Reward)
    {
        auto item = Operand<false>(text);
        if (!item) return {};
        result.Item = *item;
    }
    Skip(text);
    if (!text.empty() || !PlayerbotQuestAccept::Valid(result.Action, result.Quest, result.Item)) return {};
    return result;
}
}
#endif
