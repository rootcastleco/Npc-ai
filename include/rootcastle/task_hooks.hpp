// Rootcastle NPC AI - portable task hooks and engine task bridge.
// Copyright (c) 2026 Batuhan Ayribas. MIT License.
#pragma once

#include "rootcastle/npc.hpp"

#include <array>
#include <cstdint>
#include <map>

namespace rootcastle::npc {

// Symbolic gameplay tasks, NOT binary detours or game-specific addresses.
// Each adapter maps these requests to the host game's own task API.
enum class TaskKind : std::uint8_t {
    Any = 0,
    StandStill,
    WanderStandard,
    FollowNavMeshToCoord,
    SmartFleePed,
    ReactAndFleePed,
    CombatPed,
    AssistPed,
    Investigate,
    PatrolRoute
};

RC_NPC_CPP_API const char* task_name(TaskKind kind) noexcept;

struct TaskCommand {
    TaskKind kind = TaskKind::StandStill;
    EntityId actor = 0;
    EntityId target = 0;
    Vec3 destination{};
    float utility = 0.0f;
    std::uint64_t sequence = 0;
};

enum class HookVerdict : std::uint8_t { Continue, Cancel };
using TaskHook = HookVerdict (*)(TaskCommand& command, void* user_data) noexcept;

// Fixed-capacity, ordered interception pipeline. A hook can modify a task
// or cancel it. Hooks must not mutate registration while being invoked.
class RC_NPC_CPP_API HookPipeline {
public:
    static constexpr std::size_t kMaxHooks = 128;
    std::uint64_t add(TaskKind kind, TaskHook callback, void* user_data = nullptr) noexcept;
    bool remove(std::uint64_t handle) noexcept;
    HookVerdict apply(TaskCommand& command) const noexcept;
    std::size_t count() const noexcept { return count_; }

private:
    struct Slot {
        std::uint64_t handle = 0;
        TaskKind kind = TaskKind::Any;
        TaskHook callback = nullptr;
        void* user = nullptr;
    };
    std::array<Slot, kMaxHooks> slots_{};
    std::size_t count_ = 0;
    std::uint64_t next_handle_ = 1;
};

class TaskSink {
public:
    virtual ~TaskSink() = default;
    // Return true only once the host engine accepted the command.
    virtual bool submit(const TaskCommand& command) = 0;
};

struct BridgeMetrics {
    std::uint64_t accepted = 0;
    std::uint64_t rejected_by_hook = 0;
    std::uint64_t invalid_hook_output = 0;
    std::uint64_t sink_failures = 0;
    std::uint64_t retried = 0;
};

// Calls remain on the engine's thread. Failed commands are retained and
// retried on the next flush (not sent every tick unless something changes).
class RC_NPC_CPP_API TaskBridge {
public:
    TaskBridge(World& world, TaskSink& sink, const HookPipeline* hooks = nullptr) noexcept;
    void flush();
    const BridgeMetrics& metrics() const noexcept { return metrics_; }
    std::size_t pending_count() const noexcept { return pending_.size(); }

private:
    World& world_;
    TaskSink& sink_;
    const HookPipeline* hooks_;
    struct Pending { TaskCommand command{}; bool attempted = false; };
    std::map<EntityId, Pending> pending_;
    std::map<EntityId, std::uint64_t> last_seen_;
    BridgeMetrics metrics_{};
};

} // namespace rootcastle::npc
