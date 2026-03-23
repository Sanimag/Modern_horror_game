using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// THE RESONANT (Sound-Based Hunter)
    /// Floating mass of translucent geometric shards that hum at low frequency.
    /// Blind but has perfect sound tracking. Any noise attracts it.
    /// Contact causes Frequency Burn (screen disruption + damage over time).
    /// Counter: Crouch-walk (silent), noise makers, Operator drone distraction.
    /// </summary>
    public class EntityResonant : EntityBase
    {
        private enum ResonantState { Patrolling, Investigating, Chasing }

        [Header("Resonant Settings")]
        [SerializeField] private float soundDetectionRange = 30f;
        [SerializeField] private float noiseThreshold = 0.2f;
        [SerializeField] private float chaseSpeed = 10f;
        [SerializeField] private float patrolSpeed = 1.5f;
        [SerializeField] private float frequencyBurnDPS = 15f;
        [SerializeField] private float soundMemoryDuration = 8f;
        [SerializeField] private float contactRange = 2f;

        [SerializeField] private ResonantState resonantState = ResonantState.Patrolling;
        [SerializeField] private Vector3 lastHeardLocation;
        private float soundMemoryTimer = 0f;

        [Header("Patrol")]
        [SerializeField] private List<Transform> patrolPoints = new List<Transform>();
        private int currentPatrolIndex = 0;

        protected override void Awake()
        {
            base.Awake();
            entityType = EntityType.Resonant;
            attackDamage = 0f; // Uses frequency burn DPS
            movementSpeed = patrolSpeed;
            detectionRange = soundDetectionRange;
            perceptionDriftInflicted = 10f;
            canBeStunned = true;
        }

        protected override void UpdateBehavior(float deltaTime)
        {
            ScanForSounds();

            switch (resonantState)
            {
                case ResonantState.Patrolling:
                    Patrol(deltaTime);
                    break;

                case ResonantState.Investigating:
                case ResonantState.Chasing:
                    ChaseSound(deltaTime);
                    soundMemoryTimer -= deltaTime;
                    if (soundMemoryTimer <= 0f)
                    {
                        resonantState = ResonantState.Patrolling;
                        agent.speed = patrolSpeed;
                        currentTarget = null;
                    }
                    break;
            }

            // Frequency Burn: damage players on contact
            ApplyFrequencyBurn(deltaTime);
        }

        private void ScanForSounds()
        {
            float loudestNoise = 0f;
            PlayerCharacter loudestPlayer = null;

            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                float dist = Vector3.Distance(transform.position, player.transform.position);
                if (dist > soundDetectionRange) continue;

                float noise = player.GetNoiseLevel();
                float attenuated = noise * (1f - dist / soundDetectionRange);

                if (attenuated > noiseThreshold && attenuated > loudestNoise)
                {
                    loudestNoise = attenuated;
                    loudestPlayer = player;
                }
            }

            if (loudestPlayer != null)
            {
                lastHeardLocation = loudestPlayer.transform.position;
                soundMemoryTimer = soundMemoryDuration;
                currentTarget = loudestPlayer;

                if (loudestNoise > 0.6f)
                {
                    resonantState = ResonantState.Chasing;
                    agent.speed = chaseSpeed;
                }
                else
                {
                    resonantState = ResonantState.Investigating;
                    agent.speed = chaseSpeed * 0.5f;
                }
            }
        }

        private void Patrol(float deltaTime)
        {
            agent.speed = patrolSpeed;

            if (patrolPoints.Count == 0)
            {
                // Wander randomly
                if (!agent.hasPath || agent.remainingDistance < 1f)
                {
                    Vector3 randomPoint = transform.position + Random.insideUnitSphere * 15f;
                    randomPoint.y = transform.position.y;
                    agent.SetDestination(randomPoint);
                }
                return;
            }

            agent.SetDestination(patrolPoints[currentPatrolIndex].position);
            if (agent.remainingDistance < 1f)
            {
                currentPatrolIndex = (currentPatrolIndex + 1) % patrolPoints.Count;
            }
        }

        private void ChaseSound(float deltaTime)
        {
            agent.SetDestination(lastHeardLocation);

            if (Vector3.Distance(transform.position, lastHeardLocation) < 2f)
            {
                if (resonantState == ResonantState.Chasing)
                    resonantState = ResonantState.Investigating;
            }
        }

        private void ApplyFrequencyBurn(float deltaTime)
        {
            foreach (var player in GameManager.Instance.GetAlivePlayers())
            {
                float dist = Vector3.Distance(transform.position, player.transform.position);
                if (dist < contactRange)
                {
                    player.TakeDamage(frequencyBurnDPS * deltaTime, entityType);
                    // Trigger screen distortion effect on player HUD
                    SignalLostHUD.Instance?.ApplyFrequencyBurn(0.8f, 1f);
                }
            }
        }

        // Override: Resonant targets by sound, not sight
        protected override PlayerCharacter FindTarget() => null;
    }
}
