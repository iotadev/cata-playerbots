/*
 * Adapted from AzerothCore mod-playerbots Value.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_VALUE_H
#define PLAYERBOTS_VALUE_H

#include "../PlayerbotAIAware.h"
#include "Timer.h"
#include <cstdint>
#include <string>
#include <utility>

class UntypedValue : public PlayerbotAIAware
{
public:
    UntypedValue(PlayerbotAI* botAI, std::string name) : PlayerbotAIAware(botAI), name(std::move(name)) { }
    virtual ~UntypedValue() = default;
    virtual void Update() { }
    virtual void Reset() { }
    virtual std::string const Format() { return "?"; }
    virtual std::string const Save() { return "?"; }
    virtual bool Load([[maybe_unused]] std::string const value) { return false; }
    std::string const getName() { return name; }

protected:
    std::string const name;
};

template <class T>
class Value
{
public:
    virtual ~Value() = default;
    virtual T Get() = 0;
    virtual T LazyGet() = 0;
    virtual T& RefGet() = 0;
    virtual void Reset() { }
    virtual void Set(T value) = 0;
    operator T() { return Get(); }
};

template <class T>
class CalculatedValue : public UntypedValue, public Value<T>
{
public:
    CalculatedValue(PlayerbotAI* botAI, std::string name = "value", uint32_t interval = 1)
        : UntypedValue(botAI, std::move(name)),
          checkInterval(interval == 1 ? 1 : (interval < 100 ? interval * 1000 : interval)) { }

    T Get() override
    {
        Refresh();
        return value;
    }
    T LazyGet() override
    {
        if (!hasCached)
            return Get();
        return value;
    }
    T& RefGet() override
    {
        Refresh();
        return value;
    }
    void Set(T updated) override { value = updated; }
    void Reset() override { lastCheckTime = 0; hasCached = false; }

protected:
    virtual T Calculate() = 0;

    void Refresh()
    {
        if (checkInterval < 2)
        {
            value = Calculate();
            hasCached = true;
            return;
        }

        uint32_t now = getMSTime();
        if (!hasCached || now - lastCheckTime >= checkInterval)
        {
            lastCheckTime = now;
            value = Calculate();
            hasCached = true;
        }
    }

    uint32_t checkInterval;
    uint32_t lastCheckTime = 0;
    bool hasCached = false;
    T value{};
};

template <class T>
class SingleCalculatedValue : public CalculatedValue<T>
{
public:
    SingleCalculatedValue(PlayerbotAI* botAI, std::string name = "value")
        : CalculatedValue<T>(botAI, std::move(name)) { }

    T Get() override
    {
        if (!calculated)
        {
            this->value = this->Calculate();
            calculated = true;
        }
        return this->value;
    }
    T LazyGet() override { return Get(); }
    T& RefGet() override
    {
        Get();
        return this->value;
    }
    void Reset() override
    {
        CalculatedValue<T>::Reset();
        calculated = false;
    }

private:
    bool calculated = false;
};

template <class T>
class ManualSetValue : public UntypedValue, public Value<T>
{
public:
    ManualSetValue(PlayerbotAI* botAI, T defaultValue, std::string name = "value")
        : UntypedValue(botAI, std::move(name)), value(defaultValue), defaultValue(defaultValue) { }

    T Get() override { return value; }
    T LazyGet() override { return value; }
    T& RefGet() override { return value; }
    void Set(T updated) override { value = updated; }
    void Reset() override { value = defaultValue; }

protected:
    T value;
    T defaultValue;
};

#endif
