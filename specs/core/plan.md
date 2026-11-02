# Implementation plan

1. Pure `World` core (`include/rootcastle/npc.hpp`, `src/npc.cpp`) implements bounded agents, perception, memory and utility scoring.
2. Optional `TaskBridge` and `HookPipeline` (`task_hooks.hpp/.cpp`) emit named gameplay commands behind a `TaskSink` adapter; validate all changed hook outputs.
3. `npc_c.h/.cpp` implements an opaque, exception-contained C ABI for cross-language interop. Unity P/Invoke declarations provide a starting point.
4. CMake builds static by default, shared on request. Samples demonstrate polling and task dispatch. CTest and multi-OS CI provide regression evidence.
5. Documentation defines engine boundary, threat model, instrumentation, release limitations, example benchmark and rollback strategy.

## Boundary invariants

- No game-specific offsets or runtime patching.
- No engine calls in `World::tick`.
- Decisions emitted only on state change, monotonically sequenced per world.
- Known invalid values never enter actor state; hooks cannot redirect actor identity.
- Sink failures retain pending commands until accepted or superseded.
- World and TaskBridge are single-owner objects; FFI handles require explicit destroy.

## Test / rollback

Test contracts FR-001..FR-006 against `tests/test_npc.cpp`. Run ASan/UBSan, shared-library build and CSmoke test; for game rollout use a feature flag at the adapter and disable task dispatch on performance or gameplay regression. No persistence schema migration in v0.1.0.
