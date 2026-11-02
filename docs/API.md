# Public API reference (v0.1.0)

## Build and source compatibility

C++: include `rootcastle/npc.hpp`, link target `rootcastle::npc`; API requires C++20. C: include `rootcastle/npc_c.h` and link the same library. There are no required runtime dependencies beyond the target platform's C/C++ runtime. Stable source compatibility is intended within 0.1.x; only the C ABI has an explicit `rc_abi_version()` contract.

## Ownership and timing

One thread owns a `World` instance. The host may collect observations on any thread but must copy/synchronize them before calling public methods on the world. No method calls an engine API except the optional `TaskBridge::flush()` calling the injected `TaskSink` and registered hook callbacks. All decisions and output vectors are valid until subsequent world mutation; never hold `Decision*` across removal or concurrent access. Call `tick(delta_seconds)` using a positive finite interval `(0, 1]` seconds, typically a fixed `1/60`.

## World methods

| C++ | Description | Result / lifetime |
|---|---|---|
| `add(Config, Pose)` | Insert actor with initial position and normalized facing. Actor ID must be nonzero. | `Ok`, `InvalidArgument`, `AlreadyExists`, `CapacityExceeded` |
| `remove(id)` | Erase actor, state, memory, patrol. | `Ok`, `NotFound` |
| `update_pose(id,Pose)` | Set position, facing and health. | `Ok`, `InvalidArgument`, `NotFound` |
| `submit(id,Stimulus)` | Queue one event for actor. If inbox full, oldest event is dropped; increments counter. | `Ok`, `InvalidArgument`, `NotFound` |
| `broadcast(event,radius)` | Queue for actors in radial range. No spatial index: linear in actors. | `Ok`, `InvalidArgument` |
| `set_relation(from,to,Relation)` | Set directed faction relation; Neutral removes custom entry. | `Ok`, `InvalidArgument`, `CapacityExceeded` |
| `observe(observer,target,los,event_id)` | Classify visible actor by faction and submit sighting; `los=false` is a no-op. | `Ok`, `InvalidArgument`, `NotFound` |
| `set_patrol(id,vector<Vec3>)` | Set/clear patrol path, max 128 points. | `Ok`, `InvalidArgument`, `NotFound`, `CapacityExceeded` |
| `arrived(id)` | Advance patrol, clear wander goal, consume investigation event. | `Ok`, `NotFound` |
| `tick(dt)` | Process memory TTL, events, decisions and collect changed decisions. | `Ok`, `InvalidArgument` |
| `current(id)` | Read current `Decision`, or null for unknown actor. | Borrowed pointer |
| `changes()` | Decisions changed during most recent tick. | Borrowed vector until next tick |
| `metrics()` | Read lifetime counters. | Borrowed struct |
| `reset_metrics()` | Clear counters without resetting simulation state. | void |

### Input contracts

- `Config.id > 0`, `Config.seed > 0`, all personality scalars in `[0,1]`, `sight_range/hearing_range` in `[0,10000]`, FOV in `(0,360]`, TTL `[0.1,3600]`, decision interval `[0.001,10]`, switch margin `[0,1]`, wander radius `[0,10000]`, flee distance `(0,10000]`, and queue/memory capacities within supported bounds.
- `Pose.position/forward` must contain finite floats. Health must be in `[0,1]`. Zero forward vector defaults to +X.
- `Stimulus` event kind must be a defined enumerator; position finite and strength in `[0,1]`. Event ID zero is allowed (source/type dedup still works for a nonzero source). LOS is **not** calculated internally: host must only submit visible-sight events when a raycast or equivalent has validated them. Damage and `ClearThreat` are trusted direct events (not subject to distance filters).
- `observe` requires two distinct existing actor IDs; target must be alive and host LOS true. When unspecified, same nonzero faction is Friendly and different factions are Neutral. Faction mappings are directed; define reverse relationships if necessary.
- `Submit` accepts an event before filtering: `filtered` increases during a subsequent tick. Broadcast radius and agent sight/hearing range are independent. Event queues drop oldest first (including damage/alarm); a host requiring guaranteed delivery must implement higher-priority staging.

### Decision semantics

`Decision.actor` is the agent, `intent` is the chosen action, `reason` explains selection, `target` identifies an entity or zero, `destination` is a proposed point for the engine to validate, and `sequence` increases only when chosen intent/target/reason/destination changes. `changes()` contains only changed decisions. A newly added actor has an initial Idle decision with sequence 0 until `tick` chooses a non-idle action. Agent order is ascending numeric ID; tie-breaking uses deterministic candidate order.

### Personality and decision arbitration

Personality parameters `courage`, `aggression`, `curiosity`, `empathy`, `discipline` are `0..1`. Higher courage and aggression favour engagement; higher curiosity favours investigation; empathy and discipline favour responding to help. Lower health increases flight preference and reduces engagement preference. Stimuli decay linearly by configured memory TTL. The highest utility action wins, with a configurable switching margin that can retain an existing task when competing actions are close. These are adjustable game-design heuristics, not a model of human psychology.

## Hook API

Include `rootcastle/task_hooks.hpp`. A `HookPipeline` stores up to 128 registered callbacks in insertion order.

```cpp
using TaskHook = HookVerdict (*)(TaskCommand&, void*) noexcept;
std::uint64_t HookPipeline::add(TaskKind, TaskHook, void* user_data=nullptr);
bool HookPipeline::remove(std::uint64_t hook_token);
HookVerdict HookPipeline::apply(TaskCommand&) const;
```

`TaskKind::Any` matches all commands. A callback may change kind/target/destination/utility or return `Cancel`; it must not change actor/sequence, call `add/remove` while dispatching or throw. Invalid hook output is discarded and counted. A `TaskBridge` transforms changed decisions into `TaskCommand` values, validates post-hook fields and sends them through a user-provided `TaskSink`. A sink returns **true only after accepting** a command. A failed command is retained until retry/supersession. The bridge must be called on the owner thread after each tick and must outlive its references. Hook registration and adapter invocation are not binary executable hooks.

## C ABI

Use `rc_world_create(&handle)` / `rc_world_destroy(handle)`. Handles are owned by callers. `rc_default_agent_config(id)` and `rc_default_pose()` supply valid defaults. All status-returning functions use `rc_result` (`RC_OK`..`RC_OUT_OF_MEMORY`). `rc_abi_version()` returns `0x0100` for 1.0. `rc_world_tick(handle, dt)` updates state; `rc_world_change_count(handle)` gives the number of decisions in the most recent tick; `rc_world_get_change(handle,index,&out)` copies one decision. Obtain active actor decision with `rc_world_get_decision`. For diagnostics use `rc_world_get_metrics`. For named C methods and struct field layouts, the authoritative contract is [`include/rootcastle/npc_c.h`](../include/rootcastle/npc_c.h).

### C FFI safety

Never dereference or reuse a destroyed handle. All output pointer parameters must point to valid writable memory and all arrays must be of the declared size. A pointer to an invalid memory region cannot be recognized by a C ABI. Return values must be checked; a failed call should not be interpreted as a valid decision. Do not unload a shared native library while handles exist.
