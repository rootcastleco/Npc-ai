# Rootcastle NPC AI v0.1.0 - product specification

## Scope

An independent NPC decision system usable from portable C++20 games and foreign-function bindings. The user/game developer supplies observations and executes actions through an adapter. The library must be usable without external frameworks, proprietary runtime data or game-specific addresses.

### Explicit exclusions

Native game binary detouring, navmesh implementation, animation, ragdoll, physics, weapons/damage, perception raycasts, dialogue LLMs, network service, persistence and complete engine plugins.

## User stories and functional requirements

- **US-01 / FR-001:** As a developer, I can add/update/remove many uniquely identified agents with bounded configuration and finite poses. Invalid values are rejected without corrupting state.
- **US-02 / FR-002:** As a developer, I can deliver sound/sight/damage/help/clear events. Events are filtered by range/FOV or accepted as trusted direct signals; each agent has bounded inbox and expiring memory.
- **US-03 / FR-003:** As a designer, I can tune personality and faction relations so characters may wander, patrol, investigate, flee, engage or assist. A fixed input sequence yields the same decisions on a fixed build/platform.
- **US-04 / FR-004:** As an engine author, I can receive changed decisions, intercept a symbolic task, cancel/rewrite it, validate outputs and route it to a task sink with retries after transient sink failures.
- **US-05 / FR-005:** As a Unity or C user, I can create/destroy worlds, send events and retrieve decisions through a native C ABI without using C++ types.
- **US-06 / FR-006:** As an operator, I can observe bounded queue loss, evaluation and decision counts and reproduce tests/benchmarks without network dependencies.

## Acceptance scenarios

- **AC-01:** Invalid config and non-finite tick input return explicit error; duplicate ID returns AlreadyExists.
- **AC-02:** A threat behind a narrow vision cone is ignored. A valid nearby threat makes a timid civilian flee and a brave/aggressive actor engage.
- **AC-03:** Memory events expire according to configured TTL; ClearThreat removes remembered target threats.
- **AC-04:** Faction observation uses directed relation and host-validated LOS. Patrol advances after arrival; dead actors return Idle.
- **AC-05:** Two runs with identical seeds, events and tick intervals return identical decision sequences within one build.
- **AC-06:** Hook cancellation blocks dispatch, invalid hook output is rejected, and sink failure is retried without duplicate successful dispatch.
- **AC-07:** C header compiles for C and C++ and ABI decisions match C++ results.
- **AC-08:** CMake debug/release builds and CTest pass; CI covers desktop targets and Linux sanitizers.

## Constraints

Single-thread world ownership; bounded per-agent memory and events; no forced engine dependencies; MIT licensing; no externally copied proprietary game assets. Inactive NPCs still incur per-tick iteration; the host can partition/LOD as needed.

## Assumptions and known limitations

- Host events are trusted for source classification and LOS, since the library has no physics queries.
- Target entities beyond registered agents can appear as stimulus source IDs; host resolves them.
- Flee and wander destinations are candidates, not guaranteed navigable paths.
- Floating-point lockstep equality is not guaranteed between architectures.
- Bounded queues use drop-oldest rather than priority scheduling.

## Proposed measurement targets

Deterministic regression tests pass, no sanitizer errors, simulation tick cost and agent/event budget recorded from a representative machine. In-game p95/p99 latency targets are host-specific and must be verified during integration.
