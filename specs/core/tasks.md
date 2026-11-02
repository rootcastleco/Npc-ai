# Task checklist

- [x] T001 [FR-001] Add typed actor configuration, lifecycle and input validation; `test_validation`.
- [x] T002 [FR-002] Add bounded event queue, perception, memory TTL and clearing; `test_hearing_and_sight`, `test_personality_and_expiry`, `test_queue_and_memory_bounds`.
- [x] T003 [FR-003] Add personality scoring, faction observation, patrol/arrival and deterministic RNG; `test_factions_and_nav`, `test_patrol_arrival_and_incapacitation`, `test_determinism`.
- [x] T004 [FR-004] Add task-kind names, hook registry, task sink bridge with validation/retry; `test_hooks`.
- [x] T005 [FR-005] Add stable opaque C ABI, P/Invoke contract and CMake shared build; `test_c_abi`.
- [x] T006 [FR-006] Add counters, console/adapter samples, benchmark and Linux sanitizer pipeline; `npc_bench`, CI definition.
- [ ] T007 [FR-005] Build and execute Unity plugin in an actual Unity project; requires host integration and editor installation.
- [ ] T008 [FR-003] Validate game-design scores against playtest telemetry from at least one target game; requires field data.
- [ ] T009 [FR-006] Record p95/p99 tick and end-to-end adapter latencies with real event load; requires host scene and profiling instrumentation.
