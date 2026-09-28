/*
 * Adapted from AzerothCore mod-playerbots Trigger.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_TRIGGER_H
#define PLAYERBOTS_TRIGGER_H

#include "../Action/Action.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Player;

class Trigger : public PlayerbotAIAware
{
public:
    Trigger(PlayerbotAI* botAI, std::string name = "trigger", int32_t checkInterval = 1);
    virtual ~Trigger() = default;

    virtual Event Check();
    virtual void ExternalEvent([[maybe_unused]] std::string const param,
                               [[maybe_unused]] Player* owner = nullptr) { }
    virtual void ExternalEvent([[maybe_unused]] WorldPacket& packet,
                               [[maybe_unused]] Player* owner = nullptr) { }
    virtual bool IsActive() { return false; }
    virtual bool IsBuffTrigger() { return false; }
    virtual bool IsDebuffTrigger() { return false; }
    virtual std::vector<NextAction> getHandlers() { return {}; }
    void Update() { }
    virtual void Reset() { lastCheckTime = 0; }
    std::string const getName() { return name; }

    // Force-rebuff state is supplied by the future PlayerbotAI runtime instead
    // of being read from a not-yet-ported global context.
    bool needCheck(uint32_t now, bool forceRebuffPending = false, bool inCombat = false);

protected:
    std::string const name;
    uint32_t checkInterval;
    uint32_t lastCheckTime = 0;
};

class TriggerNode
{
public:
    TriggerNode(std::string name, std::vector<NextAction> handlers = {})
        : handlers(std::move(handlers)), name(std::move(name)) { }

    Trigger* getTrigger() { return trigger; }
    void setTrigger(Trigger* value) { trigger = value; }
    std::string const getName() { return name; }
    std::vector<NextAction> getHandlers();
    float getFirstRelevance() { return handlers.empty() ? -1.0f : handlers[0].getRelevance(); }

private:
    Trigger* trigger = nullptr;
    std::vector<NextAction> handlers;
    std::string const name;
};

#endif
