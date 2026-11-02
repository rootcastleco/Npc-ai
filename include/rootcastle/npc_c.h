/* Rootcastle NPC AI - stable C ABI for game engines and scripting FFI.
 * Copyright (c) 2026 Batuhan Ayribas. MIT License.
 */
#ifndef ROOTCASTLE_NPC_C_H
#define ROOTCASTLE_NPC_C_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(RC_NPC_SHARED)
#  if defined(RC_NPC_EXPORTS)
#    define RC_NPC_API __declspec(dllexport)
#  else
#    define RC_NPC_API __declspec(dllimport)
#  endif
#else
#  define RC_NPC_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rc_world rc_world;

typedef enum rc_result {
    RC_OK = 0,
    RC_INVALID_ARGUMENT = 1,
    RC_ALREADY_EXISTS = 2,
    RC_NOT_FOUND = 3,
    RC_CAPACITY_EXCEEDED = 4,
    RC_OUT_OF_MEMORY = 5
} rc_result;

typedef enum rc_stimulus_kind {
    RC_SOUND = 1,
    RC_HOSTILE_SIGHT = 2,
    RC_NEUTRAL_SIGHT = 3,
    RC_DAMAGE = 4,
    RC_HELP_REQUEST = 5,
    RC_ALARM = 6,
    RC_CLEAR_THREAT = 7,
    RC_FRIENDLY_SIGHT = 8
} rc_stimulus_kind;

typedef enum rc_intent {
    RC_IDLE = 0, RC_WANDER = 1, RC_PATROL = 2, RC_INVESTIGATE = 3,
    RC_FLEE = 4, RC_ENGAGE = 5, RC_ASSIST = 6
} rc_intent;

typedef enum rc_relation {
    RC_NEUTRAL = 0, RC_FRIENDLY = 1, RC_HOSTILE = 2
} rc_relation;

typedef struct rc_vec3 { float x, y, z; } rc_vec3;
typedef struct rc_pose { rc_vec3 position, forward; float health; } rc_pose;

typedef struct rc_agent_config {
    uint64_t id;
    uint32_t faction, seed;
    float courage, aggression, curiosity, empathy, discipline;
    float sight_range, hearing_range, field_of_view_degrees;
    float memory_seconds, decision_interval_seconds, switch_margin;
    float wander_radius, flee_distance;
    uint32_t max_memories, max_stimuli;
    uint32_t can_fight, can_flee, can_investigate, can_assist, can_wander;
} rc_agent_config;

typedef struct rc_stimulus {
    uint64_t event_id;
    uint32_t kind;
    uint64_t source;
    rc_vec3 position;
    float strength;
} rc_stimulus;

typedef struct rc_decision {
    uint64_t actor;
    uint32_t intent, reason;
    uint64_t target;
    rc_vec3 destination;
    float utility;
    uint64_t sequence;
} rc_decision;

typedef struct rc_metrics {
    uint64_t ticks, received, filtered, dropped, memory_evictions;
    uint64_t decisions, agents_evaluated;
} rc_metrics;

/* ABI: 0xMMmm where MM is major and mm minor; currently 0x0100. */
RC_NPC_API uint32_t rc_abi_version(void);
RC_NPC_API rc_agent_config rc_default_agent_config(uint64_t id);
RC_NPC_API rc_pose rc_default_pose(void);
RC_NPC_API rc_result rc_world_create(rc_world** out);
RC_NPC_API void rc_world_destroy(rc_world* world);
RC_NPC_API rc_result rc_world_add(rc_world* world, rc_agent_config config, rc_pose pose);
RC_NPC_API rc_result rc_world_remove(rc_world* world, uint64_t id);
RC_NPC_API rc_result rc_world_update_pose(rc_world* world, uint64_t id, rc_pose pose);
RC_NPC_API rc_result rc_world_set_patrol(rc_world* world, uint64_t id, const rc_vec3* points, size_t count);
RC_NPC_API rc_result rc_world_arrived(rc_world* world, uint64_t id);
RC_NPC_API rc_result rc_world_submit(rc_world* world, uint64_t actor, rc_stimulus stimulus);
RC_NPC_API rc_result rc_world_broadcast(rc_world* world, rc_stimulus stimulus, float radius);
RC_NPC_API rc_result rc_world_set_relation(rc_world* world, uint32_t from, uint32_t to, uint32_t relation);
RC_NPC_API rc_result rc_world_observe(rc_world* world, uint64_t observer, uint64_t target,
                                      uint32_t line_of_sight, uint64_t event_id);
RC_NPC_API rc_result rc_world_tick(rc_world* world, float delta_seconds);
RC_NPC_API rc_result rc_world_get_decision(const rc_world* world, uint64_t id, rc_decision* out);
RC_NPC_API size_t rc_world_change_count(const rc_world* world);
RC_NPC_API rc_result rc_world_get_change(const rc_world* world, size_t index, rc_decision* out);
RC_NPC_API rc_result rc_world_get_metrics(const rc_world* world, rc_metrics* out);

#ifdef __cplusplus
}
#endif
#endif
