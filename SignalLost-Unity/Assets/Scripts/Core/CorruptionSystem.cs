using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Central corruption system. Manages the Corruption Index (0-100%),
    /// phase transitions, and all corruption-driven effects:
    /// - Light failures at 25%+
    /// - Entity spawning at 50%+
    /// - Layout shifting at 75%+
    /// - Cascade at 100% (90-second countdown)
    /// </summary>
    public class CorruptionSystem : MonoBehaviour
    {
        public static CorruptionSystem Instance { get; private set; }

        [Header("Corruption State")]
        [SerializeField] private float corruptionIndex = 0f;
        [SerializeField] private CorruptionPhase currentPhase = CorruptionPhase.Quiet;

        [Header("Cascade")]
        [SerializeField] private bool cascadeActive = false;
        [SerializeField] private float cascadeCountdown = 90f;
        private const float CASCADE_DURATION = 90f;

        [Header("Light Failure")]
        [SerializeField] private float lightFailureTimer = 0f;
        private int failedLightCount = 0;

        // Properties
        public float CorruptionIndex => corruptionIndex;
        public CorruptionPhase CurrentPhase => currentPhase;
        public bool IsCascadeActive => cascadeActive;
        public float CascadeCountdown => cascadeCountdown;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Update()
        {
            if (!GameManager.Instance || !GameManager.Instance.IsMissionActive) return;

            TickLightFailures(Time.deltaTime);

            if (cascadeActive)
            {
                TickCascade(Time.deltaTime);
            }
        }

        // ====================================================================
        // CORRUPTION CONTROL
        // ====================================================================

        public void AddCorruption(float amount)
        {
            float previous = corruptionIndex;
            corruptionIndex = Mathf.Clamp(corruptionIndex + amount, 0f, 100f);

            UpdatePhase();

            if (corruptionIndex >= 100f && previous < 100f)
            {
                TriggerCascade();
            }

            Debug.Log($"[Corruption] {previous:F1}% -> {corruptionIndex:F1}% (Phase: {currentPhase})");
        }

        public void SetCorruption(float value)
        {
            corruptionIndex = Mathf.Clamp(value, 0f, 100f);
            UpdatePhase();
        }

        public void Reset()
        {
            corruptionIndex = 0f;
            currentPhase = CorruptionPhase.Quiet;
            cascadeActive = false;
            cascadeCountdown = CASCADE_DURATION;
            failedLightCount = 0;
            lightFailureTimer = 0f;
        }

        // ====================================================================
        // PHASE MANAGEMENT
        // ====================================================================

        public static CorruptionPhase GetPhaseForValue(float corruption)
        {
            if (corruption >= 100f) return CorruptionPhase.Cascade;
            if (corruption >= 75f) return CorruptionPhase.Critical;
            if (corruption >= 50f) return CorruptionPhase.Active;
            if (corruption >= 25f) return CorruptionPhase.Stirring;
            return CorruptionPhase.Quiet;
        }

        private void UpdatePhase()
        {
            CorruptionPhase newPhase = GetPhaseForValue(corruptionIndex);
            if (newPhase != currentPhase)
            {
                CorruptionPhase oldPhase = currentPhase;
                currentPhase = newPhase;
                GameEvents.RaisePhaseTransition(oldPhase, newPhase);
                Debug.LogWarning($"[Corruption] PHASE CHANGE: {oldPhase} -> {newPhase}");
            }
            GameEvents.RaiseCorruptionChanged(corruptionIndex, currentPhase);
        }

        // ====================================================================
        // CASCADE
        // ====================================================================

        private void TriggerCascade()
        {
            cascadeActive = true;
            cascadeCountdown = CASCADE_DURATION;
            GameEvents.RaiseCascade(cascadeCountdown);
            Debug.LogError("[Corruption] CASCADE TRIGGERED - 90 seconds to extraction!");
        }

        private void TickCascade(float deltaTime)
        {
            cascadeCountdown -= deltaTime;
            if (cascadeCountdown <= 0f)
            {
                GameManager.Instance?.FailMission();
            }
        }

        // ====================================================================
        // CORRUPTION EFFECTS
        // ====================================================================

        private void TickLightFailures(float deltaTime)
        {
            if (currentPhase < CorruptionPhase.Stirring) return;

            lightFailureTimer += deltaTime;
            float failureInterval = Mathf.Lerp(15f, 3f, (corruptionIndex - 25f) / 75f);

            if (lightFailureTimer >= failureInterval)
            {
                lightFailureTimer = 0f;
                failedLightCount++;
                StationGenerator.Instance?.FailRandomLight();
            }
        }

        /// <summary>Voice chat range multiplier based on corruption (1.0 = full, 0.3 = minimum)</summary>
        public float GetVoiceRangeMultiplier()
        {
            return Mathf.Lerp(1f, 0.3f, corruptionIndex / 100f);
        }

        /// <summary>Radio reliability (1.0 = perfect, 0.0 = useless)</summary>
        public float GetRadioReliability()
        {
            if (corruptionIndex < 25f) return 1f;
            return Mathf.Lerp(1f, 0f, (corruptionIndex - 25f) / 75f);
        }

        /// <summary>Should phantom audio play?</summary>
        public bool ShouldPlayPhantomAudio()
        {
            if (corruptionIndex < 75f) return false;
            return cascadeActive || Random.value < 0.3f;
        }

        /// <summary>Equipment failure rate (0 = never, 0.15 = frequent)</summary>
        public float GetEquipmentFailureRate()
        {
            if (corruptionIndex < 25f) return 0f;
            return Mathf.Lerp(0f, 0.15f, (corruptionIndex - 25f) / 75f);
        }

        /// <summary>Entity spawn interval multiplier (faster at higher corruption)</summary>
        public float GetEntitySpawnIntervalMultiplier()
        {
            if (corruptionIndex < 50f) return 1f;
            return Mathf.Lerp(1f, 0.3f, (corruptionIndex - 50f) / 50f);
        }

        /// <summary>Max active entities for current phase</summary>
        public int GetMaxEntities()
        {
            switch (currentPhase)
            {
                case CorruptionPhase.Quiet: return 0;
                case CorruptionPhase.Stirring: return 2;
                case CorruptionPhase.Active: return 5;
                case CorruptionPhase.Critical: return 8;
                case CorruptionPhase.Cascade: return 12;
                default: return 0;
            }
        }
    }
}
