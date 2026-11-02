// Rootcastle NPC AI - deterministic utility-based task selection.
// Copyright (c) 2026 Batuhan Ayribas. MIT License.
#include "rootcastle/npc.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rootcastle::npc {
namespace {
constexpr float kEpsilon = 0.00001f;
constexpr float kPi = 3.14159265358979323846f;

bool finite(float value) { return std::isfinite(value); }
bool valid(Vec3 v) { return finite(v.x) && finite(v.y) && finite(v.z); }
float clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }
Vec3 subtract(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
Vec3 plus(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
Vec3 scale(Vec3 v, float factor) { return {v.x*factor, v.y*factor, v.z*factor}; }
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
float length_sq(Vec3 v) { return dot(v,v); }
float distance_sq(Vec3 a, Vec3 b) { return length_sq(subtract(a,b)); }
Vec3 normalized(Vec3 v) {
    const float squared = length_sq(v);
    if (!finite(squared) || squared < kEpsilon) return {1,0,0};
    return scale(v, 1.0f / std::sqrt(squared));
}
float unit(std::uint32_t& rng) {
    // xorshift32: only deterministic variety, NOT cryptographic randomness.
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return static_cast<float>(rng & 0x00ffffffu) / 16777216.0f;
}
bool valid_kind(StimulusKind kind) {
    return kind >= StimulusKind::Sound && kind <= StimulusKind::FriendlySight;
}
bool valid_personality(Personality p) {
    const float scores[] = {p.courage,p.aggression,p.curiosity,p.empathy,p.discipline};
    for (float n : scores) if (!finite(n) || n < 0.0f || n > 1.0f) return false;
    return true;
}
bool moved(Vec3 a, Vec3 b) { return distance_sq(a,b) > 0.25f; }
}

bool World::valid_config(const Config& c) {
    return c.id != 0 && valid_personality(c.personality) && c.seed != 0 &&
        finite(c.sight_range) && c.sight_range >= 0 && c.sight_range <= 10000 &&
        finite(c.hearing_range) && c.hearing_range >= 0 && c.hearing_range <= 10000 &&
        finite(c.field_of_view_degrees) && c.field_of_view_degrees > 0 && c.field_of_view_degrees <= 360 &&
        finite(c.memory_seconds) && c.memory_seconds >= 0.1f && c.memory_seconds <= 3600 &&
        finite(c.decision_interval_seconds) && c.decision_interval_seconds >= 0.001f &&
        c.decision_interval_seconds <= 10 && finite(c.switch_margin) && c.switch_margin >= 0 &&
        c.switch_margin <= 1 && finite(c.wander_radius) && c.wander_radius >= 0 &&
        c.wander_radius <= 10000 && finite(c.flee_distance) && c.flee_distance > 0 &&
        c.flee_distance <= 10000 && c.max_memories >= 1 && c.max_memories <= 64 &&
        c.max_stimuli >= 1 && c.max_stimuli <= 256;
}

bool World::valid_pose(const Pose& p) {
    return valid(p.position) && valid(p.forward) &&
        finite(p.health) && p.health >= 0 && p.health <= 1;
}

bool World::valid_stimulus(const Stimulus& e) {
    return valid_kind(e.kind) && valid(e.position) && finite(e.strength) &&
        e.strength >= 0 && e.strength <= 1;
}

Result World::add(Config config, Pose pose) {
    if (!valid_config(config) || !valid_pose(pose)) return Result::InvalidArgument;
    if (agents_.find(config.id) != agents_.end()) return Result::AlreadyExists;
    if (agents_.size() >= kMaximumAgents) return Result::CapacityExceeded;
    pose.forward = normalized(pose.forward);
    Agent agent{};
    agent.config = config;
    agent.pose = pose;
    agent.home = pose.position;
    agent.rng = config.seed;
    agent.decision.actor = config.id;
    agent.decision.destination = pose.position;
    agents_.emplace(config.id, std::move(agent));
    return Result::Ok;
}

Result World::remove(EntityId id) {
    if (agents_.erase(id) == 0) return Result::NotFound;
    return Result::Ok;
}

Result World::update_pose(EntityId id, Pose pose) {
    if (!valid_pose(pose)) return Result::InvalidArgument;
    auto it = agents_.find(id);
    if (it == agents_.end()) return Result::NotFound;
    pose.forward = normalized(pose.forward);
    it->second.pose = pose;
    if (pose.health <= 0) it->second.replan = true;
    return Result::Ok;
}

bool World::perceives(const Agent& a, const Stimulus& e) {
    const float d2 = distance_sq(a.pose.position, e.position);
    if (e.kind == StimulusKind::HostileSight || e.kind == StimulusKind::NeutralSight ||
        e.kind == StimulusKind::FriendlySight) {
        if (d2 > a.config.sight_range * a.config.sight_range) return false;
        if (d2 <= kEpsilon || a.config.field_of_view_degrees >= 360) return true;
        const float half_angle = a.config.field_of_view_degrees * kPi / 360.0f;
        return dot(a.pose.forward, normalized(subtract(e.position, a.pose.position))) >= std::cos(half_angle);
    }
    if (e.kind == StimulusKind::Sound || e.kind == StimulusKind::HelpRequest ||
        e.kind == StimulusKind::Alarm) {
        const float range = a.config.hearing_range * (0.5f + e.strength * 0.5f);
        return d2 <= range * range;
    }
    // Damage and explicitly routed clear signals are directly observed gameplay events.
    return true;
}

Result World::submit(EntityId id, Stimulus event) {
    if (!valid_stimulus(event)) return Result::InvalidArgument;
    auto it = agents_.find(id);
    if (it == agents_.end()) return Result::NotFound;
    auto& a = it->second;
    if (a.inbox.size() >= a.config.max_stimuli) {
        a.inbox.pop_front();
        ++metrics_.dropped;
    }
    a.inbox.push_back(event);
    ++metrics_.received;
    return Result::Ok;
}

Result World::broadcast(Stimulus event, float radius) {
    if (!valid_stimulus(event) || !finite(radius) || radius < 0 || radius > 10000)
        return Result::InvalidArgument;
    for (const auto& [id, agent] : agents_) {
        if (distance_sq(agent.pose.position, event.position) <= radius*radius) {
            // The bounded inbox has drop-oldest semantics under bursts.
            (void)submit(id, event);
        }
    }
    return Result::Ok;
}

Result World::set_relation(std::uint32_t from, std::uint32_t to, Relation relation) {
    if (static_cast<unsigned>(relation) > static_cast<unsigned>(Relation::Hostile))
        return Result::InvalidArgument;
    const auto key = std::make_pair(from, to);
    if (relation == Relation::Neutral) {
        relations_.erase(key);
        return Result::Ok;
    }
    if (relations_.size() >= 4096 && !relations_.contains(key))
        return Result::CapacityExceeded;
    relations_[key] = relation;
    return Result::Ok;
}

Result World::observe(EntityId observer, EntityId target, bool line_of_sight, std::uint64_t event_id) {
    if (observer == target || observer == 0 || target == 0) return Result::InvalidArgument;
    const auto from = agents_.find(observer);
    const auto to = agents_.find(target);
    if (from == agents_.end() || to == agents_.end()) return Result::NotFound;
    if (!line_of_sight || to->second.pose.health <= 0.0f) return Result::Ok;
    const auto from_faction = from->second.config.faction;
    const auto to_faction = to->second.config.faction;
    const auto lookup = relations_.find(std::make_pair(from_faction, to_faction));
    const Relation disposition = lookup != relations_.end() ? lookup->second :
        (from_faction != 0 && from_faction == to_faction ? Relation::Friendly : Relation::Neutral);
    const StimulusKind kind = disposition == Relation::Hostile ? StimulusKind::HostileSight :
        (disposition == Relation::Friendly ? StimulusKind::FriendlySight : StimulusKind::NeutralSight);
    return submit(observer, {event_id, kind, target, to->second.pose.position, 1.0f});
}

Result World::set_patrol(EntityId id, std::vector<Vec3> waypoints) {
    if (waypoints.size() > kMaximumPatrolPoints) return Result::CapacityExceeded;
    for (Vec3 p : waypoints) if (!valid(p)) return Result::InvalidArgument;
    auto it = agents_.find(id);
    if (it == agents_.end()) return Result::NotFound;
    it->second.patrol = std::move(waypoints);
    it->second.patrol_index = 0;
    it->second.replan = true;
    return Result::Ok;
}

Result World::arrived(EntityId id) {
    auto it = agents_.find(id);
    if (it == agents_.end()) return Result::NotFound;
    Agent& a = it->second;
    if (a.decision.intent == Intent::Patrol && !a.patrol.empty())
        a.patrol_index = (a.patrol_index + 1) % a.patrol.size();
    if (a.decision.intent == Intent::Wander) a.has_wander_goal = false;
    if (a.decision.intent == Intent::Investigate || a.decision.intent == Intent::Assist) {
        // Completing an investigation consumes just the selected event.
        const auto target = a.decision.target;
        const Vec3 destination = a.decision.destination;
        a.memories.erase(std::remove_if(a.memories.begin(), a.memories.end(),
            [target, destination](const Memory& m) {
                return m.event.source == target && !moved(m.event.position, destination);
            }), a.memories.end());
    }
    a.replan = true;
    return Result::Ok;
}

void World::remember(Agent& a, const Stimulus& e) {
    if (e.kind == StimulusKind::ClearThreat) {
        a.memories.erase(std::remove_if(a.memories.begin(), a.memories.end(),
            [&e](const Memory& m) {
                const bool threat = m.event.kind == StimulusKind::HostileSight ||
                    m.event.kind == StimulusKind::Damage || m.event.kind == StimulusKind::Alarm;
                return threat && (e.source == 0 || e.source == m.event.source);
            }), a.memories.end());
        return;
    }
    // Named sources refresh the same sense. Anonymous events use their event ID.
    for (Memory& m : a.memories) {
        const bool same = (e.source != 0 && e.source == m.event.source && e.kind == m.event.kind) ||
            (e.event_id != 0 && e.event_id == m.event.event_id);
        if (same) {
            m.event = e;
            m.remaining = a.config.memory_seconds;
            m.salience = std::max(e.strength, 0.01f);
            return;
        }
    }
    if (a.memories.size() >= a.config.max_memories) {
        a.memories.pop_front();
        ++metrics_.memory_evictions;
    }
    a.memories.push_back({e, a.config.memory_seconds, std::max(e.strength, 0.01f)});
}

Vec3 World::next_wander(Agent& a) {
    const float angle = unit(a.rng) * (2.0f*kPi);
    const float radius = (0.25f + 0.75f*unit(a.rng)) * a.config.wander_radius;
    a.wander_goal = plus(a.home, {std::cos(angle)*radius, std::sin(angle)*radius, 0});
    a.has_wander_goal = true;
    return a.wander_goal;
}

void World::evaluate(Agent& a) {
    ++metrics_.agents_evaluated;
    struct Candidate { Intent intent; Reason reason; EntityId target; Vec3 at; float score; };
    std::vector<Candidate> options{};
    options.reserve(a.memories.size()*2 + 3);
    const auto& p = a.config.personality;
    const float health = a.pose.health;
    options.push_back({Intent::Idle, health <= 0 ? Reason::Incapacitated : Reason::Routine,
                       0, a.pose.position, 0.05f});
    if (health > 0) {
        if (!a.patrol.empty())
            options.push_back({Intent::Patrol, Reason::Routine, 0, a.patrol[a.patrol_index], 0.27f});
        else if (a.config.can_wander)
            options.push_back({Intent::Wander, Reason::Routine, 0, a.has_wander_goal ?
                               a.wander_goal : next_wander(a), 0.16f});
        for (const Memory& memory : a.memories) {
            const Stimulus& e = memory.event;
            const float salience = clamp01(memory.salience * memory.remaining / a.config.memory_seconds);
            const bool threat = e.kind == StimulusKind::HostileSight ||
                e.kind == StimulusKind::Damage || e.kind == StimulusKind::Alarm;
            if (threat) {
                if (a.config.can_flee) {
                    const float score = 0.16f + 0.64f*(1-p.courage) + 0.34f*(1-health) +
                        0.28f*salience - 0.20f*p.aggression;
                    const Vec3 away = normalized(subtract(a.pose.position, e.position));
                    options.push_back({Intent::Flee, Reason::Danger, e.source,
                        plus(a.pose.position, scale(away,a.config.flee_distance)), clamp01(score)});
                }
                if (a.config.can_fight) {
                    const float score = 0.12f + 0.62f*p.aggression + 0.31f*p.courage +
                        0.15f*salience - 0.40f*(1-health);
                    options.push_back({Intent::Engage, Reason::Defence, e.source,
                                       e.position, clamp01(score)});
                }
            } else if (e.kind == StimulusKind::HelpRequest && a.config.can_assist) {
                options.push_back({Intent::Assist, Reason::AllyRequest, e.source, e.position,
                    clamp01(0.18f + 0.28f*p.empathy + 0.26f*p.discipline + 0.22f*salience)});
            } else if ((e.kind == StimulusKind::Sound || e.kind == StimulusKind::NeutralSight) &&
                       a.config.can_investigate) {
                options.push_back({Intent::Investigate, Reason::Curiosity, e.source, e.position,
                    clamp01(0.08f + 0.44f*p.curiosity + 0.25f*salience)});
            }
        }
    }
    // Highest utility wins; equal scores preserve deterministic insertion order.
    const Candidate* best = &options.front();
    for (const Candidate& candidate : options)
        if (candidate.score > best->score) best = &candidate;
    // Hysteresis prevents toggling between near-equivalent tasks every frame.
    if (!a.replan) for (const Candidate& candidate : options) {
        if (candidate.intent == a.decision.intent && candidate.target == a.decision.target &&
            candidate.score + a.config.switch_margin >= best->score) {
            best = &candidate;
            break;
        }
    }
    Decision next{a.config.id,best->intent,best->reason,best->target,best->at,best->score,0};
    const bool different = next.intent != a.decision.intent || next.target != a.decision.target ||
        next.reason != a.decision.reason || moved(next.destination,a.decision.destination);
    if (different) {
        next.sequence = ++sequence_;
        changes_.push_back(next);
        ++metrics_.decisions;
    } else next.sequence = a.decision.sequence;
    a.decision = next;
    a.replan = false;
}

Result World::tick(float dt_seconds) {
    if (!finite(dt_seconds) || dt_seconds <= 0 || dt_seconds > 1.0f)
        return Result::InvalidArgument;
    changes_.clear();
    ++metrics_.ticks;
    for (auto& [id, a] : agents_) {
        (void)id;
        for (Memory& memory : a.memories) memory.remaining -= dt_seconds;
        a.memories.erase(std::remove_if(a.memories.begin(), a.memories.end(),
            [](const Memory& memory) { return memory.remaining <= 0; }), a.memories.end());
        bool had_events = !a.inbox.empty();
        for (const Stimulus& event : a.inbox) {
            if (!perceives(a,event)) { ++metrics_.filtered; continue; }
            remember(a,event);
        }
        a.inbox.clear();
        a.until_decision -= dt_seconds;
        if (a.replan || had_events || a.until_decision <= 0) {
            evaluate(a);
            a.until_decision = a.config.decision_interval_seconds;
        }
    }
    return Result::Ok;
}

const Decision* World::current(EntityId id) const {
    const auto it = agents_.find(id);
    return it == agents_.end() ? nullptr : &it->second.decision;
}

} // namespace rootcastle::npc
