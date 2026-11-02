# Engineering architecture and limits

## Layers

```text
Engine adapter (external, one thread owner)
  | Perception events / accurate transforms / entity lifecycle
  v
World::submit / observe / set_relation / update_pose
  | bounded inboxes
  v
perceives -> remember -> evaluate
  | bounded memory (TTL, salience), utility scores, hysteresis
  v
Decision changes (intent, reason, target, destination, sequence)
  v
TaskBridge + HookPipeline (optional)
  | validate -> host TaskSink -> host physics/navmesh/animation/combat
  v
Engine execution and arrival callbacks
```

Core includes no reference to a renderer, operating system, physics engine, network or platform loader. `src/npc.cpp` is pure simulation logic and state; `src/task_hooks.cpp` is an optional command adapter; `src/npc_c.cpp` is a versioned FFI boundary. This separation makes the simulation portable without binary patching or engine ABI dependencies.

## Behaviour scoring

An actor generates routine Idle, Patrol or Wander candidate(s) and one or two candidates for each remembered event. Hostile sightings, damage and alarms add Flee and Engage candidates (if enabled). HelpRequest adds Assist; Sound and NeutralSight add Investigate. FriendlySight is remembered but does not trigger investigation. Candidate scores depend on personality, health and decaying event salience, clamped to `[0,1]`. Highest score wins; ties maintain input order. `switch_margin` preserves the current intent/target when within the margin of a newly preferred candidate. The system prioritizes predictable behaviour over claims of emergent general intelligence.

### State lifecycle

- Add: records a stable home location and seed; no background work.
- Submit: queue event; when full, oldest event is discarded. Metrics expose overload.
- Tick: decay old memories, filter new events, refresh/evict memory, select actions if due or when new input/replan occurs, publish changed decisions.
- Arrival: patrol advances to next waypoint; wander generates a new destination; investigate/assist removes relevant memory.
- Remove: drop state and prevent continued lookup. Engine owns actual despawn.

## Determinism and replay

`std::map` iteration orders agents and directed faction relations; xorshift32 with explicit seed provides reproducible wander goals. All time derives from caller-supplied `delta_seconds`; the runtime does not read wall time. The host must replay actor addition order, pose updates, arrivals, events and `tick` calls in the same order. Floating-point results across different CPU architectures/compiler modes are not guaranteed bit-for-bit identical. No global RNG, process-wide mutable actor registry or background task exists. For production networking, replicate authoritative decisions rather than assume cross-architecture lockstep.

## Resource budgets and performance

| Budget | Maximum / policy |
|---|---|
| Agents | 100,000 per World hard cap; actual performance much lower depending on hardware |
| Inbox | 1-256 events per actor; drop oldest on overflow |
| Memories | 1-64 remembered events per actor; FIFO eviction |
| Patrol route | 128 points per actor |
| Registered hooks | 128 per pipeline; registration order |
| Faction dispositions | 4,096 directed non-neutral relationships |
| Tick delta | `(0, 1]` seconds; larger pause must be split or handled by game logic |
| Complexity | Per tick O(agents + events + memories); `broadcast` O(agents); no built-in spatial index |

Heap allocations occur when adding actors, submitting events, remembering stimuli, setting patrols and collecting changes. This is not a real-time no-allocation hard-RT component. Expose tick time in your host telemetry and keep expensive physics/LOS outside the runtime.

## Threat model

**Assets:** playable state integrity, frame-time budget, target ownership and safety of calling native code.

**Trust boundary:** external game scripts, mod input, replicated events and plugin hooks are *not* inherently trusted. The runtime validates scalar value ranges, types, capacities and actor identity but cannot validate hostile intent or physics line of sight by itself.

**Failure cases and controls:**

- Spoofed hostile events can manipulate decisions: host authenticates source and target, checks LOS for sight and emits direct damage only on verified hit.
- Event floods cause memory/time pressure: queues and memory are bounded with drop counters. Host rate-limits producers and checks `metrics().dropped`.
- Non-finite/invalid coordinates can poison navigation: library rejects non-finite external input and validates post-hook commands; the host must clamp world bounds and navmesh destinations.
- Untrusted native hooks can crash the process: only compiled trusted code can be registered; do not load arbitrary plugin binaries. Hooks cannot be used as an OS security boundary.
- Actor deletion during callback or concurrent use can invalidate pointers: single-thread World owner; callback must not mutate registration or reenter `World` from another thread.
- Cross-platform float divergence can desynchronize network simulation: use authoritative state/decisions and reconcile; no deterministic lockstep claim across architectures.
- TaskSink failure can silently drop gameplay actions if not handled: bridge retains failed sends and increments counters; host logs or alerts if failures persist.

## Observability

`Metrics` stores counts of ticks, inputs received, inputs filtered, inbox drops, memory evictions, decisions changed and agents evaluated. `BridgeMetrics` stores accepted commands, hook rejects, invalid hook outputs, sink failures and retries. These are in-process counters (not structured logs or exporters). The host should log decision changes with `actor`, `sequence`, `intent`, `target` and tick ID, and measure wall-clock p50/p95/p99 simulation and adapter latency. Avoid logging player identifiers or private data unless needed.

## Rollout / rollback

Use the same `World` interface behind a feature flag in the game. During a trial, run a read-only shadow `World` receiving the same inputs but discard its tasks; compare decisions and tick duration with the existing NPC system. Enable task dispatch for a limited actor cohort before all NPCs. Roll back by disabling the `TaskBridge` and returning task ownership to the engine's original behaviour; do not unload the native library until all worlds are destroyed. The v0.1.0 schema is in-memory only, so no persistent migration is needed.

## Deferred design areas

No built-in behaviour tree editor, utility curve assets, memory persistence, GOAP planning, schedule simulation, squad tactics, cover system, navmesh path solver, dialogue model, blackboard replication, NPC LOD or engine-specific plugin is shipped in 0.1.0. These should be adapters or optional modules, not unconditional core dependencies.
