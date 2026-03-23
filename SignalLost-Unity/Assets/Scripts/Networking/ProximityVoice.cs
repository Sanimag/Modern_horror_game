using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Proximity Voice Chat System.
    /// Handles distance-based voice attenuation, radio communication,
    /// signal degradation effects, and phantom audio injection.
    /// </summary>
    public class ProximityVoice : MonoBehaviour
    {
        public static ProximityVoice Instance { get; private set; }

        [Header("Proximity Voice")]
        [SerializeField] private float baseProximityRange = 20f;
        [SerializeField] private float currentProximityRange = 20f;

        [Header("Radio")]
        [SerializeField] private bool radioActive = false;
        [SerializeField] private float radioBatteryLife = 100f;
        [SerializeField] private float radioDrainRate = 5f; // % per minute
        [SerializeField] private float radioReliability = 1f;
        [SerializeField] private float radioNoiseOutput = 0f;

        [Header("Signal Degradation")]
        [SerializeField] private float degradationLevel = 0f;

        [Header("Phantom Audio")]
        [SerializeField] private float phantomAudioTimer = 0f;
        [SerializeField] private float phantomAudioInterval = 30f;
        [SerializeField] private List<AudioClip> phantomClips;
        [SerializeField] private AudioSource phantomAudioSource;

        private readonly string[] phantomEventNames = {
            "Footsteps", "DoorSlam", "CrewVoice_Help", "CrewVoice_Behind",
            "CrewVoice_RunNow", "Breathing", "RadioStatic", "MetalScrape",
            "WaterDrip", "Whisper"
        };

        // Properties
        public bool RadioActive => radioActive;
        public float RadioBattery => radioBatteryLife;
        public float DegradationLevel => degradationLevel;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Update()
        {
            var corruption = CorruptionSystem.Instance;
            if (corruption != null)
                UpdateSignalDegradation(corruption.CorruptionIndex);

            TickRadioBattery(Time.deltaTime);
            TickPhantomAudio(Time.deltaTime);
        }

        // ====================================================================
        // PROXIMITY VOICE
        // ====================================================================

        /// <summary>Get volume multiplier for voice at a given distance</summary>
        public float GetVolumeForDistance(float distance)
        {
            if (distance >= currentProximityRange) return 0f;
            if (distance <= 0f) return 1f;
            float norm = distance / currentProximityRange;
            return Mathf.Clamp01(1f - norm * norm); // Inverse square falloff
        }

        /// <summary>Get distortion level for voice processing</summary>
        public float GetVoiceDistortionLevel() => degradationLevel;

        // ====================================================================
        // RADIO
        // ====================================================================

        public void ToggleRadio()
        {
            if (radioBatteryLife <= 0f) { radioActive = false; return; }
            radioActive = !radioActive;
        }

        public bool IsRadioUsable() =>
            radioActive && radioBatteryLife > 0f && radioReliability > 0.1f;

        private void TickRadioBattery(float deltaTime)
        {
            if (!radioActive) return;

            radioBatteryLife -= (radioDrainRate / 60f) * deltaTime;
            if (radioBatteryLife <= 0f)
            {
                radioBatteryLife = 0f;
                radioActive = false;
                Debug.LogWarning("[Voice] Radio battery depleted");
            }

            radioNoiseOutput = Mathf.Lerp(0.1f, 0.8f, degradationLevel);
        }

        // ====================================================================
        // SIGNAL DEGRADATION
        // ====================================================================

        public void UpdateSignalDegradation(float corruptionPercent)
        {
            currentProximityRange = baseProximityRange *
                Mathf.Lerp(1f, 0.3f, corruptionPercent / 100f);

            if (corruptionPercent < 25f)
            {
                radioReliability = 1f;
                degradationLevel = 0f;
            }
            else
            {
                radioReliability = Mathf.Lerp(1f, 0f, (corruptionPercent - 25f) / 75f);
                degradationLevel = Mathf.Lerp(0f, 1f, (corruptionPercent - 25f) / 75f);
            }

            phantomAudioInterval = Mathf.Lerp(60f, 8f, degradationLevel);
        }

        // ====================================================================
        // PHANTOM AUDIO
        // ====================================================================

        public bool ShouldInjectPhantomAudio() => degradationLevel > 0.6f;

        private void TickPhantomAudio(float deltaTime)
        {
            if (!ShouldInjectPhantomAudio()) return;

            phantomAudioTimer += deltaTime;
            if (phantomAudioTimer < phantomAudioInterval) return;

            phantomAudioTimer = 0f;

            string eventName = phantomEventNames[Random.Range(0, phantomEventNames.Length)];
            Debug.Log($"[Phantom] Audio event: {eventName}");

            // Play phantom audio clip
            if (phantomClips != null && phantomClips.Count > 0 && phantomAudioSource != null)
            {
                var clip = phantomClips[Random.Range(0, phantomClips.Count)];
                phantomAudioSource.PlayOneShot(clip);
            }
        }
    }
}
