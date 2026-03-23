using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Minimal HUD for Signal Lost.
    /// Design: minimal on-screen elements.
    /// - Health = screen vignetting + heartbeat audio
    /// - Corruption % = Scanner role only
    /// - Equipment = physical model indicators
    /// - Inventory = wrist-mounted device (player must look down)
    /// </summary>
    public class SignalLostHUD : MonoBehaviour
    {
        public static SignalLostHUD Instance { get; private set; }

        [Header("References")]
        [SerializeField] private PlayerCharacter localPlayer;

        [Header("Wrist Device")]
        [SerializeField] private bool wristDeviceOpen = false;

        [Header("Effects")]
        [SerializeField] private float frequencyBurnTimer = 0f;
        [SerializeField] private float frequencyBurnIntensity = 0f;
        [SerializeField] private float broadcastDistortion = 0f;

        // GUIStyle cache
        private GUIStyle headerStyle;
        private GUIStyle bodyStyle;
        private GUIStyle warningStyle;
        private GUIStyle criticalStyle;
        private bool stylesInitialized = false;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Update()
        {
            if (localPlayer == null)
            {
                localPlayer = FindObjectOfType<PlayerCharacter>();
            }

            if (frequencyBurnTimer > 0f)
                frequencyBurnTimer -= Time.deltaTime;
        }

        private void InitStyles()
        {
            if (stylesInitialized) return;

            headerStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 16,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(0.2f, 1f, 0.3f) }
            };
            bodyStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                normal = { textColor = new Color(0.2f, 0.9f, 0.3f) }
            };
            warningStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 20,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = Color.yellow }
            };
            criticalStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 28,
                fontStyle = FontStyle.Bold,
                alignment = TextAnchor.MiddleCenter,
                normal = { textColor = Color.red }
            };
            stylesInitialized = true;
        }

        private void OnGUI()
        {
            if (localPlayer == null) return;
            InitStyles();

            DrawHealthVignette();
            DrawAirlockCompass();

            // Scanner: corruption indicator
            if (localPlayer.Passives?.canSeeCorruptionHUD == true)
                DrawCorruptionIndicator();

            // Signal Fade
            if (localPlayer.CurrentState == PlayerState.SignalFade)
                DrawSignalFadeTimer();

            // Cascade countdown
            var corruption = CorruptionSystem.Instance;
            if (corruption != null && corruption.IsCascadeActive)
                DrawCascadeCountdown(corruption.CascadeCountdown);

            // Extraction progress
            var mission = MissionManager.Instance;
            if (mission != null && mission.IsExtracting)
                DrawExtractionProgress(mission.ExtractionProgress, mission.CurrentExtractionPhase);

            // Equipment resync
            if (localPlayer.Equipment != null && localPlayer.Equipment.IsResyncing)
                DrawResyncProgress(localPlayer.Equipment.ResyncProgress);

            // Wrist device
            if (wristDeviceOpen)
                DrawWristDevice();

            // Perception drift effects
            if (localPlayer.PerceptionDrift > 20f)
                DrawPerceptionDriftOverlay();

            // Frequency burn effect
            if (frequencyBurnTimer > 0f)
                DrawFrequencyBurnEffect();

            // Broadcast distortion
            if (broadcastDistortion > 0.1f)
                DrawBroadcastDistortionEffect();
        }

        // ====================================================================
        // HUD ELEMENTS
        // ====================================================================

        private void DrawHealthVignette()
        {
            float healthPercent = localPlayer.Health / localPlayer.MaxHealth;
            if (healthPercent >= 0.8f) return;

            float alpha = (1f - healthPercent) * 0.5f;
            Color vignetteColor = healthPercent < 0.25f
                ? new Color(0.5f, 0f, 0f, alpha * (Mathf.Sin(Time.time * 3f) * 0.3f + 0.7f))
                : new Color(0f, 0f, 0f, alpha);

            // Draw screen-edge darkening
            Texture2D tex = Texture2D.whiteTexture;
            GUI.color = vignetteColor;
            float border = 80f * (1f - healthPercent);
            GUI.DrawTexture(new Rect(0, 0, Screen.width, border), tex);
            GUI.DrawTexture(new Rect(0, Screen.height - border, Screen.width, border), tex);
            GUI.DrawTexture(new Rect(0, 0, border, Screen.height), tex);
            GUI.DrawTexture(new Rect(Screen.width - border, 0, border, Screen.height), tex);
            GUI.color = Color.white;
        }

        private void DrawCorruptionIndicator()
        {
            var corruption = CorruptionSystem.Instance;
            if (corruption == null) return;

            Color phaseColor;
            string phaseName;
            switch (corruption.CurrentPhase)
            {
                case CorruptionPhase.Quiet: phaseColor = Color.green; phaseName = "QUIET"; break;
                case CorruptionPhase.Stirring: phaseColor = Color.yellow; phaseName = "STIRRING"; break;
                case CorruptionPhase.Active: phaseColor = new Color(1, 0.5f, 0); phaseName = "ACTIVE"; break;
                case CorruptionPhase.Critical: phaseColor = Color.red; phaseName = "CRITICAL"; break;
                case CorruptionPhase.Cascade: phaseColor = Color.red; phaseName = "CASCADE"; break;
                default: phaseColor = Color.white; phaseName = "???"; break;
            }

            var style = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                normal = { textColor = phaseColor },
                alignment = TextAnchor.UpperRight
            };

            GUI.Label(new Rect(Screen.width - 220, 20, 200, 30),
                $"{corruption.CorruptionIndex:F0}% {phaseName}", style);
        }

        private void DrawSignalFadeTimer()
        {
            float y = Screen.height * 0.4f;

            // Pulsing red
            float pulse = Mathf.Sin(Time.time * 2f) * 0.3f + 0.7f;
            criticalStyle.normal.textColor = new Color(1f, pulse * 0.2f, pulse * 0.2f);

            GUI.Label(new Rect(0, y, Screen.width, 40),
                $"SIGNAL FADING: {localPlayer.SignalFadeTimer:F0}", criticalStyle);

            warningStyle.fontSize = 14;
            warningStyle.normal.textColor = new Color(0.8f, 0.8f, 0.8f, 0.8f);
            GUI.Label(new Rect(0, y + 45, Screen.width, 30),
                "[F] BROADCAST - Sacrifice to reveal all entities", warningStyle);
        }

        private void DrawCascadeCountdown(float countdown)
        {
            bool flash = (Time.time % 0.5f) < 0.25f;
            if (!flash) return;

            GUI.Label(new Rect(0, 50, Screen.width, 40),
                $"CASCADE: {countdown:F0}", criticalStyle);
        }

        private void DrawExtractionProgress(float progress, ExtractionPhase phase)
        {
            string[] phaseNames = { "SCANNING", "DISCONNECTING", "EXTRACTING" };
            string phaseName = phaseNames[(int)phase];

            float barWidth = 300f;
            float barHeight = 20f;
            float x = (Screen.width - barWidth) / 2f;
            float y = Screen.height * 0.7f;

            // Background
            GUI.color = new Color(0.15f, 0.15f, 0.15f, 0.8f);
            GUI.DrawTexture(new Rect(x, y, barWidth, barHeight), Texture2D.whiteTexture);

            // Fill
            GUI.color = new Color(0.2f, 0.8f, 0.3f, 0.9f);
            GUI.DrawTexture(new Rect(x, y, barWidth * progress, barHeight), Texture2D.whiteTexture);

            GUI.color = Color.white;
            GUI.Label(new Rect(x, y - 20, barWidth, 20), phaseName, bodyStyle);
        }

        private void DrawResyncProgress(float progress)
        {
            float barWidth = 200f;
            float barHeight = 15f;
            float x = (Screen.width - barWidth) / 2f;
            float y = Screen.height * 0.6f;

            GUI.color = new Color(0.1f, 0.1f, 0.1f, 0.8f);
            GUI.DrawTexture(new Rect(x, y, barWidth, barHeight), Texture2D.whiteTexture);
            GUI.color = new Color(0.3f, 0.6f, 1f, 0.9f);
            GUI.DrawTexture(new Rect(x, y, barWidth * progress, barHeight), Texture2D.whiteTexture);
            GUI.color = Color.white;
            GUI.Label(new Rect(x, y - 18, barWidth, 18), "RESYNCING...", bodyStyle);
        }

        private void DrawAirlockCompass()
        {
            if (GameManager.Instance == null) return;

            Vector3 toAirlock = GameManager.Instance.AirlockPosition - localPlayer.transform.position;
            toAirlock.y = 0;
            toAirlock.Normalize();

            Vector3 forward = localPlayer.transform.forward;
            forward.y = 0;
            forward.Normalize();

            float angle = Vector3.SignedAngle(forward, toAirlock, Vector3.up);
            float compassX = Screen.width / 2f + Mathf.Sin(angle * Mathf.Deg2Rad) * 40f;
            float compassY = Screen.height - 60f;

            var compassStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 18,
                normal = { textColor = new Color(1f, 0.5f, 0.2f, 0.7f) },
                alignment = TextAnchor.MiddleCenter
            };

            GUI.Label(new Rect(compassX - 10, compassY, 20, 20), "^", compassStyle);

            compassStyle.fontSize = 10;
            GUI.Label(new Rect(Screen.width / 2f - 30, Screen.height - 40, 60, 20),
                "AIRLOCK", compassStyle);
        }

        // ====================================================================
        // WRIST DEVICE
        // ====================================================================

        public void ToggleWristDevice()
        {
            wristDeviceOpen = !wristDeviceOpen;
        }

        private void DrawWristDevice()
        {
            float w = Screen.width * 0.4f;
            float h = Screen.height * 0.5f;
            float x = Screen.width * 0.3f;
            float y = Screen.height * 0.35f;

            // Background (dark terminal green)
            GUI.color = new Color(0.02f, 0.05f, 0.02f, 0.95f);
            GUI.DrawTexture(new Rect(x, y, w, h), Texture2D.whiteTexture);
            GUI.color = Color.white;

            float textY = y + 15;
            float lineH = 20f;
            float textX = x + 15;

            GUI.Label(new Rect(textX, textY, w, lineH),
                $"ROLE: {localPlayer.CurrentRole}", headerStyle);
            textY += lineH;

            GUI.Label(new Rect(textX, textY, w, lineH),
                $"VITALS: {localPlayer.Health:F0}%", bodyStyle);
            textY += lineH;

            GUI.Label(new Rect(textX, textY, w, lineH),
                $"PERCEPTION: {(localPlayer.IsPerceptionCompromised ? "COMPROMISED" : "NORMAL")}", bodyStyle);
            textY += lineH * 1.5f;

            GUI.Label(new Rect(textX, textY, w, lineH),
                $"CORES: {localPlayer.CarriedCores.Count}/{localPlayer.MaxCarryCores}", headerStyle);
            textY += lineH;

            foreach (var core in localPlayer.CarriedCores)
            {
                GUI.Label(new Rect(textX + 10, textY, w, lineH),
                    $"> {core.coreID}: {core.creditValue} cr ({core.integrity:F0}% integrity)", bodyStyle);
                textY += lineH;
            }

            textY += lineH;
            GUI.Label(new Rect(textX, textY, w, lineH), "EQUIPMENT:", headerStyle);
            textY += lineH;

            var equip = localPlayer.Equipment;
            if (equip != null)
            {
                for (int i = 0; i < equip.Inventory.Count; i++)
                {
                    var item = equip.Inventory[i];
                    string active = i == equip.ActiveSlot ? ">> " : "   ";
                    string charges = item.maxCharges >= 0 ? $" [{item.currentCharges}]" : $" [{item.batteryLife:F0}%]";
                    GUI.Label(new Rect(textX, textY, w, lineH),
                        $"{active}{item.displayName}{charges}", bodyStyle);
                    textY += lineH;
                }
            }

            // Team status
            textY += lineH;
            GUI.Label(new Rect(textX, textY, w, lineH), "CREW STATUS:", headerStyle);
            textY += lineH;

            var players = GameManager.Instance?.GetAlivePlayers();
            if (players != null)
            {
                GUI.Label(new Rect(textX + 10, textY, w, lineH),
                    $"Alive: {players.Count} / {GameManager.Instance.GetPlayerCount()}", bodyStyle);
            }
        }

        // ====================================================================
        // EFFECTS
        // ====================================================================

        public void ApplyFrequencyBurn(float intensity, float duration)
        {
            frequencyBurnIntensity = intensity;
            frequencyBurnTimer = duration;
        }

        public void ApplyBroadcastDistortion(float intensity)
        {
            broadcastDistortion = intensity;
        }

        private void DrawPerceptionDriftOverlay()
        {
            float drift = localPlayer.PerceptionDrift / 100f;

            if (drift > 0.3f)
            {
                // Shadow movement at screen edges
                float shadowAlpha = drift * 0.3f;
                float offset = Mathf.Sin(Time.time * 0.5f) * drift * 50f;
                GUI.color = new Color(0, 0, 0, shadowAlpha);
                GUI.DrawTexture(new Rect(-20 + offset, 0, 40, Screen.height), Texture2D.whiteTexture);
                GUI.DrawTexture(new Rect(Screen.width - 20 - offset, 0, 40, Screen.height), Texture2D.whiteTexture);
                GUI.color = Color.white;
            }

            if (drift > 0.5f)
            {
                // Slight color tint
                GUI.color = new Color(0.1f, 0.3f, 0.1f, drift * 0.1f);
                GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture);
                GUI.color = Color.white;
            }
        }

        private void DrawFrequencyBurnEffect()
        {
            // Static-like screen disruption
            float alpha = frequencyBurnIntensity * (frequencyBurnTimer > 0 ? 1f : 0f) * 0.4f;
            GUI.color = new Color(0.8f, 0.2f, 0.1f, alpha * Random.Range(0.5f, 1f));
            GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture);
            GUI.color = Color.white;
        }

        private void DrawBroadcastDistortionEffect()
        {
            float alpha = broadcastDistortion * 0.5f;
            // Vertical scan line effect
            float scanY = (Time.time * 200f) % Screen.height;
            GUI.color = new Color(1, 1, 1, alpha);
            GUI.DrawTexture(new Rect(0, scanY, Screen.width, 3), Texture2D.whiteTexture);
            GUI.color = Color.white;

            broadcastDistortion *= 0.95f; // Decay
        }
    }
}
