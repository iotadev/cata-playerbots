/* Adapted from AcceptQuestAction/QuestAction at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_QUEST_ACCEPT_H
#define PLAYERBOT_QUEST_ACCEPT_H
#include "Define.h"
#include <charconv>
#include <mutex>
#include <optional>
#include <limits>
#include <string_view>
class WorldSession;
namespace PlayerbotQuestAccept
{
enum class Operation : uint32 { Accept, Reward, Inspect, Share, Abandon, AcceptAll,
    InspectCompleted, InspectIncompleted, InspectSummary, RewardAll };
inline bool IsInspection(Operation action)
{
    return action == Operation::Inspect || action == Operation::InspectCompleted ||
        action == Operation::InspectIncompleted || action == Operation::InspectSummary;
}
inline std::optional<Operation> ParseInspection(std::string_view command)
{
    if (command == "quests" || command == "quests all") return Operation::Inspect;
    if (command == "quests completed" || command == "quests co") return Operation::InspectCompleted;
    if (command == "quests incompleted" || command == "quests in") return Operation::InspectIncompleted;
    if (command == "quests summary") return Operation::InspectSummary;
    return {};
}
inline bool ShowQuest(Operation action, bool completed)
{
    return action == Operation::Inspect || (action == Operation::InspectCompleted && completed) ||
        (action == Operation::InspectIncompleted && !completed);
}
inline bool Valid(Operation action, uint32 quest, uint32 item)
{
    if (IsInspection(action) || action == Operation::AcceptAll || action == Operation::RewardAll) return !quest && !item;
    if (action == Operation::Abandon) return quest && !item;
    if (action == Operation::Share) return quest && !item && quest <= uint32(std::numeric_limits<int32>::max());
    return quest && ((action == Operation::Accept && !item) ||
        (action == Operation::Reward && quest <= uint32(std::numeric_limits<int32>::max())));
}
inline bool RecognizesReward(std::string_view command) { return command == "reward" || command.substr(0, 7) == "reward "; }
struct RewardArgs { uint32 Quest = 0, Item = 0; };
inline std::optional<RewardArgs> ParseReward(std::string_view command)
{
    if (command.substr(0, 7) != "reward ") return {};
    auto text = command.substr(7);
    auto space = text.find(' ');
    if (space == std::string_view::npos || !space || space > 10 || text.size() - space - 1 > 10) return {};
    RewardArgs args;
    auto first = std::from_chars(text.data(), text.data() + space, args.Quest);
    auto second = std::from_chars(text.data() + space + 1, text.data() + text.size(), args.Item);
    if (first.ec != std::errc{} || first.ptr != text.data() + space || second.ec != std::errc{} ||
        second.ptr != text.data() + text.size() || !Valid(Operation::Reward, args.Quest, args.Item)) return {};
    return args;
}
inline bool Recognizes(std::string_view command) { return command == "accept" || command.substr(0, 7) == "accept "; }
inline uint32 ParseQuestNumber(std::string_view text)
{
    if (text.empty() || text.size() > 10) return 0;
    uint32 quest = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), quest);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? quest : 0;
}
inline uint32 Parse(std::string_view command) { return command.substr(0, 7) == "accept " ? ParseQuestNumber(command.substr(7)) : 0; }
inline bool RecognizesShare(std::string_view command) { return command == "share" || command.substr(0, 6) == "share "; }
inline uint32 ParseShare(std::string_view command)
{
    uint32 quest = command.substr(0, 6) == "share " ? ParseQuestNumber(command.substr(6)) : 0;
    return Valid(Operation::Share, quest, 0) ? quest : 0;
}
inline bool RecognizesDrop(std::string_view command) { return command == "drop" || command.substr(0, 5) == "drop "; }
inline uint32 ParseDrop(std::string_view command) { return command.substr(0, 5) == "drop " ? ParseQuestNumber(command.substr(5)) : 0; }
struct Request
{
    uint32 Requester = 0, Quest = 0, Map = 0, Instance = 0, Created = 0;
    uint64 Giver = 0, Serial = 0;
    Operation Action = Operation::Accept;
    uint32 Item = 0;
};
inline bool Fresh(Request const& request, uint32 now) { return uint32(now - request.Created) < 5000; }
class Mailbox
{
public:
    bool Post(uint32 requester, uint32 quest, uint64 giver, uint32 map, uint32 instance, uint32 now,
        Operation action = Operation::Accept, uint32 item = 0)
    {
        if (!requester || ((!IsInspection(action) && action != Operation::Share && action != Operation::Abandon) && !giver) || !Valid(action, quest, item)) return false;
        std::lock_guard<std::mutex> guard(_mutex);
        if (_busy) return false;
        _busy = true;
        _pending = Request{requester, quest, map, instance, now, giver, ++_serial, action, item};
        return true;
    }
    std::optional<Request> Take()
    {
        std::lock_guard<std::mutex> guard(_mutex);
        auto request = _pending; _pending.reset(); return request;
    }
    void Finish(uint64 serial)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (serial == _serial) _busy = false;
    }
private:
    std::mutex _mutex;
    bool _busy = false;
    uint64 _serial = 0;
    std::optional<Request> _pending;
};
void Update(WorldSession& session, Mailbox& mailbox);
}
#endif
