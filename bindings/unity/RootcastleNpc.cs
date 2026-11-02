// Rootcastle NPC AI - Unity C# P/Invoke declarations for the C ABI.
// Copyright (c) 2026 Batuhan Ayribas. MIT License.
// Tested header consistency by inspection; compile against your Unity target architecture.
using System;
using System.Runtime.InteropServices;

public static class RootcastleNpc
{
    private const string Library = "rootcastle_npc";

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3 { public float x, y, z; }

    [StructLayout(LayoutKind.Sequential)]
    public struct Pose { public Vec3 position, forward; public float health; }

    [StructLayout(LayoutKind.Sequential)]
    public struct Config
    {
        public ulong id;
        public uint faction, seed;
        public float courage, aggression, curiosity, empathy, discipline;
        public float sight_range, hearing_range, field_of_view_degrees;
        public float memory_seconds, decision_interval_seconds, switch_margin;
        public float wander_radius, flee_distance;
        public uint max_memories, max_stimuli;
        public uint can_fight, can_flee, can_investigate, can_assist, can_wander;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Stimulus
    {
        public ulong event_id;
        public uint kind;
        public ulong source;
        public Vec3 position;
        public float strength;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Decision
    {
        public ulong actor;
        public uint intent, reason;
        public ulong target;
        public Vec3 destination;
        public float utility;
        public ulong sequence;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Metrics
    {
        public ulong ticks, received, filtered, dropped, memory_evictions;
        public ulong decisions, agents_evaluated;
    }

    public enum Intent : uint
    {
        Idle=0, Wander=1, Patrol=2, Investigate=3,
        Flee=4, Engage=5, Assist=6
    }

    public enum StimulusKind : uint
    {
        Sound=1, HostileSight=2, NeutralSight=3, Damage=4,
        HelpRequest=5, Alarm=6, ClearThreat=7, FriendlySight=8
    }

    [DllImport(Library, EntryPoint="rc_abi_version", CallingConvention=CallingConvention.Cdecl)]
    public static extern uint AbiVersion();

    [DllImport(Library, EntryPoint="rc_default_agent_config", CallingConvention=CallingConvention.Cdecl)]
    public static extern Config DefaultConfig(ulong actorId);

    [DllImport(Library, EntryPoint="rc_default_pose", CallingConvention=CallingConvention.Cdecl)]
    public static extern Pose DefaultPose();

    [DllImport(Library, EntryPoint="rc_world_create", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Create(out IntPtr world);

    [DllImport(Library, EntryPoint="rc_world_destroy", CallingConvention=CallingConvention.Cdecl)]
    public static extern void Destroy(IntPtr world);

    [DllImport(Library, EntryPoint="rc_world_add", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Add(IntPtr world, Config config, Pose pose);

    [DllImport(Library, EntryPoint="rc_world_remove", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Remove(IntPtr world, ulong actorId);

    [DllImport(Library, EntryPoint="rc_world_update_pose", CallingConvention=CallingConvention.Cdecl)]
    public static extern int UpdatePose(IntPtr world, ulong actorId, Pose pose);

    [DllImport(Library, EntryPoint="rc_world_arrived", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Arrived(IntPtr world, ulong actorId);

    [DllImport(Library, EntryPoint="rc_world_submit", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Submit(IntPtr world, ulong actorId, Stimulus stimulus);

    [DllImport(Library, EntryPoint="rc_world_broadcast", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Broadcast(IntPtr world, Stimulus stimulus, float radius);

    [DllImport(Library, EntryPoint="rc_world_set_relation", CallingConvention=CallingConvention.Cdecl)]
    public static extern int SetRelation(IntPtr world, uint from, uint to, uint relation);

    [DllImport(Library, EntryPoint="rc_world_observe", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Observe(IntPtr world, ulong observer, ulong target,
                                     uint lineOfSight, ulong eventId);

    [DllImport(Library, EntryPoint="rc_world_tick", CallingConvention=CallingConvention.Cdecl)]
    public static extern int Tick(IntPtr world, float deltaSeconds);

    [DllImport(Library, EntryPoint="rc_world_get_decision", CallingConvention=CallingConvention.Cdecl)]
    public static extern int GetDecision(IntPtr world, ulong actorId, out Decision decision);

    [DllImport(Library, EntryPoint="rc_world_change_count", CallingConvention=CallingConvention.Cdecl)]
    public static extern UIntPtr ChangeCount(IntPtr world);

    [DllImport(Library, EntryPoint="rc_world_get_change", CallingConvention=CallingConvention.Cdecl)]
    public static extern int GetChange(IntPtr world, UIntPtr index, out Decision decision);

    [DllImport(Library, EntryPoint="rc_world_get_metrics", CallingConvention=CallingConvention.Cdecl)]
    public static extern int GetMetrics(IntPtr world, out Metrics metrics);
}
