using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Horror Audio Manager.
    /// Generative audio system that layers ambient tones, rhythmic pulses,
    /// and stingers based on corruption, entity proximity, and player state.
    /// Sound is the game's primary horror vector.
    /// </summary>
    public class HorrorAudioManager : MonoBehaviour
    {
        public static HorrorAudioManager Instance { get; private set; }

        [Header("Audio Sources")]
        [SerializeField] private AudioSource ambientSource;
        [SerializeField] private AudioSource stingerSource;
        [SerializeField] private AudioSource heartbeatSource;
        [SerializeField] private AudioSource environmentSource;
        [SerializeField] private AudioSource entityProximitySource;

        [Header("Ambient Clips")]
        [SerializeField] private AudioClip ambientQuiet;
        [SerializeField] private AudioClip ambientStirring;
        [SerializeField] private AudioClip ambientActive;
        [SerializeField] private AudioClip ambientCritical;
        [SerializeField] private AudioClip ambientCascade;

        [Header("Stingers")]
        [SerializeField] private List<AudioClip> jumpScareStingers;
        [SerializeField] private List<AudioClip> tensionStingers;
        [SerializeField] private AudioClip cascadeAlarm;
        [SerializeField] private AudioClip coreExtractGroan;

        [Header("Entity Cues")]
        [SerializeField] private AudioClip echoAmbient;      // Whisper/static at edge of hearing
        [SerializeField] private AudioClip resonantHum;       // Low frequency hum
        [SerializeField] private AudioClip architectShift;    // Metal groaning, geometry shifting
        [SerializeField] private AudioClip mimicTrigger;      // Piercing signal
        [SerializeField] private AudioClip broadcastRoar;     // Wall of distorted signal

        [Header("Environment")]
        [SerializeField] private AudioClip metalGroan;
        [SerializeField] private AudioClip waterDrip;
        [SerializeField] private AudioClip pressureSeal;
        [SerializeField] private AudioClip doorCreak;
        [SerializeField] private AudioClip footstepsMetal;

        [Header("Heartbeat")]
        [SerializeField] private AudioClip heartbeatNormal;
        [SerializeField] private AudioClip heartbeatFast;
        [SerializeField] private AudioClip heartbeatCritical;

        [Header("Settings")]
        [SerializeField] private float ambientBaseVolume = 0.3f;
        [SerializeField] private float stingerCooldown = 15f;

        // State
        private CorruptionPhase lastPhase = CorruptionPhase.Quiet;
        private float stingerTimer = 0f;
        private float environmentSoundTimer = 0f;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Start()
        {
            GameEvents.OnCorruptionChanged += OnCorruptionChanged;
            GameEvents.OnPhaseTransition += OnPhaseTransition;
            GameEvents.OnEntitySpawned += OnEntitySpawned;
            GameEvents.OnLocalPlayerStateChanged += OnPlayerStateChanged;
            GameEvents.OnCascadeTriggered += OnCascade;
            GameEvents.OnCoreExtracted += OnCoreExtracted;

            // Start with quiet ambient
            PlayAmbient(ambientQuiet);
        }

        private void OnDestroy()
        {
            GameEvents.OnCorruptionChanged -= OnCorruptionChanged;
            GameEvents.OnPhaseTransition -= OnPhaseTransition;
            GameEvents.OnEntitySpawned -= OnEntitySpawned;
            GameEvents.OnLocalPlayerStateChanged -= OnPlayerStateChanged;
            GameEvents.OnCascadeTriggered -= OnCascade;
            GameEvents.OnCoreExtracted -= OnCoreExtracted;
        }

        private void Update()
        {
            stingerTimer += Time.deltaTime;
            TickEnvironmentSounds(Time.deltaTime);
            TickHeartbeat();
            TickEntityProximity();
        }

        // ====================================================================
        // AMBIENT
        // ====================================================================

        private void PlayAmbient(AudioClip clip)
        {
            if (ambientSource == null || clip == null) return;
            if (ambientSource.clip == clip) return;

            ambientSource.clip = clip;
            ambientSource.loop = true;
            ambientSource.volume = ambientBaseVolume;
            ambientSource.Play();
        }

        private void OnCorruptionChanged(float value, CorruptionPhase phase)
        {
            // Modulate ambient pitch based on corruption
            if (ambientSource != null)
            {
                ambientSource.pitch = Mathf.Lerp(1.0f, 0.85f, value / 100f);
                ambientSource.volume = Mathf.Lerp(ambientBaseVolume, ambientBaseVolume * 1.5f, value / 100f);
            }
        }

        private void OnPhaseTransition(CorruptionPhase oldPhase, CorruptionPhase newPhase)
        {
            lastPhase = newPhase;

            // Switch ambient track
            switch (newPhase)
            {
                case CorruptionPhase.Quiet: PlayAmbient(ambientQuiet); break;
                case CorruptionPhase.Stirring: PlayAmbient(ambientStirring); break;
                case CorruptionPhase.Active: PlayAmbient(ambientActive); break;
                case CorruptionPhase.Critical: PlayAmbient(ambientCritical); break;
                case CorruptionPhase.Cascade: PlayAmbient(ambientCascade); break;
            }

            // Play transition stinger
            PlayTensionStinger();
        }

        // ====================================================================
        // STINGERS
        // ====================================================================

        public void PlayTensionStinger()
        {
            if (stingerTimer < stingerCooldown) return;
            stingerTimer = 0f;

            if (stingerSource != null && tensionStingers.Count > 0)
            {
                var clip = tensionStingers[Random.Range(0, tensionStingers.Count)];
                stingerSource.PlayOneShot(clip);
            }
        }

        public void PlayJumpScareStinger()
        {
            if (stingerSource != null && jumpScareStingers.Count > 0)
            {
                var clip = jumpScareStingers[Random.Range(0, jumpScareStingers.Count)];
                stingerSource.PlayOneShot(clip, 0.8f);
            }
        }

        // ====================================================================
        // ENTITY SOUNDS
        // ====================================================================

        private void OnEntitySpawned(EntityType type, Vector3 position)
        {
            // Play distant cue for entity spawn
            AudioClip cue = null;
            switch (type)
            {
                case EntityType.Echo: cue = echoAmbient; break;
                case EntityType.Resonant: cue = resonantHum; break;
                case EntityType.Architect: cue = architectShift; break;
                case EntityType.Broadcast: cue = broadcastRoar; break;
            }

            if (cue != null && environmentSource != null)
            {
                environmentSource.PlayOneShot(cue, 0.4f);
            }
        }

        private void TickEntityProximity()
        {
            if (entityProximitySource == null || GameManager.Instance == null) return;

            // Find nearest entity and play proximity audio
            float nearestDist = float.MaxValue;
            EntityBase nearestEntity = null;

            foreach (var entity in GameManager.Instance.ActiveEntities)
            {
                if (entity == null) continue;
                var player = FindObjectOfType<PlayerCharacter>();
                if (player == null) continue;

                float dist = Vector3.Distance(entity.transform.position, player.transform.position);
                if (dist < nearestDist)
                {
                    nearestDist = dist;
                    nearestEntity = entity;
                }
            }

            if (nearestEntity != null && nearestDist < 25f)
            {
                float volume = Mathf.Lerp(0.8f, 0f, nearestDist / 25f);
                entityProximitySource.volume = volume;

                // Select entity-specific audio
                AudioClip entityClip = null;
                switch (nearestEntity.entityType)
                {
                    case EntityType.Echo: entityClip = echoAmbient; break;
                    case EntityType.Resonant: entityClip = resonantHum; break;
                    case EntityType.Broadcast: entityClip = broadcastRoar; break;
                }

                if (entityClip != null && entityProximitySource.clip != entityClip)
                {
                    entityProximitySource.clip = entityClip;
                    entityProximitySource.loop = true;
                    entityProximitySource.Play();
                }
            }
            else
            {
                entityProximitySource.volume = Mathf.Lerp(entityProximitySource.volume, 0f, Time.deltaTime * 2f);
                if (entityProximitySource.volume < 0.01f)
                    entityProximitySource.Stop();
            }
        }

        // ====================================================================
        // HEARTBEAT (Health indicator)
        // ====================================================================

        private void TickHeartbeat()
        {
            if (heartbeatSource == null) return;

            var player = FindObjectOfType<PlayerCharacter>();
            if (player == null) return;

            float healthPercent = player.Health / player.MaxHealth;

            if (healthPercent > 0.5f)
            {
                heartbeatSource.Stop();
            }
            else if (healthPercent > 0.25f)
            {
                if (!heartbeatSource.isPlaying)
                {
                    heartbeatSource.clip = heartbeatNormal;
                    heartbeatSource.loop = true;
                    heartbeatSource.Play();
                }
                heartbeatSource.volume = Mathf.Lerp(0f, 0.3f, (0.5f - healthPercent) / 0.25f);
            }
            else
            {
                if (heartbeatSource.clip != heartbeatCritical)
                {
                    heartbeatSource.clip = heartbeatCritical;
                    heartbeatSource.Play();
                }
                heartbeatSource.volume = 0.6f;
                heartbeatSource.pitch = Mathf.Lerp(1f, 1.4f, (0.25f - healthPercent) / 0.25f);
            }
        }

        // ====================================================================
        // ENVIRONMENT
        // ====================================================================

        private void TickEnvironmentSounds(float deltaTime)
        {
            environmentSoundTimer += deltaTime;

            float interval = Mathf.Lerp(10f, 3f,
                (CorruptionSystem.Instance?.CorruptionIndex ?? 0) / 100f);

            if (environmentSoundTimer >= interval)
            {
                environmentSoundTimer = 0f;
                PlayRandomEnvironmentSound();
            }
        }

        private void PlayRandomEnvironmentSound()
        {
            if (environmentSource == null) return;

            AudioClip[] envSounds = { metalGroan, waterDrip, pressureSeal, doorCreak };
            var validSounds = new List<AudioClip>();
            foreach (var s in envSounds)
                if (s != null) validSounds.Add(s);

            if (validSounds.Count > 0)
            {
                var clip = validSounds[Random.Range(0, validSounds.Count)];
                environmentSource.PlayOneShot(clip, Random.Range(0.1f, 0.4f));
            }
        }

        // ====================================================================
        // EVENT HANDLERS
        // ====================================================================

        private void OnPlayerStateChanged(PlayerState state)
        {
            if (state == PlayerState.SignalFade)
                PlayJumpScareStinger();
        }

        private void OnCascade(float countdown)
        {
            if (cascadeAlarm != null && stingerSource != null)
            {
                stingerSource.clip = cascadeAlarm;
                stingerSource.loop = true;
                stingerSource.Play();
            }
        }

        private void OnCoreExtracted(SignalCore core)
        {
            if (coreExtractGroan != null && environmentSource != null)
                environmentSource.PlayOneShot(coreExtractGroan, 0.6f);
        }

        /// <summary>Play 3D positioned audio at a world location</summary>
        public void PlaySoundAtPosition(AudioClip clip, Vector3 position, float volume = 1f)
        {
            if (clip == null) return;
            AudioSource.PlayClipAtPoint(clip, position, volume);
        }
    }
}
