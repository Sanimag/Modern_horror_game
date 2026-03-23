using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// THE MIMIC (Deceptive Trap)
    /// Identical to a loot object — signal core, supply cache, or equipment.
    /// Indistinguishable until interacted with.
    /// Latches onto player, deals damage, emits signal attracting entities.
    /// If player is alone, drags them into the floor over 10 seconds.
    /// Counter: Scanner focused scan (3 sec). Faint vibration visual tell.
    /// </summary>
    public class EntityMimic : EntityBase
    {
        [Header("Mimic Settings")]
        [SerializeField] private EquipmentType disguiseType;
        [SerializeField] private float latchDPS = 10f;
        [SerializeField] private float dragDuration = 10f;
        [SerializeField] private float signalAttractRadius = 50f;

        [Header("State")]
        [SerializeField] private bool isDisguised = true;
        [SerializeField] private bool isLatched = false;
        [SerializeField] private bool isDragging = false;
        [SerializeField] private PlayerCharacter latchedPlayer;
        private float dragTimer = 0f;

        // Properties
        public bool IsDisguised => isDisguised;
        public bool IsLatched => isLatched;

        /// <summary>Subtle vibration offset — slightly different from real items</summary>
        public float DisguiseVibrationOffset => 0.02f;

        protected override void Awake()
        {
            base.Awake();
            entityType = EntityType.Mimic;
            attackDamage = 0f;
            movementSpeed = 0f;
            canBeStunned = false;
            perceptionDriftInflicted = 30f;

            if (agent != null) agent.enabled = false;
        }

        protected override void UpdateBehavior(float deltaTime)
        {
            if (isDisguised) return;

            if (isLatched && latchedPlayer != null)
            {
                // Deal DPS while latched
                latchedPlayer.TakeDamage(latchDPS * deltaTime, entityType);

                // Follow latched player
                transform.position = latchedPlayer.transform.position + Vector3.up * 0.5f;

                // If player is alone, start dragging underground
                if (latchedPlayer.IsIsolated() && !isDragging)
                {
                    isDragging = true;
                    dragTimer = dragDuration;
                    Debug.LogWarning("[Mimic] Dragging isolated player underground!");
                }

                if (isDragging)
                {
                    DragPlayerDown(deltaTime);
                }
            }
        }

        public void OnPlayerInteract(PlayerCharacter player)
        {
            if (!isDisguised || player == null) return;

            isDisguised = false;
            LatchOntoPlayer(player);
            EmitAttractSignal();

            Debug.LogWarning("[Mimic] TRIGGERED! Latched onto player!");
        }

        private void LatchOntoPlayer(PlayerCharacter player)
        {
            isLatched = true;
            latchedPlayer = player;
            player.DropAllCores();
            player.AddPerceptionDrift(perceptionDriftInflicted);

            // Attach visually to player
            transform.SetParent(player.transform);
            transform.localPosition = Vector3.up * 0.5f;
        }

        public void DetachFromPlayer(PlayerCharacter rescuer)
        {
            if (!isLatched) return;

            isLatched = false;
            isDragging = false;
            latchedPlayer = null;
            dragTimer = 0f;
            transform.SetParent(null);

            Debug.Log("[Mimic] Detached by teammate rescue");
            Destroy(gameObject);
        }

        private void DragPlayerDown(float deltaTime)
        {
            dragTimer -= deltaTime;

            if (latchedPlayer != null)
            {
                // Slowly sink the player
                Vector3 pos = latchedPlayer.transform.position;
                pos.y -= 0.3f * deltaTime;
                latchedPlayer.transform.position = pos;
            }

            if (dragTimer <= 0f && latchedPlayer != null)
            {
                latchedPlayer.EnterSignalFade();
                isLatched = false;
                latchedPlayer = null;
                Destroy(gameObject);
                Debug.LogError("[Mimic] Dragged player underground!");
            }
        }

        private void EmitAttractSignal()
        {
            // Alert all entities within radius
            if (GameManager.Instance == null) return;

            foreach (var entity in GameManager.Instance.ActiveEntities)
            {
                if (entity == null || entity == this) continue;
                float dist = Vector3.Distance(transform.position, entity.transform.position);
                if (dist < signalAttractRadius)
                {
                    entity.OnPlayerDetected(latchedPlayer);
                }
            }

            Debug.LogWarning($"[Mimic] Attract signal emitted (radius: {signalAttractRadius}m)");
        }
    }
}
