using System;
using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    // ========================================================================
    // ENUMS
    // ========================================================================

    public enum CorruptionPhase
    {
        Quiet,      // 0-24%
        Stirring,   // 25-49%
        Active,     // 50-74%
        Critical,   // 75-99%
        Cascade     // 100%
    }

    public enum PlayerRole
    {
        Scanner,
        Technician,
        Warden,
        Courier,
        Operator,
        None
    }

    public enum EntityType
    {
        Echo,
        Resonant,
        Architect,
        Mimic,
        Broadcast
    }

    public enum EquipmentType
    {
        Flashlight,
        UVLantern,
        MotionScanner,
        ResyncKit,
        NoiseMaker,
        SignalJammer,
        GlowStick,
        ReinforcedHarness,
        EmergencyBeacon,
        PulseRadar,
        ExtractionAccelerator,
        SignalFlareGun,
        MagneticHarness,
        RemoteDrone
    }

    public enum MissionType
    {
        StandardSalvage,
        DataRecovery,
        RescueOp,
        PurgeContract,
        BlackBox
    }

    public enum PlayerState
    {
        Alive,
        SignalFade,     // Downed, 60s to revive
        Dead,
        Broadcasting    // Sacrificed to reveal entities
    }

    public enum RoomType
    {
        ProcessingHall,
        ServerCrypt,
        MaintenanceTunnel,
        PressureChamber,
        ObservationDome,
        ControlRoom,
        Junction,
        Airlock,
        Corridor
    }

    public enum ExtractionPhase
    {
        Scanning,
        Disconnecting,
        Pulling
    }

    // ========================================================================
    // DATA CLASSES
    // ========================================================================

    [Serializable]
    public class SignalCore
    {
        public string coreID;
        public int creditValue = 100;
        public float corruptionCost = 10f;
        public float weight = 1f;
        public float integrity = 100f;
        public bool requiresMultiplePlayers = false;

        public SignalCore() { }
        public SignalCore(string id, int value, float corruption, float w)
        {
            coreID = id;
            creditValue = value;
            corruptionCost = corruption;
            weight = w;
        }
    }

    [Serializable]
    public class MissionContract
    {
        public string contractID;
        public MissionType missionType = MissionType.StandardSalvage;
        public int threatRating = 1;
        public float payoutMultiplier = 1f;
        public float baseCorruption = 0f;
        public int minDepth = 1;
        public List<EntityType> guaranteedEntities = new List<EntityType>();
        public string description;
    }

    [Serializable]
    public class RoomModule
    {
        public string roomID;
        public RoomType roomType = RoomType.Corridor;
        public Vector3 roomSize = new Vector3(10f, 4f, 10f);
        public int maxDoorways = 4;
        public List<Vector3> doorwayPositions = new List<Vector3>();
        public List<Vector3> lootSpawnPoints = new List<Vector3>();
        public List<Vector3> entitySpawnPoints = new List<Vector3>();
        public bool hasEmergencyLighting = false;
        public bool isArchitectModifiable = true;
    }

    [Serializable]
    public class EquipmentData
    {
        public EquipmentType type;
        public string displayName;
        public string description;
        public int purchaseCost = 50;
        public int maxCharges = -1;
        public int currentCharges;
        public float durability = 100f;
        public float batteryLife = 100f;
        public float weightPenalty = 0f;
        public int upgradeLevel = 0;
        public int maxUpgradeLevel = 3;

        public EquipmentData() { }
        public EquipmentData(EquipmentType t, string name, int cost, int charges = -1)
        {
            type = t;
            displayName = name;
            purchaseCost = cost;
            maxCharges = charges;
            currentCharges = charges;
        }

        public bool HasCharges => maxCharges < 0 || currentCharges > 0;
        public bool HasBattery => batteryLife > 0f;
    }

    [Serializable]
    public class SubmarineUpgrade
    {
        public string upgradeID;
        public string displayName;
        public int cost = 500;
        public int currentLevel = 0;
        public int maxLevel = 5;
    }

    [Serializable]
    public class RolePassives
    {
        public bool canSeeCorruptionHUD;
        public bool canRepairEquipment;
        public float corruptionDamageMultiplier = 1f;
        public int maxCarryCores = 1;
        public float carrySpeedMultiplier = 1f;

        public static RolePassives ForRole(PlayerRole role)
        {
            var p = new RolePassives();
            switch (role)
            {
                case PlayerRole.Scanner:
                    p.canSeeCorruptionHUD = true;
                    break;
                case PlayerRole.Technician:
                    p.canRepairEquipment = true;
                    break;
                case PlayerRole.Warden:
                    p.corruptionDamageMultiplier = 0.7f;
                    break;
                case PlayerRole.Courier:
                    p.maxCarryCores = 2;
                    p.carrySpeedMultiplier = 1.1f;
                    break;
                case PlayerRole.Operator:
                    // Drone and door control handled separately
                    break;
            }
            return p;
        }
    }

    // ========================================================================
    // EVENTS
    // ========================================================================

    public static class GameEvents
    {
        public static event Action<float, CorruptionPhase> OnCorruptionChanged;
        public static event Action<CorruptionPhase, CorruptionPhase> OnPhaseTransition;
        public static event Action<float> OnCascadeTriggered;
        public static event Action<EntityType, Vector3> OnEntitySpawned;
        public static event Action<PlayerState> OnLocalPlayerStateChanged;
        public static event Action<SignalCore> OnCoreExtracted;
        public static event Action<string> OnDynamicEvent;
        public static event Action<float> OnPerceptionDriftChanged;
        public static event Action OnMissionComplete;
        public static event Action OnMissionFailed;

        public static void RaiseCorruptionChanged(float value, CorruptionPhase phase)
            => OnCorruptionChanged?.Invoke(value, phase);
        public static void RaisePhaseTransition(CorruptionPhase from, CorruptionPhase to)
            => OnPhaseTransition?.Invoke(from, to);
        public static void RaiseCascade(float countdown)
            => OnCascadeTriggered?.Invoke(countdown);
        public static void RaiseEntitySpawned(EntityType type, Vector3 pos)
            => OnEntitySpawned?.Invoke(type, pos);
        public static void RaisePlayerStateChanged(PlayerState state)
            => OnLocalPlayerStateChanged?.Invoke(state);
        public static void RaiseCoreExtracted(SignalCore core)
            => OnCoreExtracted?.Invoke(core);
        public static void RaiseDynamicEvent(string eventName)
            => OnDynamicEvent?.Invoke(eventName);
        public static void RaisePerceptionDrift(float drift)
            => OnPerceptionDriftChanged?.Invoke(drift);
        public static void RaiseMissionComplete() => OnMissionComplete?.Invoke();
        public static void RaiseMissionFailed() => OnMissionFailed?.Invoke();
    }
}
