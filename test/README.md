# test/ — FiveM b3751 Patches & Extensions

## Files

| File | Purpose |
|------|---------|
| `ANALYSIS.md` | Full process analysis: modules, native modes, resource crash root cause, Lua state layout |
| `NativeSafeInit.hpp` | Patch: skip Citizen mode → no native table hook → no Adhesive trigger |
| `ResourceManagerV2.hpp` | Fixed resource start/stop using QueueUserAPC instead of CreateRemoteThread |
| `LuaExecutor.hpp` | Arbitrary Lua 5.4 execution via process memory scan + APC shellcode |

---

## 1. Native Caller — Adhesive Bypass

**Problem**: `NativeCaller::Initialize()` tries Citizen mode first, which permanently  
overwrites a native table slot with a pointer to our RWX shellcode. Adhesive scans  
that table and flags the modified pointer.

**Fix** — in `kdex/game/Core/SDK/Natives/NativeCaller.cpp`, replace the body of  
`CNativeCaller::Initialize()`:

```cpp
void CNativeCaller::Initialize() {
    if (m_ready) return;
    if (!Mem.ProcId || !Mem.ProcHandle) return;
    if (!m_build) m_build = DetectBuild();

    RefreshModRanges();
    ScanCitizenTable();  // still needed for handler lookups in APC/Direct modes

    // Skip TryCitizenMode() entirely — it hooks the native table
    if (TryMainFnMode())  { m_ready = true; return; }
    if (TryApcMode())     { m_ready = true; return; }
    if (TryDirectMode())  { m_ready = true; return; }
    // Only fall back to Citizen as absolute last resort:
    if (TryCitizenMode()) { m_ready = true; return; }
}
```

On a normal FiveM server (no ScriptHookV), this will use **APC mode** — which  
queues APCs to existing alertable threads without touching any native table.

---

## 2. Resource Start/Stop — Crash Fix

**Problem**: `CallVtableSlot()` uses `CreateRemoteThread` to call vtable methods.  
FiveM resource vtable methods access thread-local state (fiber scheduler, Lua runtime)  
that a raw OS thread does not have → crash.

**Fix** — replace `cResourceList::CallVtableSlot` with `ResourceV2::g_ResourceV2`:

```cpp
// In your exploit thread or wherever you call resource start/stop:
#include "../../test/ResourceManagerV2.hpp"

// Stop a resource:
ResourceV2::g_ResourceV2.stop(resource_ptr);

// Start a resource:
ResourceV2::g_ResourceV2.start(resource_ptr);

// Revive (force-stop then start):
ResourceV2::g_ResourceV2.revive(resource_ptr);
```

The new implementation queues an APC shellcode to FiveM's existing scripting threads  
so the vtable call executes in the correct thread context.

**Verifying vtable slots** — call once with logging enabled:
```cpp
ResourceV2::g_ResourceV2.LogVtable(resource_ptr, resource_name);
// Check DebugLog output; start should be slot 8, stop slot 9
// If they differ, change the slot constants in stop()/start()
```

---

## 3. Lua Executor

**Usage**:
```cpp
#include "../../test/LuaExecutor.hpp"

// Simple execution
bool ok = LuaExec::ExecLua("print('hello from kdex')");

// Or use the full executor for status codes:
auto result = LuaExec::g_LuaExecutor.Execute(R"lua(
    local ped = PlayerPedId()
    SetEntityHealth(ped, 200)
    TriggerEvent("chatMessage", "", {255,0,0}, "injected by kdex")
)lua");

if (!result.ok)        -- timeout
if (result.loadStatus) -- Lua syntax error
if (result.pcallStatus) -- Lua runtime error
```

**How it works**:
1. Scans `citizen-scripting-lua.dll` export table for `luaL_loadbuffer` + `lua_pcall`
2. Scans process heap for valid `lua_State*` (Lua 5.4 thread header check + global_State cross-validation)
3. Writes Lua source + a small APC shellcode into the target process
4. Queues the APC to FiveM's scripting threads
5. Waits for the `done` flag, reads back status codes

**Notes**:
- First call takes longer (state scan over ~3 GB heap). Result is cached.
- If the cached state becomes invalid (resource restart), call `g_LuaExecutor.Invalidate()`.
- `lua_pcall` with `msgh=0` means errors are stored as a string on the Lua stack.  
  If you want the error message, you need to read the stack top via additional shellcode.

---

## Integration Checklist

- [ ] Patch `Initialize()` in `NativeCaller.cpp` (skip Citizen mode)
- [ ] Wire `ResourceV2::g_ResourceV2` into the resource list UI (replace `g_ResourceList.start/stop`)
- [ ] Wire `LuaExec::ExecLua` to the lua_field execute button ("K" button in `lua.cpp`)
- [ ] Add `g_LuaExecutor.Init()` call to `Core.hpp` startup sequence
- [ ] Add `g_ResourceV2.Init()` call to `Core.hpp` startup sequence
- [ ] Add `g_ResourceV2.Cleanup()` to shutdown path (alongside NativeCaller::Shutdown)
