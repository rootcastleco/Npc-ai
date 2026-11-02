#include "rootcastle/npc.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace rootcastle::npc;

int main(int argc, char** argv) {
    const int n = argc > 1 ? std::atoi(argv[1]) : 1000;
    const int frames = argc > 2 ? std::atoi(argv[2]) : 300;
    if (n <= 0 || n > 100000 || frames <= 0 || frames > 100000) return 2;
    World world;
    for (int i = 0; i < n; ++i) {
        Config cfg;
        cfg.id = static_cast<EntityId>(i+1);
        cfg.seed = static_cast<std::uint32_t>(i+1);
        cfg.decision_interval_seconds = 0.2f;
        Pose pose;
        pose.position = {static_cast<float>(i % 200),static_cast<float>(i / 200),0};
        if (world.add(cfg,pose) != Result::Ok) return 3;
    }
    const auto start = std::chrono::steady_clock::now();
    for (int frame=0; frame<frames; ++frame) {
        if (world.tick(1.0f / 60.0f) != Result::Ok) return 4;
    }
    const auto end = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double,std::milli>(end-start).count();
    std::cout << "agents=" << n << " ticks=" << frames << " total_ms=" << ms
              << " ms_per_tick=" << ms/frames
              << " evaluations=" << world.metrics().agents_evaluated
              << " decisions=" << world.metrics().decisions << '\n';
}
