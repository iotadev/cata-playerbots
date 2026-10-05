/* Ground-only adaptation of donor CheckMountStateAction at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_MOUNT_H
#define PLAYERBOT_MOUNT_H
#include <cstdint>
class PlayerbotAI;
namespace PlayerbotMount
{
struct State
{
    uint32_t Spell = 0, Started = 0, LastAttempt = 0;
    uint64_t Owner = 0;
    bool Attempted = false;
    bool CanAttempt(uint32_t now) const { return !Attempted || uint32_t(now - LastAttempt) >= 5000; }
    void Attempt(uint32_t now) { Attempted = true; LastAttempt = now; }
    void Forget() { Spell = 0; Owner = 0; Started = 0; }
};
inline bool ShouldMount(bool enabled, bool following, bool ownerGroundMounted, bool botMounted, bool busy, bool nativeAllowed)
{ return enabled && following && ownerGroundMounted && !botMounted && !busy && nativeAllowed; }
inline bool ShouldRelease(bool owned, bool enabled, bool following, bool sameOwner, bool ownerGroundMounted, bool busy)
{ return owned && (!enabled || !following || !sameOwner || !ownerGroundMounted || busy); }
inline bool GroundCapability(bool mounted, bool ground, bool flight, bool underwater)
{ return mounted && ground && !flight && !underwater; }
struct Candidate { uint32_t Base, Cast; int32_t Speed; };
inline bool Better(Candidate const& a, Candidate const& b)
{ return a.Speed != b.Speed ? a.Speed > b.Speed : a.Base < b.Base; }
struct Result { bool BlockFollow = false; bool ResetFollow = false; };
// Map-thread only. Does not grant mounts/riding, replace flight flags, teleport,
// or own a second movement controller. Only its own spell/aura is canceled.
Result Update(PlayerbotAI& ai, State& state, bool following, bool command, uint32_t now);
}
#endif
