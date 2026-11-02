#include "rootcastle/npc.hpp"
#include "rootcastle/npc_c.h"
#include "rootcastle/task_hooks.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace rootcastle::npc;

#define CHECK(EXPR) do { if (!(EXPR)) { std::cerr << __FILE__ << ':' << __LINE__ \
    << " FAILED: " << #EXPR << '\n'; std::exit(1); } } while(false)

Config pedestrian(EntityId id) {
    Config cfg;
    cfg.id=id;
    cfg.seed=static_cast<std::uint32_t>(id);
    cfg.personality.courage=0.1f;
    cfg.personality.aggression=0.1f;
    return cfg;
}

void test_validation() {
    World w;
    Config c=pedestrian(0);
    CHECK(w.add(c)==Result::InvalidArgument);
    c.id=1;
    c.seed=1;
    c.field_of_view_degrees=400;
    CHECK(w.add(c)==Result::InvalidArgument);
    c.field_of_view_degrees=160;
    CHECK(w.add(c)==Result::Ok);
    CHECK(w.add(c)==Result::AlreadyExists);
    CHECK(w.tick(-1)==Result::InvalidArgument);
    CHECK(w.tick(std::numeric_limits<float>::quiet_NaN())==Result::InvalidArgument);
    CHECK(w.tick(1.1f)==Result::InvalidArgument);
    CHECK(w.submit(1,{0,StimulusKind::Damage,9,{0,0,0},2})==Result::InvalidArgument);
    CHECK(w.remove(9)==Result::NotFound);
    CHECK(w.remove(1)==Result::Ok);
    CHECK(w.current(1)==nullptr);
}

void test_hearing_and_sight() {
    World w;
    Config c=pedestrian(1);
    c.hearing_range=10;
    c.field_of_view_degrees=90;
    CHECK(w.add(c)==Result::Ok);
    CHECK(w.submit(1,{1,StimulusKind::HostileSight,3,{-3,0,0},1})==Result::Ok);
    CHECK(w.submit(1,{2,StimulusKind::Sound,4,{100,0,0},1})==Result::Ok);
    CHECK(w.tick(0.1f)==Result::Ok);
    CHECK(w.current(1)->intent==Intent::Wander);
    CHECK(w.metrics().filtered==2);
    CHECK(w.submit(1,{3,StimulusKind::HostileSight,3,{3,0,0},1})==Result::Ok);
    CHECK(w.tick(0.1f)==Result::Ok);
    CHECK(w.current(1)->intent==Intent::Flee);
    CHECK(w.current(1)->target==3);
    CHECK(w.current(1)->destination.x < 0);
}

void test_personality_and_expiry() {
    World w;
    Config shy=pedestrian(1);
    shy.memory_seconds=0.2f;
    Config brave=pedestrian(2);
    brave.personality.courage=1;
    brave.personality.aggression=1;
    CHECK(w.add(shy)==Result::Ok);
    CHECK(w.add(brave)==Result::Ok);
    const Stimulus threat{5,StimulusKind::Damage,999,{2,0,0},1};
    w.submit(1,threat);
    w.submit(2,threat);
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Flee);
    CHECK(w.current(2)->intent==Intent::Engage);
    w.tick(0.25f);
    CHECK(w.current(1)->intent==Intent::Wander);
    CHECK(w.current(2)->intent==Intent::Engage);
    w.submit(2,{6,StimulusKind::ClearThreat,999,{0,0,0},1});
    w.tick(0.1f);
    CHECK(w.current(2)->intent==Intent::Wander);
}

void test_factions_and_nav() {
    World w;
    Config c=pedestrian(1);
    c.faction=10;
    Config enemy=pedestrian(2);
    enemy.faction=20;
    w.add(c);
    w.add(enemy,{{2,0,0},{1,0,0},1});
    CHECK(w.set_relation(10,20,Relation::Hostile)==Result::Ok);
    CHECK(w.observe(1,2,true,1)==Result::Ok);
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Flee);
    CHECK(w.current(1)->target==2);
    CHECK(w.observe(2,1,false,2)==Result::Ok);
    w.tick(0.1f);
    CHECK(w.current(2)->intent==Intent::Wander);
    CHECK(w.set_relation(10,20,Relation::Neutral)==Result::Ok);
    w.submit(1,{3,StimulusKind::ClearThreat,2,{0,0,0},1});
    w.observe(1,2,true,4);
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Investigate);
    CHECK(w.set_patrol(1,{{1,0,0},{2,0,0}})==Result::Ok);
    w.tick(0.1f);
    // An existing investigation can outrank a routine patrol.
    w.submit(1,{5,StimulusKind::ClearThreat,0,{0,0,0},1});
    w.tick(0.5f);
    CHECK(w.current(1)!=nullptr);
}

void test_patrol_arrival_and_incapacitation() {
    World w;
    Config c=pedestrian(1);
    c.can_wander=false;
    CHECK(w.add(c)==Result::Ok);
    CHECK(w.set_patrol(1,{{1,0,0},{3,0,0}})==Result::Ok);
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Patrol);
    CHECK(std::fabs(w.current(1)->destination.x-1)<0.01f);
    w.arrived(1);
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Patrol);
    CHECK(std::fabs(w.current(1)->destination.x-3)<0.01f);
    w.update_pose(1,{{0,0,0},{1,0,0},0});
    w.tick(0.1f);
    CHECK(w.current(1)->intent==Intent::Idle);
    CHECK(w.current(1)->reason==Reason::Incapacitated);
}

void test_queue_and_memory_bounds() {
    World w;
    Config c=pedestrian(1);
    c.max_stimuli=2;
    c.max_memories=1;
    w.add(c);
    for (uint64_t i=1;i<=5;++i) CHECK(w.submit(1,{i,StimulusKind::Sound,i,{2,0,0},1})==Result::Ok);
    CHECK(w.metrics().dropped==3);
    w.tick(0.1f);
    CHECK(w.metrics().memory_evictions==1);
    CHECK(w.metrics().received==5);
}

std::vector<Decision> replay() {
    World w;
    for (EntityId id=1;id<=10;id++) w.add(pedestrian(id));
    std::vector<Decision> changes;
    for (int t=0;t<100;t++) {
        if (t%7==0) w.broadcast({static_cast<uint64_t>(t+1),StimulusKind::Sound,
            99,{0,0,0},0.8f},10);
        w.tick(1.0f/60.0f);
        for(const auto& d:w.changes()) changes.push_back(d);
    }
    return changes;
}

void test_determinism() {
    const auto a=replay();
    const auto b=replay();
    CHECK(!a.empty() && a.size()==b.size());
    for(size_t i=0;i<a.size();i++) {
        CHECK(a[i].actor==b[i].actor);
        CHECK(a[i].intent==b[i].intent);
        CHECK(a[i].target==b[i].target);
        CHECK(a[i].destination.x==b[i].destination.x);
        CHECK(a[i].destination.y==b[i].destination.y);
        CHECK(a[i].utility==b[i].utility);
        CHECK(a[i].sequence==b[i].sequence);
    }
}

class RetrySink : public TaskSink {
public:
    bool accept=false;
    std::vector<TaskCommand> commands;
    bool submit(const TaskCommand& command) override {
        if (!accept) return false;
        commands.push_back(command);
        return true;
    }
};
HookVerdict translate(TaskCommand& cmd, void*) noexcept { cmd.destination.x=12; return HookVerdict::Continue; }
HookVerdict corrupt(TaskCommand& cmd, void*) noexcept { cmd.actor=999; return HookVerdict::Continue; }
HookVerdict cancel(TaskCommand&, void*) noexcept { return HookVerdict::Cancel; }

void test_hooks() {
    World w;
    w.add(pedestrian(1));
    RetrySink sink;
    HookPipeline hooks;
    const auto token=hooks.add(TaskKind::WanderStandard,translate);
    CHECK(token!=0);
    TaskBridge bridge(w,sink,&hooks);
    w.tick(0.1f);
    bridge.flush();
    CHECK(bridge.metrics().sink_failures==1);
    CHECK(bridge.pending_count()==1);
    sink.accept=true;
    bridge.flush();
    CHECK(bridge.metrics().retried==1);
    CHECK(bridge.pending_count()==0);
    CHECK(sink.commands.size()==1);
    CHECK(sink.commands[0].destination.x==12);
    bridge.flush();
    CHECK(sink.commands.size()==1);
    CHECK(hooks.remove(token));
    CHECK(!hooks.remove(token));
    // New decisions can be suppressed or validated at the adapter boundary.
    const auto corrupt_token=hooks.add(TaskKind::Any,corrupt);
    w.submit(1,{1,StimulusKind::Sound,5,{3,0,0},1});
    w.tick(0.1f);
    bridge.flush();
    CHECK(bridge.metrics().invalid_hook_output==1);
    hooks.remove(corrupt_token);
    hooks.add(TaskKind::Any,cancel);
    w.submit(1,{2,StimulusKind::Damage,5,{2,0,0},1});
    w.tick(0.1f);
    bridge.flush();
    CHECK(bridge.metrics().rejected_by_hook==1);
}

void test_c_abi() {
    CHECK(rc_abi_version()==0x0100);
    rc_world* w=nullptr;
    CHECK(rc_world_create(nullptr)==RC_INVALID_ARGUMENT);
    CHECK(rc_world_create(&w)==RC_OK && w!=nullptr);
    rc_agent_config c=rc_default_agent_config(55);
    c.aggression=1; c.courage=1;
    CHECK(rc_world_add(w,c,rc_default_pose())==RC_OK);
    CHECK(rc_world_add(w,c,rc_default_pose())==RC_ALREADY_EXISTS);
    CHECK(rc_world_submit(w,55,{1,RC_DAMAGE,100,{2,0,0},1})==RC_OK);
    CHECK(rc_world_tick(w,0.1f)==RC_OK);
    rc_decision d{};
    CHECK(rc_world_get_decision(w,55,&d)==RC_OK && d.intent==RC_ENGAGE);
    CHECK(rc_world_change_count(w)==1);
    CHECK(rc_world_get_change(w,0,&d)==RC_OK && d.actor==55);
    rc_metrics m{};
    CHECK(rc_world_get_metrics(w,&m)==RC_OK && m.ticks==1);
    CHECK(rc_world_set_patrol(w,55,nullptr,1)==RC_INVALID_ARGUMENT);
    CHECK(rc_world_observe(w,55,55,1,1)==RC_INVALID_ARGUMENT);
    rc_world_destroy(w);
}

int main() {
    test_validation();
    test_hearing_and_sight();
    test_personality_and_expiry();
    test_factions_and_nav();
    test_patrol_arrival_and_incapacitation();
    test_queue_and_memory_bounds();
    test_determinism();
    test_hooks();
    test_c_abi();
    std::cout << "All Rootcastle NPC AI regression tests passed.\n";
}
