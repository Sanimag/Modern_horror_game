using UnityEngine;
using UnityEngine.AI;

namespace SignalLost
{
    /// <summary>
    /// Base class for all Signal Lost entities.
    /// Entities are manifestations of corrupted signal data —
    /// patterns that have learned to interact with physical space.
    /// </summary>
    [RequireComponent(typeof(NavMeshAgent))]
    public abstract class EntityBase : MonoBehaviour
    {
        [Header("Entity Properties")]
        public EntityType entityType;
        public float detectionRange = 20f;
        public float attackDamage = 30f;
        public float movementSpeed = 3f;
        public float perceptionDriftInflicted = 15f;
        public bool canBeStunned = true;

        [Header("State")]
        [SerializeField] protected PlayerCharacter currentTarget;
        [SerializeField] protected bool isStunned = false;
        protected float stunTimer = 0f;

        protected NavMeshAgent agent;

        public PlayerCharacter CurrentTarget => currentTarget;
        public bool IsStunned => isStunned;

        protected virtual void Awake()
        {
            agent = GetComponent<NavMeshAgent>();
            agent.speed = movementSpeed;
        }

        protected virtual void Update()
        {
            if (isStunned)
            {
                stunTimer -= Time.deltaTime;
                if (stunTimer <= 0f) OnStunEnd();
                return;
            }

            UpdateBehavior(Time.deltaTime);
        }

        protected abstract void UpdateBehavior(float deltaTime);

        // ====================================================================
        // COMBAT
        // ====================================================================

        public virtual void OnPlayerDetected(PlayerCharacter player)
        {
            currentTarget = player;
            player.AddPerceptionDrift(perceptionDriftInflicted);
        }

        public virtual void AttackPlayer(PlayerCharacter player)
        {
            if (player == null || player.CurrentState != PlayerState.Alive) return;
            player.TakeDamage(attackDamage, entityType);
        }

        public virtual void Stun(float duration)
        {
            if (!canBeStunned) return;
            isStunned = true;
            stunTimer = duration;
            agent.isStopped = true;
        }

        protected virtual void OnStunEnd()
        {
            isStunned = false;
            stunTimer = 0f;
            agent.isStopped = false;
        }

        public virtual void OnCorruptionPhaseChanged(CorruptionPhase newPhase)
        {
            if (newPhase >= CorruptionPhase.Critical)
            {
                movementSpeed *= 1.3f;
                detectionRange *= 1.5f;
                agent.speed = movementSpeed;
            }
        }

        // ====================================================================
        // TARGETING
        // ====================================================================

        protected virtual PlayerCharacter FindTarget()
        {
            PlayerCharacter best = null;
            float bestScore = 0f;

            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                float dist = Vector3.Distance(transform.position, player.transform.position);
                if (dist > detectionRange) continue;

                float score = (detectionRange - dist) / detectionRange;
                if (player.IsIsolated()) score *= 2f;
                score += player.GetNoiseLevel() * 0.5f;

                if (score > bestScore)
                {
                    bestScore = score;
                    best = player;
                }
            }
            return best;
        }

        protected float DistanceToTarget()
        {
            if (currentTarget == null) return float.MaxValue;
            return Vector3.Distance(transform.position, currentTarget.transform.position);
        }

        protected bool IsPlayerLookingAtMe(PlayerCharacter player)
        {
            if (player == null || player.CameraTransform == null) return false;

            Vector3 toEntity = (transform.position - player.transform.position).normalized;
            float dot = Vector3.Dot(player.CameraTransform.forward, toEntity);
            float dist = Vector3.Distance(transform.position, player.transform.position);

            return dot > 0.5f && dist < 20f;
        }
    }
}
