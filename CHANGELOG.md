# Changelog

## 0.1.0 - 2026-11-03

- Initial standalone C++20 NPC simulation runtime with deterministic, utility-based decision selection.
- Bounded sensory inputs, directional faction relationships, sight/hearing filters, actor memory, configurable personality, patrol and arrival handling.
- Portable symbolic task hooks with mutation, veto and host adapter retry semantics.
- Stable C ABI v1 for engine FFI, Unity P/Invoke declarations, CMake install/export.
- Regression tests, examples, CI matrix and repeatable load benchmark.

### Non-goals in 0.1.0
- Built-in navmesh, physics/LOS, locomotion, animation, combat resolution or network replication.
- Engine plugin binaries and automated game integration.
