using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// THE BROADCAST (Boss Entity)
    /// Massive entity of cascading signal waves and fractured light.
    /// Fills entire corridors. Looking at it causes screen distortion.
    /// Spawns only during Cascade (100% Corruption).
    /// Moves slowly but inevitably toward the airlock.
    /// Contact is instantly lethal. Jams all equipment within 30m.
    /// You don't fight it. You outrun it.
    /// </summary>
    public class EntityBroadcast : EntityBase
    {
        [Header("Broadcast Settings")]
        [SerializeField] private float advanceSpeed = 3f;
        [SerializeField] private float jamRadius = 30f;
        [SerializeField] private float distortionRadius = 50f;
        [SerializeField] private float killRadius = 5f;

        private Vector3 airlockTarget;

        protected override void Awake()
        {
            base.Awake();
            entityType = EntityType.Broadcast;
            attackDamage = 9999f;
            movementSpeed = advanceSpeed;
            canBeStunned = false;
            perceptionDriftInflicted = 50f;
        }

        private void Start()
        {
            if (GameManager.Instance != null)
                airlockTarget = GameManager.Instance.AirlockPosition;
        }

        protected override void UpdateBehavior(float deltaTime)
        {
            AdvanceTowardAirlock(deltaTime);
            KillPlayersInPath();
            ApplyVisualDistortion();
        }

        private void AdvanceTowardAirlock(float deltaTime)
        {
            Vector3 direction = (airlockTarget - transform.position).normalized;
            transform.position += direction * advanceSpeed * deltaTime;

            // Face direction of travel
            if (direction != Vector3.zero)
                transform.rotation = Quaternion.LookRotation(direction);
        }

        private void KillPlayersInPath()
        {
            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                float dist = Vector3.Distance(transform.position, player.transform.position);
                if (dist < killRadius)
                {
                    Debug.LogError("[Broadcast] Consumed a player!");
                    AttackPlayer(player);
                }
            }
        }

        /// <summary>Check if a world position is within the equipment jam zone</summary>
        public bool IsLocationJammed(Vector3 location)
        {
            return Vector3.Distance(transform.position, location) < jamRadius;
        }

        /// <summary>Get visual distortion intensity for a player based on look angle and distance</summary>
        public float GetDistortionIntensity(PlayerCharacter player)
        {
            if (player == null) return 0f;

            float dist = Vector3.Distance(transform.position, player.transform.position);
            if (dist > distortionRadius) return 0f;

            if (!IsPlayerLookingAtMe(player)) return 0f;

            float distFactor = 1f - (dist / distortionRadius);
            Vector3 toBroadcast = (transform.position - player.transform.position).normalized;
            float lookDot = Vector3.Dot(player.CameraTransform.forward, toBroadcast);
            float lookFactor = Mathf.Clamp01((lookDot - 0.3f) / 0.7f);

            return distFactor * lookFactor;
        }

        private void ApplyVisualDistortion()
        {
            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                float intensity = GetDistortionIntensity(player);
                if (intensity > 0.1f)
                {
                    SignalLostHUD.Instance?.ApplyBroadcastDistortion(intensity);
                }
            }
        }
    }
}
