// Copyright (c) 2026 Batuhan Ayribas. MIT License.
#include "rootcastle/npc_c.h"
#include "rootcastle/npc.hpp"

#include <new>
#include <vector>

using namespace rootcastle::npc;

struct rc_world { World implementation; };

namespace {
rc_result convert(Result r) noexcept { return static_cast<rc_result>(r); }
Vec3 convert(rc_vec3 p) noexcept { return {p.x,p.y,p.z}; }
rc_vec3 convert(Vec3 p) noexcept { return {p.x,p.y,p.z}; }
Pose convert(rc_pose p) noexcept { return {convert(p.position),convert(p.forward),p.health}; }
Config convert(rc_agent_config c) noexcept {
    Config v{};
    v.id=c.id; v.faction=c.faction; v.seed=c.seed;
    v.personality={c.courage,c.aggression,c.curiosity,c.empathy,c.discipline};
    v.sight_range=c.sight_range; v.hearing_range=c.hearing_range;
    v.field_of_view_degrees=c.field_of_view_degrees;
    v.memory_seconds=c.memory_seconds;
    v.decision_interval_seconds=c.decision_interval_seconds;
    v.switch_margin=c.switch_margin;
    v.wander_radius=c.wander_radius; v.flee_distance=c.flee_distance;
    v.max_memories=c.max_memories; v.max_stimuli=c.max_stimuli;
    v.can_fight=c.can_fight!=0; v.can_flee=c.can_flee!=0;
    v.can_investigate=c.can_investigate!=0; v.can_assist=c.can_assist!=0;
    v.can_wander=c.can_wander!=0;
    return v;
}
Stimulus convert(rc_stimulus s) noexcept {
    return {s.event_id, static_cast<StimulusKind>(s.kind), s.source,
            convert(s.position), s.strength};
}
rc_decision convert(Decision d) noexcept {
    return {d.actor, static_cast<uint32_t>(d.intent), static_cast<uint32_t>(d.reason),
            d.target, convert(d.destination), d.utility, d.sequence};
}
}

extern "C" {
uint32_t rc_abi_version(void) { return 0x0100; }
rc_agent_config rc_default_agent_config(uint64_t id) {
    const Config c = [] (uint64_t actor) { Config cfg; cfg.id=actor; return cfg; }(id);
    return {c.id,c.faction,c.seed,c.personality.courage,c.personality.aggression,
            c.personality.curiosity,c.personality.empathy,c.personality.discipline,
            c.sight_range,c.hearing_range,c.field_of_view_degrees,c.memory_seconds,
            c.decision_interval_seconds,c.switch_margin,c.wander_radius,c.flee_distance,
            c.max_memories,c.max_stimuli,static_cast<uint32_t>(c.can_fight),
            static_cast<uint32_t>(c.can_flee),static_cast<uint32_t>(c.can_investigate),
            static_cast<uint32_t>(c.can_assist),static_cast<uint32_t>(c.can_wander)};
}
rc_pose rc_default_pose(void) { return {{0,0,0},{1,0,0},1.0f}; }
rc_result rc_world_create(rc_world** out) {
    if (!out) return RC_INVALID_ARGUMENT;
    *out=nullptr;
    try { *out = new rc_world(); return RC_OK; }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
void rc_world_destroy(rc_world* world) { delete world; }
rc_result rc_world_add(rc_world* world, rc_agent_config cfg, rc_pose pose) {
    if (!world) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.add(convert(cfg),convert(pose))); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_remove(rc_world* world,uint64_t id) {
    return world ? convert(world->implementation.remove(id)) : RC_INVALID_ARGUMENT;
}
rc_result rc_world_update_pose(rc_world* world,uint64_t id,rc_pose pose) {
    return world ? convert(world->implementation.update_pose(id,convert(pose))) : RC_INVALID_ARGUMENT;
}
rc_result rc_world_set_patrol(rc_world* world,uint64_t id,const rc_vec3* points,size_t count) {
    if (!world || (!points && count>0)) return RC_INVALID_ARGUMENT;
    if (count > World::kMaximumPatrolPoints) return RC_CAPACITY_EXCEEDED;
    try {
        std::vector<Vec3> list;
        list.reserve(count);
        for (size_t i=0;i<count;++i) list.push_back(convert(points[i]));
        return convert(world->implementation.set_patrol(id,std::move(list)));
    } catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_arrived(rc_world* world,uint64_t id) {
    return world ? convert(world->implementation.arrived(id)) : RC_INVALID_ARGUMENT;
}
rc_result rc_world_submit(rc_world* world,uint64_t actor,rc_stimulus event) {
    if (!world) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.submit(actor,convert(event))); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_broadcast(rc_world* world,rc_stimulus event,float radius) {
    if (!world) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.broadcast(convert(event),radius)); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_set_relation(rc_world* world,uint32_t from,uint32_t to,uint32_t relation) {
    if (!world) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.set_relation(from,to,static_cast<Relation>(relation))); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_observe(rc_world* world,uint64_t observer,uint64_t target,uint32_t los,uint64_t event_id) {
    if (!world || los>1) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.observe(observer,target,los!=0,event_id)); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_tick(rc_world* world,float dt) {
    if (!world) return RC_INVALID_ARGUMENT;
    try { return convert(world->implementation.tick(dt)); }
    catch (...) { return RC_OUT_OF_MEMORY; }
}
rc_result rc_world_get_decision(const rc_world* world,uint64_t id,rc_decision* out) {
    if (!world || !out) return RC_INVALID_ARGUMENT;
    const Decision* d=world->implementation.current(id);
    if (!d) return RC_NOT_FOUND;
    *out=convert(*d); return RC_OK;
}
size_t rc_world_change_count(const rc_world* world) {
    return world ? world->implementation.changes().size() : 0;
}
rc_result rc_world_get_change(const rc_world* world,size_t index,rc_decision* out) {
    if (!world || !out) return RC_INVALID_ARGUMENT;
    const auto& changes=world->implementation.changes();
    if (index>=changes.size()) return RC_NOT_FOUND;
    *out=convert(changes[index]); return RC_OK;
}
rc_result rc_world_get_metrics(const rc_world* world,rc_metrics* out) {
    if (!world || !out) return RC_INVALID_ARGUMENT;
    const Metrics& m=world->implementation.metrics();
    *out={m.ticks,m.received,m.filtered,m.dropped,m.memory_evictions,
          m.decisions,m.agents_evaluated};
    return RC_OK;
}
}
