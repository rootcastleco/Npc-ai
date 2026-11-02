#include "rootcastle/npc.hpp"

#include <iostream>

using namespace rootcastle::npc;

const char* intent_name(Intent intent) {
    switch (intent) {
        case Intent::Idle: return "idle";
        case Intent::Wander: return "wander";
        case Intent::Patrol: return "patrol";
        case Intent::Investigate: return "investigate";
        case Intent::Flee: return "flee";
        case Intent::Engage: return "engage";
        case Intent::Assist: return "assist";
    }
    return "unknown";
}

int main() {
    World world;
    Config civilian;
    civilian.id = 1;
    civilian.personality.courage = 0.1f;
    civilian.personality.aggression = 0.05f;
    Config guard;
    guard.id = 2;
    guard.faction = 7;
    guard.personality.courage = 0.95f;
    guard.personality.aggression = 0.95f;
    const Pose at_origin{{0,0,0},{1,0,0},1};
    if (world.add(civilian, at_origin) != Result::Ok ||
        world.add(guard, {{2,0,0},{1,0,0},1}) != Result::Ok) return 1;
    world.submit(1, {1, StimulusKind::Sound, 42, {5,0,0}, 0.7f});
    world.submit(2, {2, StimulusKind::HostileSight, 42, {5,0,0}, 1.0f});
    world.tick(0.1f);
    for (const Decision& decision : world.changes()) {
        std::cout << "actor=" << decision.actor << " intent=" << intent_name(decision.intent)
                  << " target=" << decision.target << " score=" << decision.utility << '\n';
    }
    world.submit(1, {3, StimulusKind::Damage, 42, {5,0,0}, 1.0f});
    world.tick(0.1f);
    const Decision* d = world.current(1);
    if (!d) return 1;
    std::cout << "civilian after damage: " << intent_name(d->intent) << '\n';
}
