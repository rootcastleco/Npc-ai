# Integrating Rootcastle NPC AI into a game

## Minimal host contract

1. Assign each NPC a stable nonzero `EntityId` and a `Config`/`Pose`.
2. Every simulation step, update positions, direction and normalized health.
3. Submit game events through `submit` or `broadcast`; for other actors, use `observe(observer, target, LOS)` after engine raycasts. **Do not send invisible targets as sight events.**
4. Call `World::tick(fixed_dt)` on a single owner thread.
5. Read changed `Decision`s or call `TaskBridge::flush` to convert decisions into your engine's tasks.
6. Use engine navmesh/pathfinding for proposed destinations, and call `arrived(actor_id)` upon completion. Host must handle unreachable paths, invalid targets, animation and combat damage.
7. Remove NPCs from the world as soon as the game despawns them.

### C++ TaskSink example

```cpp
#include "rootcastle/task_hooks.hpp"
using namespace rootcastle::npc;

class GameTaskSink final : public TaskSink {
public:
    bool submit(const TaskCommand& c) override {
        switch (c.kind) {
            case TaskKind::StandStill:
                // engine.cancel_navigation(c.actor);
                return true;
            case TaskKind::PatrolRoute:
            case TaskKind::Investigate:
            case TaskKind::FollowNavMeshToCoord:
                // return engine.navigate_to(c.actor, c.destination);
                return true;
            case TaskKind::SmartFleePed:
                // return engine.flee_from(c.actor, c.target, c.destination);
                return true;
            case TaskKind::CombatPed:
                // return engine.engage(c.actor, c.target);
                return true;
            case TaskKind::AssistPed:
                // return engine.help(c.actor, c.target, c.destination);
                return true;
            case TaskKind::WanderStandard:
                // return engine.move_to_random_reachable(c.actor, c.destination);
                return true;
            default:
                return false;
        }
    }
};
```

This listing is a mapping template: replacing each comment with an actual engine call is required. Returning `true` from a real adapter before the engine accepts the task may lose decisions.

## Unreal Engine

Use a C++ `UWorldSubsystem` or actor component to own a `rootcastle::npc::World`. Read transforms and LOS on the game thread, call `World::tick` at a fixed interval, then map intents to AIController/Behavior Tree/MoveTo or Gameplay Ability actions. Use a `TaskSink` to make the boundary explicit. Include the library via your module's third-party dependency settings and add platform-specific binary packaging. No Unreal plugin module is shipped; the above integration requires a local adapter.

## Unity

Build the shared C ABI, then place the DLL/SO/DYLIB in an appropriate Unity `Assets/Plugins` folder for the target platform. [`bindings/unity/RootcastleNpc.cs`](../bindings/unity/RootcastleNpc.cs) declares P/Invoke signatures and blittable sequential structs. Attach a `MonoBehaviour` or update manager that owns the native handle, collects perception events, calls fixed-step `rc_world_tick` and copies decisions to Unity agents. Call `rc_world_destroy` on teardown. Native library loading, mobile architectures and Unity project-specific gameplay still require integration testing.

### Minimal C# call sequence

```csharp
IntPtr world;
RootcastleNpc.Create(out world);
var cfg = RootcastleNpc.DefaultConfig(1);
var pose = RootcastleNpc.DefaultPose();
RootcastleNpc.Add(world, cfg, pose);
RootcastleNpc.Tick(world, Time.fixedDeltaTime);
RootcastleNpc.Decision decision;
if (RootcastleNpc.GetDecision(world, 1, out decision) == 0) {
    // Drive your NavMeshAgent / Animator / combat controller.
}
RootcastleNpc.Destroy(world);
```

## Godot 4

Create a small GDExtension C++ wrapper around the `World` or use a compatible native C ABI binding. Convert Godot vectors to `Vec3`, physics queries to validated `Stimulus`s, and decisions into `NavigationAgent3D` target positions or other actor tasks. No precompiled GDExtension or official Godot class is included in this SDK.

## Dedicated servers / custom engines

The simulation has no rendering dependencies and can run server-side. Send only authoritative decisions or sanitized event inputs over the network, not arbitrary function pointers or hooks. For many agents, partition the world into simulation regions in the host; the library does not yet provide an optimized spatial index or distributed state replication.

## Gameplay event examples

```cpp
world.submit(victim_id, {event_id, StimulusKind::Damage,
                         attacker_id, attacker_location, 1.0f});
world.broadcast({event_id+1, StimulusKind::Sound, attacker_id,
                 shot_location, 0.8f}, 30.0f);
world.set_relation(guards_faction, bandits_faction, Relation::Hostile);
world.observe(guard_id, bandit_id, engine_has_line_of_sight, event_id+2);
```

## Troubleshooting

- NPC never moves: `World` only outputs decisions; wire a host task adapter.
- NPC does not see the target: verify FOV, sight range, facing vector and host LOS flag.
- NPC ignores an event: inspect `metrics().filtered`, whether event source was deduplicated, and memory TTL.
- NPC switches too often: increase `switch_margin` and decision interval; add host task-completion gating.
- Commands repeat after a sink error: expected retry behaviour; return `true` only after acceptance.
- Strange movement goal: generated flee/wander positions are proposed positions; project them onto your navmesh and account for blocked terrain.
- Android/iOS missing symbol: compile for the device architecture and ensure the engine loads the correct native library.
