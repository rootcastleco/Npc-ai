# Performance measurement

## Scope

This benchmark times the simulation `World::tick` on synthetic actors. It does not include host raycasts, navmesh, graphics, animation, scripting, memory allocations from a real game, combat or network round trips. Do not translate these numbers directly into in-game frame rates.

## Reproduce

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRC_NPC_BUILD_BENCH=ON
cmake --build build -j
./build/npc_bench 1000 300
./build/npc_bench 10000 300
```

`npc_bench [agents] [ticks]` creates the specified number of actors, simulates them for `ticks` at 60 Hz and prints total elapsed milliseconds, average milliseconds per tick, evaluations and decision changes. Record CPU model, compiler version, OS, CMake build type, thread load and game sensor volume. For representative p95/p99, drive the benchmark with realistic event rates, actor turnover and patrol routes; do not extrapolate this idle-agent workload.

## Example local measurement (development sandbox)

Linux container, GCC 14.2, initial Debug build, 1,000 agents, 300 ticks, no dynamic stimuli: `90.54 ms total`, `0.302 ms/tick` average. It is an indicative one-off measurement, not a guaranteed budget, median, tail latency or independent third-party result.

## Performance goals (targets, not verified claims)

- Host-defined budget for AI tick and event queues, measured at p95/p99 under representative gameplay.
- Zero out-of-range writes or memory leaks under ASan/UBSan.
- No unbounded per-agent event or memory queues.
- Profiling-driven optimization before adding spatial partitioning or parallel state mutation.
