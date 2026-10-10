/* GPL v2 or later. Passive map-owned action-method diagnostics. */
#ifndef PLAYERBOTS_ACTION_TRACE_H
#define PLAYERBOTS_ACTION_TRACE_H
#include "PlayerbotActionHistory.h"
#include <algorithm>
#include <deque>
#include <functional>

struct ActionTraceRecord
{
    uint32 Created = 0;
    std::string Action, Kind, Reason;
    bool ExecuteCalled = false;
    std::optional<bool> ActionReturn, EngineResult;
};

class ActionTraceBuffer
{
public:
    void Enable(bool enabled)
    {
        if (_enabled == enabled) return;
        _enabled = enabled; ++_epoch; _sequence = 0;
        _overwritten = _expired = _suppressed = 0; _records.clear();
        _admissionCount = _executionCount = 0; _windowStart = 0;
    }
    bool Enabled() const { return _enabled; }
    void Record(std::string const& engine, ActionTraceRecord record)
    {
        if (!_enabled) return;
        ++_sequence;
        auto safe = [](std::string const& name) {
            return !name.empty() && name.size() <= 64 && std::all_of(name.begin(), name.end(),
                [](unsigned char c) { return c >= 32 && c < 127; });
        };
        if (!safe(record.Action)) { ++_suppressed; return; }
        Prune(record.Created);
        if (!_records.empty())
        {
            auto& last = _records.back();
            if (last.Event.Sequence + 1 == _sequence && last.Event.Engine == engine && last.Event.Action == record.Action &&
                last.Event.Kind == record.Kind && last.Event.Reason == record.Reason &&
                last.Event.ExecuteCalled == record.ExecuteCalled && last.Event.ActionReturn == record.ActionReturn &&
                last.Event.EngineResult == record.EngineResult && uint32(record.Created - last.Last) <= 250)
            {
                last.Last = record.Created; last.Event.Sequence = _sequence; ++last.Event.Repeats;
                return;
            }
        }
        if (uint32(record.Created - _windowStart) >= 1000)
        { _windowStart = record.Created; _admissionCount = _executionCount = 0; }
        uint32& count = record.Kind == "execution" ? _executionCount : _admissionCount;
        if (count >= 16) { ++_suppressed; return; }
        ++count;
        if (_records.size() == 64) { _records.pop_front(); ++_overwritten; }
        Stored row; row.First = row.Last = record.Created;
        row.Event.Sequence = row.Event.FirstSequence = _sequence; row.Event.Engine = engine;
        row.Event.Action = std::move(record.Action); row.Event.Kind = std::move(record.Kind);
        row.Event.Reason = std::move(record.Reason); row.Event.ExecuteCalled = record.ExecuteCalled;
        row.Event.ActionReturn = record.ActionReturn; row.Event.EngineResult = record.EngineResult;
        _records.push_back(std::move(row));
    }
    PlayerbotActionHistory Read(uint32 now) const
    {
        PlayerbotActionHistory result;
        result.Available = _enabled; result.Epoch = _epoch; result.LastSequence = _sequence;
        result.Overwritten = _overwritten; result.Expired = _expired; result.Suppressed = _suppressed;
        for (auto const& row : _records)
        {
            if (uint32(now - row.First) > 60000) { ++result.Expired; continue; }
            auto event = row.Event;
            event.FirstAgeMs = uint32(now - row.First); event.LastAgeMs = uint32(now - row.Last);
            result.Events.push_back(std::move(event));
        }
        return result;
    }
private:
    struct Stored { uint32 First = 0, Last = 0; PlayerbotActionEvent Event; };
    void Prune(uint32 now)
    {
        while (!_records.empty() && uint32(now - _records.front().First) > 60000)
        { _records.pop_front(); ++_expired; }
    }
    bool _enabled = false;
    uint64 _epoch = 0, _sequence = 0, _overwritten = 0, _expired = 0, _suppressed = 0;
    uint32 _windowStart = 0, _admissionCount = 0, _executionCount = 0;
    std::deque<Stored> _records;
};
#endif
