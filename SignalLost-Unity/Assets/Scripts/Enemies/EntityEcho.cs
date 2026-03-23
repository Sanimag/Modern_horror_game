using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// THE ECHO (Stalker Type)
    /// A humanoid silhouette that mimics crew member outlines.
    /// Always at the edge of visibility. Freezes when directly observed.
    /// Waits until target is isolated, then lunges from blind spot.
    /// Counter: Stay in groups. Check behind you. Scanner shows flickering blip.
    /// </summary>
    public class EntityEcho : EntityBase
    {
        private enum EchoState { Idle, Stalking, Approaching, Lunging }

        [Header("Echo Settings")]
        [SerializeField] private float idealStalkDistance = 15f;
        [SerializeField] private float attackDistance = 2f;
        [SerializeField] private float minStalkDuration = 30f;
        [SerializeField] private float lungeSpeed = 15f;

        [SerializeField] private EchoState echoState = EchoState.Idle;
        [SerializeField] private bool isBeingObserved = false;
        [SerializeField] private float stalkTimer = 0f;

        protected override void Awake()
        {
            base.Awake();
            entityType = EntityType.Echo;
            attackDamage = 100f; // Instant down
            movementSpeed = 12f; // Fast when lunging
            detectionRange = 30f;
            perceptionDriftInflicted = 25f;
        }

        protected override void UpdateBehavior(float deltaTime)
        {
            switch (echoState)
            {
                case EchoState.Idle:
                    currentTarget = FindTarget();
                    if (currentTarget != null)
                    {
                        echoState = EchoState.Stalking;
                        stalkTimer = 0f;
                    }
                    break;

                case EchoState.Stalking:
                    if (currentTarget == null || currentTarget.CurrentState != PlayerState.Alive)
                    {
                        echoState = EchoState.Idle;
                        currentTarget = null;
                        break;
                    }

                    stalkTimer += deltaTime;

                    // Check if any player is looking at us
                    isBeingObserved = false;
                    foreach (var player in GameManager.Instance.GetAlivePlayers())
                    {
                        if (IsPlayerLookingAtMe(player))
                        {
                            isBeingObserved = true;
                            break;
                        }
                    }

                    // Only move when not observed — the classic "SCP-173" mechanic
                    if (!isBeingObserved)
                    {
                        MoveCloser(deltaTime);
                    }
                    else
                    {
                        agent.isStopped = true;
                    }

                    // Attack when conditions met: stalked long enough + target isolated
                    if (stalkTimer >= minStalkDuration && currentTarget.IsIsolated())
                    {
                        float dist = DistanceToTarget();
                        if (dist < attackDistance * 3f && !isBeingObserved)
                        {
                            echoState = EchoState.Lunging;
                            agent.speed = lungeSpeed;
                            agent.isStopped = false;
                        }
                    }
                    break;

                case EchoState.Lunging:
                    if (currentTarget == null) { ResetToIdle(); break; }

                    agent.SetDestination(currentTarget.transform.position);

                    if (DistanceToTarget() < attackDistance)
                    {
                        Debug.LogWarning("[Echo] LUNGE ATTACK!");
                        AttackPlayer(currentTarget);
                        ResetToIdle();
                    }
                    break;
            }
        }

        private void MoveCloser(float deltaTime)
        {
            if (currentTarget == null) return;

            float distance = DistanceToTarget();
            float desiredDist = Mathf.Lerp(idealStalkDistance, attackDistance * 2f,
                Mathf.Clamp01(stalkTimer / minStalkDuration));

            if (distance > desiredDist)
            {
                // Teleport-like movement in short bursts when not observed
                Vector3 direction = (currentTarget.transform.position - transform.position).normalized;
                float moveAmount = Mathf.Min(movementSpeed * deltaTime, distance - desiredDist);
                transform.position += direction * moveAmount;
            }

            // Always face toward the target
            Vector3 lookDir = currentTarget.transform.position - transform.position;
            lookDir.y = 0;
            if (lookDir != Vector3.zero)
                transform.rotation = Quaternion.LookRotation(lookDir);
        }

        private void ResetToIdle()
        {
            echoState = EchoState.Idle;
            currentTarget = null;
            stalkTimer = 0f;
            agent.speed = movementSpeed;
            agent.isStopped = true;
        }
    }
}
