# FiveM Process Analysis — Build b3751

## Process Map

| Process | PID | Description |
|---------|-----|-------------|
| FiveM_b3751_GTAProcess | 10336 | Main game process (target) |
| FiveM | 1296 | Launcher/bootstrapper |
| FiveM_ChromeBrowser | multiple | CEF/NUI renderer |
| FiveM_DumpServer | 1888 | Crash reporter |
| FiveM_ROSLauncher | 13624 | Rockstar Online Services |

**Target**: PID 10336, Working set ~3.27 GB

---

## Loaded Modules (citizen-relevant)

| Module | Base | Size |
|--------|------|------|
| citizen-scripting-core.dll | 0x7FFCBAC10000 | 1128 KB |
| citizen-scripting-lua.dll | 0x7FFCB6F60000 | **8464 KB** |
| citizen-resources-core.dll | 0x7FFCE27E0000 | 756 KB |
| citizen-resources-client.dll | 0x7FFCE29B0000 | 1016 KB |
| citizen-resources-gta.dll | 0x7FFCBA360000 | 316 KB |
| citizen-resources-metadata-lua.dll | 0x7FFCB6370000 | 656 KB |
| rage-scripting-five.dll | 0x7FFCD2EA0000 | 1680 KB |
| scripting-gta.dll | 0x7FFCE2590000 | 280 KB |
| citizen-scripting-mono.dll | 0x7FFCB6DE0000 | 360 KB |

`citizen-scripting-lua.dll` is 8.4 MB because it embeds the full Lua 5.4 runtime.

---

## Native Calling — Current Mode Analysis

The existing `NativeCaller` tries four modes in order:

### Mode 1: Citizen (DEFAULT — PROBLEMATIC)
- Scans `citizen-scripting-core.dll` native registration table
- **Permanently overwrites a native slot** to point to injected shellcode
- Shellcode dispatches a queue to call the real handler + our handler
- **Why it triggers Adhesive**: Adhesive (CFX anti-cheat) scans native table entries in `citizen-scripting-core.dll` for pointer integrity. A modified slot pointing to an unknown RWX page is an immediate flag.

### Mode 2: MainFn (only if ScriptHookV present)
- Registers a persistent script via ScriptHookV's `scriptRegister`
- ScriptHookV is not commonly present on FiveM servers. Usually not available.
- **Adhesive risk**: ScriptHookV itself is detected; if present, we inherit that detection surface.

### Mode 3: APC
- Allocates shellcode, queues `QueueUserAPC` to every thread in FiveM
- Uses `NtAlertThread` to wake alertable threads
- Does **not** modify any native table
- The shellcode itself is in RWX memory but is only active during the call window
- **Lower Adhesive risk** than Citizen mode

### Mode 4: Direct
- Hijacks a script-module thread's `RIP` register
- Thread is suspended, RIP redirected to shellcode, resumed
- Does not modify native table
- **Lowest steady-state footprint** (no permanent hooks)

### Fix: Skip Citizen Mode
The single most impactful change is to **never use Citizen mode**.  
See `NativeSafeInit.hpp` for the patched initialization order.

---

## Resource Start/Stop — Crash Analysis

Current `CallVtableSlot` in `ResourceList.hpp` uses `CreateRemoteThread` to call vtable methods:

```cpp
HANDLE thread = CreateRemoteThread(Mem.ProcHandle, nullptr, 0,
    reinterpret_cast<LPTHREAD_START_ROUTINE>(cave), nullptr, 0, nullptr);
```

### Why it crashes:

1. **Wrong execution context**: FiveM uses a fiber-based scheduler (`CitizenTask`, `CTask`).  
   `CreateRemoteThread` creates a raw OS thread with no FiveM thread-local state — when the vtable's `start()`/`stop()` methods access thread-local storage or FiveM's scheduler, they crash.

2. **vtable slot uncertainty**: Slots 8 and 9 are guesses. If the `Resource` interface has changed in build b3751, these slots are wrong.

3. **No synchronization with the resource manager's lock**: The resource manager holds an internal `std::mutex` during ticks. Calling vtable methods from a foreign thread races with the manager's tick and causes UB.

4. **Stack size**: `CreateRemoteThread` with `0` stack size uses the default (1 MB), but the shellcode sets up only a 0x20-byte shadow space which is fine for the vtable call itself. However, if the vtable method calls into Lua or any FiveM subsystem that expects a larger stack, it will overflow.

### Fix: APC-based vtable invocation via existing NativeCaller APC infrastructure
See `ResourceManagerV2.hpp`.

---

## Lua Executor Architecture

FiveM uses **Lua 5.4** embedded inside `citizen-scripting-lua.dll`.

### Key types to find:

```
lua_State {
  +0x00  GCObject*   next        ; heap chain
  +0x08  lu_byte     tt          ; == 8 (LUA_TTHREAD) 
  +0x09  lu_byte     marked
  +0x0A  lu_byte     status      ; 0 = LUA_OK
  +0x0B  lu_byte     allowhook
  +0x0C  unsigned short nci      ; call depth
  +0x10  StkId       top         ; pointer into stack
  +0x18  global_State* l_G       ; global state (pointer back)
  +0x20  CallInfo*   ci          ; current call info
  ...
}

global_State {
  +0x00  lua_Alloc   frealloc
  +0x08  void*       ud
  +0x10  l_mem       totalbytes
  ...
  +0x68  lua_State*  mainthread  ; -> back to primary state
}
```

### Finding lua_State* candidates in memory:
1. Enumerate all readable committed pages in the FiveM process
2. At each 8-byte-aligned address, check if `[addr+0x08]` == 8 (LUA_TTHREAD) and `[addr+0x18]` is a valid pointer to a `global_State` whose `mainthread` is also valid
3. Validate `[addr+0x0A]` (status) is <= 3

### Executing Lua code:
Once we have a valid `lua_State* L`:
1. Find `luaL_loadbufferx` or `luaL_dostring` inside `citizen-scripting-lua.dll` (exported or pattern scan)
2. Inject shellcode via APC or Direct mode that:
   - Pushes our Lua source code (as a remote string) into the process
   - Calls `luaL_loadbuffer(L, code, len, "=(kdex)")`
   - Calls `lua_pcall(L, 0, LUA_MULTRET, 0)`
   - Reads back the stack top for results

See `LuaExecutor.hpp`.

---

## Adhesive Threat Surface Summary

| Vector | Adhesive detects | Mitigation |
|--------|-----------------|-----------|
| Native table hook (Citizen mode) | Modified slot pointer → foreign RWX page | Skip Citizen mode entirely |
| RWX executable pages | Scanned for known shellcode signatures | Use RW+RX split; mark as image-like where possible |
| CreateRemoteThread | WinAPI hook on thread creation | Prefer QueueUserAPC (less monitored) |
| Suspended thread + RIP modification (Direct mode) | Thread context changes detected by kernel callbacks | Use APC mode as primary |
| Pattern scan bypass | Scanning for GetModuleHandle / VirtualAlloc patterns | xorstr + lazy importer already applied |

---

## Build b3751 Specific Notes

- Native handler RVAs are pre-computed in `B3751HandlerRvas.hpp` — use those first before scanning
- `CitizenNativeCore.hpp` known table offsets: `{0x10BD58, 0x10BCE8, ...}` — these are relative to `citizen-scripting-core.dll` base
- Resource state offset `0x118` confirmed for build b3751 from existing code
- Resource path at `0xF0` confirmed
