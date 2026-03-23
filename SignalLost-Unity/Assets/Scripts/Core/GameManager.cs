using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Central game manager. Coordinates mission lifecycle, player tracking,
    /// entity management, and all core game systems.
    /// </summary>
    public class GameManager : MonoBehaviour
    {
        public static GameManager Instance { get; private set; }

        [Header("Mission State")]
        [SerializeField] private bool missionActive = false;
        [SerializeField] private MissionContract currentContract;

        [Header("References")]
        [SerializeField] private Transform airlockTransform;

        [Header("Entity Spawning")]
        [SerializeField] private float entitySpawnTimer = 0f;
        [SerializeField] private float baseEntitySpawnInterval = 30f;
        [SerializeField] private List<EntityBase> activeEntities = new List<EntityBase>();

        [Header("Entity Prefabs")]
        public GameObject echoPrefab;
        public GameObject resonantPrefab;
        public GameObject architectPrefab;
        public GameObject mimicPrefab;
        public GameObject broadcastPrefab;

        // Properties
        public bool IsMissionActive => missionActive;
        public MissionContract CurrentContract => currentContract;
        public Vector3 AirlockPosition => airlockTransform ? airlockTransform.position : Vector3.zero;
        public List<EntityBase> ActiveEntities => activeEntities;

        // Player tracking
        private List<PlayerCharacter> registeredPlayers = new List<PlayerCharacter>();

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Update()
        {
            if (!missionActive) return;

            TickEntitySpawning(Time.deltaTime);
            CleanupDeadEntities();
        }

        // ====================================================================
        // PLAYER REGISTRATION
        // ====================================================================

        public void RegisterPlayer(PlayerCharacter player)
        {
            if (!registeredPlayers.Contains(player))
                registeredPlayers.Add(player);
        }

        public void UnregisterPlayer(PlayerCharacter player)
        {
            registeredPlayers.Remove(player);
        }

        public List<PlayerCharacter> GetAlivePlayers()
        {
            return registeredPlayers.Where(p => p != null && p.CurrentState == PlayerState.Alive).ToList();
        }

        public int GetPlayerCount() => registeredPlayers.Count;

        public bool AreAllPlayersAtAirlock()
        {
            return GetAlivePlayers().All(p => p.IsAtAirlock());
        }

        public bool AreAllPlayersAccountedFor()
        {
            return registeredPlayers.All(p =>
                p.IsAtAirlock() || p.CurrentState == PlayerState.Dead);
        }

        public PlayerCharacter GetNearestPlayerTo(Vector3 position, float maxRange = float.MaxValue)
        {
            PlayerCharacter nearest = null;
            float nearestDist = maxRange;

            foreach (var player in GetAlivePlayers())
            {
                float dist = Vector3.Distance(position, player.transform.position);
                if (dist < nearestDist)
                {
                    nearestDist = dist;
                    nearest = player;
                }
            }
            return nearest;
        }

        // ====================================================================
        // MISSION LIFECYCLE
        // ====================================================================

        public void StartMission(MissionContract contract)
        {
            currentContract = contract;
            missionActive = true;
            entitySpawnTimer = 0f;
            activeEntities.Clear();

            CorruptionSystem.Instance?.SetCorruption(contract.baseCorruption);

            Debug.Log($"[Mission] Started: Threat {contract.threatRating}, Payout {contract.payoutMultiplier}x");
        }

        public void CompleteMission()
        {
            missionActive = false;
            GameEvents.RaiseMissionComplete();
            Debug.Log($"[Mission] Complete! Final corruption: {CorruptionSystem.Instance?.CorruptionIndex:F1}%");
        }

        public void FailMission()
        {
            missionActive = false;
            GameEvents.RaiseMissionFailed();
            Debug.LogError("[Mission] FAILED!");
        }

        public void InitiateExtraction()
        {
            if (!AreAllPlayersAccountedFor())
            {
                Debug.LogWarning("[Mission] Cannot extract - players unaccounted for");
                return;
            }
            CompleteMission();
        }

        // ====================================================================
        // ENTITY SPAWNING
        // ====================================================================

        private void TickEntitySpawning(float deltaTime)
        {
            var corruption = CorruptionSystem.Instance;
            if (corruption == null || corruption.CurrentPhase < CorruptionPhase.Active) return;

            entitySpawnTimer += deltaTime;

            float interval = baseEntitySpawnInterval * corruption.GetEntitySpawnIntervalMultiplier();
            if (entitySpawnTimer < interval) return;
            entitySpawnTimer = 0f;

            if (activeEntities.Count >= corruption.GetMaxEntities()) return;

            // Select entity type based on corruption
            var validTypes = new List<EntityType> { EntityType.Echo, EntityType.Resonant };

            if (corruption.CorruptionIndex >= 50f)
            {
                validTypes.Add(EntityType.Architect);
                validTypes.Add(EntityType.Mimic);
            }
            if (corruption.IsCascadeActive)
            {
                validTypes.Add(EntityType.Broadcast);
            }

            EntityType selectedType = validTypes[Random.Range(0, validTypes.Count)];
            Vector3 spawnPos = GetEntitySpawnPoint();

            SpawnEntity(selectedType, spawnPos);
        }

        public EntityBase SpawnEntity(EntityType type, Vector3 position)
        {
            GameObject prefab = GetEntityPrefab(type);
            if (prefab == null)
            {
                Debug.LogWarning($"[Entity] No prefab for {type}");
                return null;
            }

            GameObject go = Instantiate(prefab, position, Quaternion.identity);
            EntityBase entity = go.GetComponent<EntityBase>();
            if (entity != null)
            {
                activeEntities.Add(entity);
                GameEvents.RaiseEntitySpawned(type, position);
                Debug.Log($"[Entity] Spawned {type} at {position}");
            }
            return entity;
        }

        private GameObject GetEntityPrefab(EntityType type)
        {
            switch (type)
            {
                case EntityType.Echo: return echoPrefab;
                case EntityType.Resonant: return resonantPrefab;
                case EntityType.Architect: return architectPrefab;
                case EntityType.Mimic: return mimicPrefab;
                case EntityType.Broadcast: return broadcastPrefab;
                default: return null;
            }
        }

        private Vector3 GetEntitySpawnPoint()
        {
            var generator = StationGenerator.Instance;
            if (generator != null)
            {
                var points = generator.GetEntitySpawnPoints();
                if (points.Count > 0)
                    return points[Random.Range(0, points.Count)];
            }
            // Fallback: random position away from players
            return Vector3.zero + Random.insideUnitSphere * 30f;
        }

        private void CleanupDeadEntities()
        {
            activeEntities.RemoveAll(e => e == null);
        }

        /// <summary>Reveal all entity positions (used by Broadcast Sacrifice)</summary>
        public List<Vector3> RevealAllEntityPositions()
        {
            return activeEntities
                .Where(e => e != null)
                .Select(e => e.transform.position)
                .ToList();
        }
    }
}
