#include "rootcastle/npc.hpp"
#include "rootcastle/task_hooks.hpp"

#include <iostream>

using namespace rootcastle::npc;

// Replace this class with Unity, Unreal, Godot, proprietary engine, ECS, etc.
// The runtime never calls or links to engine-specific APIs directly.
class LoggingSink : public TaskSink {
public:
    bool submit(const TaskCommand& command) override {
        std::cout << task_name(command.kind) << " actor=" << command.actor
                  << " target=" << command.target << " xyz=("
                  << command.destination.x << ',' << command.destination.y << ','
                  << command.destination.z << ")\n";
        return true;
    }
};

// Hook names match gameplay concepts; they do NOT patch executable memory.
HookVerdict limit_wander(TaskCommand& command, void*) noexcept {
    if (command.kind != TaskKind::WanderStandard) return HookVerdict::Continue;
    command.destination.x = 3.0f;
    return HookVerdict::Continue;
}

int main() {
    World world;
    Config config;
    config.id = 17;
    world.add(config);
    LoggingSink engine;
    HookPipeline hooks;
    hooks.add(TaskKind::WanderStandard, limit_wander);
    TaskBridge bridge(world, engine, &hooks);
    world.tick(0.1f);
    bridge.flush(); // only changed decisions become host engine commands
}
