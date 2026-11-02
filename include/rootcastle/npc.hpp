// Rootcastle NPC AI - deterministic, engine-agnostic behaviour runtime.
// Copyright (c) 2026 Batuhan Ayribas. MIT License.
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <vector>

namespace rootcastle::npc {

using EntityId = std::uint64_t;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class Result : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    AlreadyExists,
    NotFound,
    CapacityExceeded
};

enum class StimulusKind : std::uint8_t {
    Sound = 1,
    HostileSight = 2,
    NeutralSight = 3,
    Damage = 4,
    HelpRequest = 5,
    Alarm = 6,
    ClearThreat = 7,
    FriendlySight = 8
};

enum class Relation : std::uint8_t { Neutral = 0, Friendly = 1, Hostile = 2 };

enum class Intent : std::uint8_t {
    Idle = 0,
    Wander = 1,
    Patrol = 2,
    Investigate = 3,
    Flee = 4,
    Engage = 5,
    Assist = 6
};

enum class Reason : std::uint8_t {
    None = 0,
    Routine,
    Curiosity,
    Danger,
    Defence,
    AllyRequest,
    Incapacitated
};

struct Personality {
    float courage = 0.5f;
    float aggression = 0.25f;
    float curiosity = 0.5f;
    float empathy = 0.5f;
    float discipline = 0.5f;
};

struct Config {
    EntityId id = 0;
    std::uint32_t faction = 0;
    std::uint32_t seed = 1;
    Personality personality{};
    float sight_range = 25.0f;
    float hearing_range = 18.0f;
    float field_of_view_degrees = 160.0f;
    float memory_seconds = 8.0f;
    float decision_interval_seconds = 0.10f;
    float switch_margin = 0.08f;
    float wander_radius = 9.0f;
    float flee_distance = 14.0f;
    std::uint32_t max_memories = 16;
    std::uint32_t max_stimuli = 32;
    bool can_fight = true;
    bool can_flee = true;
    bool can_investigate = true;
    bool can_assist = true;
    bool can_wander = true;
};

struct Pose {
    Vec3 position{};
    Vec3 forward{1.0f, 0.0f, 0.0f};
    float health = 1.0f; // normalized [0,1]
};

// The adapter reports LOS-validated sightings and gameplay events. The core
// additionally checks distance and field of view; it does not do physics raycasts.
struct Stimulus {
    std::uint64_t event_id = 0;
    StimulusKind kind = StimulusKind::Sound;
    EntityId source = 0;
    Vec3 position{};
    float strength = 1.0f; // normalized [0,1]
};

struct Decision {
    EntityId actor = 0;
    Intent intent = Intent::Idle;
    Reason reason = Reason::None;
    EntityId target = 0;
    Vec3 destination{};
    float utility = 0.0f;
    std::uint64_t sequence = 0;
};

struct Metrics {
    std::uint64_t ticks = 0;
    std::uint64_t received = 0;
    std::uint64_t filtered = 0;
    std::uint64_t dropped = 0;
    std::uint64_t memory_evictions = 0;
    std::uint64_t decisions = 0;
    std::uint64_t agents_evaluated = 0;
};

// Single-thread owner. Call tick with a fixed dt for deterministic replay.
// No engine API, I/O, heap-owning callbacks, or background threads are used.
class World {
public:
    static constexpr std::size_t kMaximumAgents = 100000;
    static constexpr std::size_t kMaximumPatrolPoints = 128;

    Result add(Config config, Pose pose = {});
    Result remove(EntityId id);
    Result update_pose(EntityId id, Pose pose);
    Result submit(EntityId id, Stimulus event);
    Result broadcast(Stimulus event, float radius);
    Result set_relation(std::uint32_t observer_faction, std::uint32_t target_faction, Relation relation);
    Result observe(EntityId observer, EntityId target, bool line_of_sight, std::uint64_t event_id = 0);
    Result set_patrol(EntityId id, std::vector<Vec3> waypoints);
    Result arrived(EntityId id);
    Result tick(float dt_seconds);

    const Decision* current(EntityId id) const;
    const std::vector<Decision>& changes() const noexcept { return changes_; }
    const Metrics& metrics() const noexcept { return metrics_; }
    std::size_t size() const noexcept { return agents_.size(); }
    void reset_metrics() noexcept { metrics_ = {}; }

private:
    struct Memory {
        Stimulus event{};
        float remaining = 0.0f;
        float salience = 0.0f;
    };
    struct Agent {
        Config config{};
        Pose pose{};
        Vec3 home{};
        Vec3 wander_goal{};
        std::uint32_t rng = 1;
        std::deque<Stimulus> inbox{};
        std::deque<Memory> memories{};
        std::vector<Vec3> patrol{};
        std::size_t patrol_index = 0;
        float until_decision = 0.0f;
        bool replan = true;
        bool has_wander_goal = false;
        Decision decision{};
    };

    static bool valid_config(const Config& config);
    static bool valid_pose(const Pose& pose);
    static bool valid_stimulus(const Stimulus& event);
    static bool perceives(const Agent& agent, const Stimulus& event);
    void remember(Agent& agent, const Stimulus& event);
    void evaluate(Agent& agent);
    static Vec3 next_wander(Agent& agent);
    std::map<EntityId, Agent> agents_{};
    std::map<std::pair<std::uint32_t, std::uint32_t>, Relation> relations_{};
    std::vector<Decision> changes_{};
    Metrics metrics_{};
    std::uint64_t sequence_ = 0;
};

} // namespace rootcastle::npc
