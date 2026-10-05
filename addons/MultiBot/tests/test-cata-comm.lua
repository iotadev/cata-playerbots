-- Lua 5.1 transport contract test. No WoW client or personal saved variables.
local source = assert(arg[1], "pass the patched Core/MultiBotComm.lua path")
local sent = {}
local party, raid = 0, 0
UnitName = function() return "ExamplePlayer" end
GetTime = function() return 1 end
GetNumPartyMembers = function() return party end
GetNumRaidMembers = function() return raid end
SendAddonMessage = function(prefix, message, channel, target)
  sent[#sent + 1] = {prefix, message, channel, target}
end

local function loadComm(registration)
  MultiBot = {}
  RegisterAddonMessagePrefix = registration
  sent = {}
  assert(loadfile(source))()
  return MultiBot.Comm
end

local comm = loadComm(function(prefix)
  assert(prefix == "MBOT")
  return true
end)
assert(comm.SendHello())
assert(sent[1][1] == "MBOT" and sent[1][2] == "HELLO~1")
assert(sent[1][3] == "WHISPER" and sent[1][4] == "ExamplePlayer")
party = 2
assert(comm.Send("GET", "ALT_ROSTER"))
assert(sent[2][3] == "PARTY" and sent[2][4] == nil)
raid = 10
assert(comm.Send("GET", "ROSTER"))
assert(sent[3][3] == "RAID" and sent[3][4] == nil)
assert(comm.SendPing())
assert(sent[4][2] == "PING~1000")

-- Exercise the actual donor response reader, not a second test parser.
local scheduled = {}
MultiBot.TimerAfter = function(delay, callback)
  scheduled[#scheduled + 1] = {delay, callback}
end
local function receive(message, sender)
  assert(comm.HandleAddonMessage("MBOT", message, "WHISPER", sender or "ExamplePlayer"))
end
receive("ALT_ROSTER_BEGIN~2~0", "OtherPlayer")
assert(MultiBot.bridge.altRosterBatch == nil)
receive("ALT_ROSTER_BEGIN~2~0")
receive("ALT_ROSTER_ENTRY~5~Bot%25name~8~12~OFFLINE")
receive("ALT_ROSTER_ENTRY~6~Priest~5~13~ONLINE")
receive("ALT_ROSTER_END~2~0")
assert(MultiBot.bridge.lastError == nil)
assert(#MultiBot.bridge.altRoster == 2)
assert(MultiBot.bridge.altRoster[1].name == "Bot%name")
assert(MultiBot.bridge.altRoster[2].state == "ONLINE")
receive("ALT_ROSTER_BEGIN~2~0")
receive("ALT_ROSTER_END~1~0")
assert(MultiBot.bridge.lastError == "ALT_ROSTER_END_MISMATCH")
assert(#MultiBot.bridge.altRoster == 2)

-- Capability flag is supplied by this mock only; production still does not
-- advertise it. Timers are captured, not run, to avoid invented world timing.
MultiBot.bridge.botLifecycleCapable = true
local completed
local token = assert(comm.RunBotLifecycle("CONNECT", 5, function(result) completed = result end))
receive("BOT_LIFECYCLE~" .. token .. "~5~Bot%25name~CONNECT~PENDING~STARTED")
assert(completed == nil)
receive("BOT_LIFECYCLE_STATE~" .. token .. "~5~Bot%25name~CONNECTING~PENDING")
assert(completed == nil)
receive("BOT_LIFECYCLE_STATE~" .. token .. "~5~Bot%25name~ONLINE~OK")
assert(completed and completed.status == "OK" and completed.final)
assert(MultiBot.bridge.botLifecycleCommands[token] == nil)
completed = nil
token = assert(comm.RunBotLifecycle("DISCONNECT", 5, function(result) completed = result end))
receive("BOT_LIFECYCLE~" .. token .. "~5~Bot%25name~DISCONNECT~PENDING~STARTED")
receive("BOT_LIFECYCLE_STATE~" .. token .. "~5~Bot%25name~CONNECTING~STOPPING")
assert(completed == nil and MultiBot.bridge.botLifecycleCommands[token] ~= nil)
receive("BOT_LIFECYCLE_STATE~" .. token .. "~5~Bot%25name~OFFLINE~OK")
assert(completed and completed.status == "OK" and completed.final)

-- Drive the actual donor strategy reader and timer. The server's 4s aggregate
-- deadline must leave the addon's 5s pending token alive for a TIMEOUT ACK.
MultiBot.bridge.strategyMutationCapable = true
local strategyResult
local strategyToken = assert(comm.RunStrategyCommand("PARTY", "", "C", "+focus",
  function(result) strategyResult = result end))
local strategyTimer = scheduled[#scheduled]
assert(strategyTimer[1] == 5.0)
assert(sent[#sent][2]:find("RUN~STRATEGY~PARTY~~" .. strategyToken .. "~C~", 1, true))
receive("STRATEGY_ACK~PARTY~~" .. strategyToken .. "~C~2~1~0~TIMEOUT")
assert(strategyResult and strategyResult.reason == "TIMEOUT")
assert(strategyResult.matched == 2 and strategyResult.succeeded == 1 and strategyResult.failed == 0)
assert(MultiBot.bridge.strategyMutationCommands[strategyToken] == nil)
strategyTimer[2]()
assert(strategyResult.reason == "TIMEOUT")

local lateResult, lateCount = nil, 0
strategyToken = assert(comm.RunStrategyCommand("PARTY", "", "C", "+focus",
  function(result) lateResult = result; lateCount = lateCount + 1 end))
strategyTimer = scheduled[#scheduled]
assert(strategyTimer[1] == 5.0)
strategyTimer[2]()
assert(lateResult and lateResult.status == "timeout" and lateCount == 1)
receive("STRATEGY_ACK~PARTY~~" .. strategyToken .. "~C~2~1~0~TIMEOUT")
assert(lateCount == 1 and MultiBot.bridge.lastError == "STRATEGY_ACK_INVALID")

comm = loadComm(function() return false end)
assert(not comm.SendHello() and #sent == 0)
comm = loadComm(nil)
assert(not comm.SendHello() and #sent == 0)
print("Cata comm mock checks passed (transport, roster, lifecycle, strategy ACK/timer)")
