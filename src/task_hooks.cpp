// Copyright (c) 2026 Batuhan Ayribas. MIT License.
#include "rootcastle/task_hooks.hpp"

#include <cmath>

namespace rootcastle::npc {
namespace {
TaskKind map_intent(Intent intent) noexcept {
    switch (intent) {
        case Intent::Wander: return TaskKind::WanderStandard;
        case Intent::Patrol: return TaskKind::PatrolRoute;
        case Intent::Investigate: return TaskKind::Investigate;
        case Intent::Flee: return TaskKind::SmartFleePed;
        case Intent::Engage: return TaskKind::CombatPed;
        case Intent::Assist: return TaskKind::AssistPed;
        default: return TaskKind::StandStill;
    }
}
bool valid_command(const TaskCommand& command, EntityId actor, std::uint64_t sequence) noexcept {
    return command.actor == actor && sequence == command.sequence &&
        command.kind != TaskKind::Any &&
        static_cast<unsigned>(command.kind) <= static_cast<unsigned>(TaskKind::PatrolRoute) &&
        std::isfinite(command.utility) && command.utility >= 0 && command.utility <= 1 &&
        std::isfinite(command.destination.x) && std::isfinite(command.destination.y) &&
        std::isfinite(command.destination.z);
}
} // namespace

const char* task_name(TaskKind kind) noexcept {
    switch (kind) {
        case TaskKind::Any: return "ANY";
        case TaskKind::StandStill: return "TASK_STAND_STILL";
        case TaskKind::WanderStandard: return "TASK_WANDER_STANDARD";
        case TaskKind::FollowNavMeshToCoord: return "TASK_FOLLOW_NAV_MESH_TO_COORD";
        case TaskKind::SmartFleePed: return "TASK_SMART_FLEE_PED";
        case TaskKind::ReactAndFleePed: return "TASK_REACT_AND_FLEE_PED";
        case TaskKind::CombatPed: return "TASK_COMBAT_PED";
        case TaskKind::AssistPed: return "TASK_ASSIST_PED";
        case TaskKind::Investigate: return "TASK_INVESTIGATE";
        case TaskKind::PatrolRoute: return "TASK_PATROL_ROUTE";
        default: return "UNKNOWN";
    }
}

std::uint64_t HookPipeline::add(TaskKind kind, TaskHook callback, void* user) noexcept {
    if (!callback || count_ >= kMaxHooks ||
        static_cast<unsigned>(kind) > static_cast<unsigned>(TaskKind::PatrolRoute) ||
        next_handle_ == 0) return 0;
    const auto handle = next_handle_++;
    slots_[count_++] = {handle, kind, callback, user};
    return handle;
}

bool HookPipeline::remove(std::uint64_t handle) noexcept {
    if (handle == 0) return false;
    for (std::size_t i = 0; i < count_; ++i) {
        if (slots_[i].handle != handle) continue;
        for (std::size_t j = i + 1; j < count_; ++j) slots_[j-1] = slots_[j];
        slots_[--count_] = {};
        return true;
    }
    return false;
}

HookVerdict HookPipeline::apply(TaskCommand& command) const noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
        const Slot& slot = slots_[i];
        if (slot.kind == TaskKind::Any || slot.kind == command.kind) {
            if (slot.callback(command, slot.user) == HookVerdict::Cancel)
                return HookVerdict::Cancel;
        }
    }
    return HookVerdict::Continue;
}

TaskBridge::TaskBridge(World& world, TaskSink& sink, const HookPipeline* hooks) noexcept
    : world_(world), sink_(sink), hooks_(hooks) {}

void TaskBridge::flush() {
    // A newer decision supersedes a command that the sink previously rejected.
    for (auto it=last_seen_.begin(); it != last_seen_.end();) {
        if (!world_.current(it->first)) it=last_seen_.erase(it);
        else ++it;
    }
    for (const Decision& d : world_.changes()) {
        if (last_seen_[d.actor] == d.sequence) continue;
        last_seen_[d.actor] = d.sequence;
        pending_[d.actor] = {{map_intent(d.intent), d.actor, d.target,
                              d.destination, d.utility, d.sequence}, false};
    }
    for (auto it = pending_.begin(); it != pending_.end();) {
        TaskCommand command = it->second.command;
        const Decision* current = world_.current(command.actor);
        if (!current || current->sequence != command.sequence) {
            it = pending_.erase(it);
            continue;
        }
        if (it->second.attempted) ++metrics_.retried;
        const HookVerdict result = hooks_ ? hooks_->apply(command) : HookVerdict::Continue;
        if (result == HookVerdict::Cancel) {
            ++metrics_.rejected_by_hook;
            it = pending_.erase(it);
            continue;
        }
        if (!valid_command(command, it->first, current->sequence)) {
            ++metrics_.invalid_hook_output;
            it = pending_.erase(it);
            continue;
        }
        if (!sink_.submit(command)) {
            it->second.attempted = true;
            ++metrics_.sink_failures;
            ++it;
            continue;
        }
        ++metrics_.accepted;
        it = pending_.erase(it);
    }
}
} // namespace rootcastle::npc
