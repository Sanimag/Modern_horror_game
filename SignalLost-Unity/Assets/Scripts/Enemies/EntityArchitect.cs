using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// THE ARCHITECT (Environment Manipulator)
    /// Never seen directly. Rearranges the station around players.
    /// Hallways loop, rooms rotate, doors open onto walls.
    /// Activates above 50% Corruption. Targets players heading to airlock.
    /// Counter: Scanner detects modified rooms. Physical markers track changes.
    /// </summary>
    public class EntityArchitect : EntityBase
    {
        [Header("Architect Settings")]
        [SerializeField] private float modificationCooldown = 20f;
        [SerializeField] private float modificationRadius = 30f;
        [SerializeField] private int maxModifiedRooms = 5;

        [SerializeField] private float modificationTimer = 0f;
        [SerializeField] private List<string> modifiedRoomIDs = new List<string>();

        protected override void Awake()
        {
            base.Awake();
            entityType = EntityType.Architect;
            attackDamage = 0f;
            movementSpeed = 0f;
            canBeStunned = false;
            perceptionDriftInflicted = 20f;

            // Architect doesn't use NavMesh — it doesn't physically exist
            if (agent != null) agent.enabled = false;
        }

        protected override void UpdateBehavior(float deltaTime)
        {
            modificationTimer += deltaTime;
            if (modificationTimer < modificationCooldown) return;

            var target = FindAirlockBoundPlayer();
            if (target == null) return;

            modificationTimer = 0f;
            PerformModification(target);

            // Increase drift for players near modified areas
            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                player.AddPerceptionDrift(5f);
            }
        }

        private void PerformModification(PlayerCharacter target)
        {
            if (modifiedRoomIDs.Count >= maxModifiedRooms) return;

            var generator = StationGenerator.Instance;
            if (generator == null) return;

            int modType = Random.Range(0, 3);
            switch (modType)
            {
                case 0: // Create spatial loop
                    generator.CreateSpatialLoop(GetNearestRoomID(target.transform.position));
                    Debug.LogWarning("[Architect] Created spatial loop near player");
                    break;

                case 1: // Extend path to airlock
                    generator.ExtendAirlockPath();
                    Debug.LogWarning("[Architect] Extended path to airlock");
                    break;

                case 2: // Seal a doorway near the player
                    string roomID = GetNearestRoomID(target.transform.position);
                    generator.SealDoorway(roomID, 0);
                    modifiedRoomIDs.Add(roomID);
                    Debug.LogWarning($"[Architect] Sealed doorway in room {roomID}");
                    break;
            }
        }

        private PlayerCharacter FindAirlockBoundPlayer()
        {
            if (GameManager.Instance == null) return null;

            PlayerCharacter best = null;
            float bestScore = 0f;
            Vector3 airlockPos = GameManager.Instance.AirlockPosition;

            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                Vector3 toAirlock = (airlockPos - player.transform.position).normalized;
                Vector3 velocity = player.GetComponent<CharacterController>()?.velocity.normalized ?? Vector3.zero;

                float headingScore = Vector3.Dot(toAirlock, velocity);
                if (headingScore > bestScore)
                {
                    bestScore = headingScore;
                    best = player;
                }
            }
            return best;
        }

        private string GetNearestRoomID(Vector3 position)
        {
            var generator = StationGenerator.Instance;
            if (generator == null) return "Unknown";
            return generator.GetNearestRoomID(position);
        }

        public bool IsRoomModified(string roomID) => modifiedRoomIDs.Contains(roomID);
    }
}
