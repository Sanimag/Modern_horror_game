using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Mission Manager.
    /// Handles the full mission lifecycle: contract generation, deployment,
    /// core extraction workflow, extraction conditions, dynamic events, and debrief.
    /// </summary>
    public class MissionManager : MonoBehaviour
    {
        public static MissionManager Instance { get; private set; }

        [Header("Contracts")]
        [SerializeField] private MissionContract selectedContract;
        [SerializeField] private List<MissionContract> availableContracts = new List<MissionContract>();

        [Header("Deployment")]
        [SerializeField] private bool isDeploying = false;
        [SerializeField] private float pressurizationProgress = 0f;
        private const float PRESSURIZATION_DURATION = 30f;

        [Header("Extraction")]
        [SerializeField] private bool isExtracting = false;
        [SerializeField] private float extractionProgress = 0f;
        [SerializeField] private ExtractionPhase extractionPhase = ExtractionPhase.Scanning;
        private SignalCore extractingCore;
        private PlayerCharacter extractingPlayer;
        private float baseExtractionTime = 15f;
        private float acceleratedExtractionTime = 7.5f;

        [Header("Collected")]
        [SerializeField] private List<SignalCore> extractedCores = new List<SignalCore>();
        [SerializeField] private int creditsEarned = 0;
        [SerializeField] private bool missionSuccess = false;

        [Header("Dynamic Events")]
        [SerializeField] private float eventCheckTimer = 0f;
        [SerializeField] private float eventCheckInterval = 60f;
        [SerializeField] private List<string> activeEvents = new List<string>();

        // Properties
        public MissionContract SelectedContract => selectedContract;
        public bool IsExtracting => isExtracting;
        public float ExtractionProgress => extractionProgress;
        public ExtractionPhase CurrentExtractionPhase => extractionPhase;
        public List<SignalCore> ExtractedCores => extractedCores;
        public int CreditsEarned => creditsEarned;
        public float PressurizationProgress => pressurizationProgress;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        private void Update()
        {
            if (!GameManager.Instance || !GameManager.Instance.IsMissionActive) return;

            if (isDeploying)
                TickPressurization(Time.deltaTime);

            if (isExtracting)
                TickExtraction(Time.deltaTime);

            TickDynamicEvents(Time.deltaTime);
        }

        // ====================================================================
        // CONTRACTS
        // ====================================================================

        public List<MissionContract> GenerateContracts(int companyRank)
        {
            availableContracts.Clear();

            availableContracts.Add(CreateContract(MissionType.StandardSalvage,
                Mathf.Clamp(companyRank / 2, 1, 5), 1.0f,
                "Extract signal cores. More cores = more credits = more danger."));

            if (companyRank >= 2)
                availableContracts.Add(CreateContract(MissionType.DataRecovery,
                    Mathf.Clamp(companyRank / 2 + 1, 2, 5), 1.3f,
                    "Download files from 3 terminals in high-Corruption zones."));

            if (companyRank >= 4)
                availableContracts.Add(CreateContract(MissionType.RescueOp,
                    Mathf.Clamp(companyRank / 3 + 1, 2, 5), 1.5f,
                    "Find and extract a survivor with high Perception Drift."));

            if (companyRank >= 6)
                availableContracts.Add(CreateContract(MissionType.PurgeContract,
                    Mathf.Clamp(companyRank / 3 + 2, 3, 5), 2.0f,
                    "Destroy a specific entity. Extremely dangerous."));

            if (companyRank >= 8)
                availableContracts.Add(CreateContract(MissionType.BlackBox, 5, 3.0f,
                    "Retrieve the flight recorder from the deepest point. One-way trip."));

            return availableContracts;
        }

        public MissionContract GetWeeklyContract()
        {
            var weekly = new MissionContract
            {
                contractID = "Weekly",
                missionType = MissionType.StandardSalvage,
                threatRating = 3,
                payoutMultiplier = 2.0f,
                description = "Weekly Featured Station - Shared leaderboard"
            };
            return weekly;
        }

        public void SelectContract(MissionContract contract)
        {
            selectedContract = contract;
            Debug.Log($"[Mission] Selected: Threat {contract.threatRating}, Payout {contract.payoutMultiplier}x");
        }

        private MissionContract CreateContract(MissionType type, int threat, float payout, string desc)
        {
            var contract = new MissionContract
            {
                contractID = $"Contract_{type}_{Random.Range(1000, 9999)}",
                missionType = type,
                threatRating = threat,
                payoutMultiplier = payout,
                baseCorruption = (threat - 1) * 5f,
                minDepth = threat,
                description = desc
            };

            if (threat >= 3) contract.guaranteedEntities.Add(EntityType.Echo);
            if (threat >= 4) contract.guaranteedEntities.Add(EntityType.Architect);
            if (type == MissionType.PurgeContract) contract.guaranteedEntities.Add(EntityType.Resonant);

            return contract;
        }

        // ====================================================================
        // DEPLOYMENT
        // ====================================================================

        public void BeginDeployment()
        {
            isDeploying = true;
            pressurizationProgress = 0f;
            Debug.Log("[Mission] Submarine deploying...");
        }

        private void TickPressurization(float deltaTime)
        {
            pressurizationProgress += deltaTime / PRESSURIZATION_DURATION;
            if (pressurizationProgress >= 1f)
            {
                isDeploying = false;
                pressurizationProgress = 1f;
                OnPressurizationComplete();
            }
        }

        private void OnPressurizationComplete()
        {
            Debug.Log("[Mission] Pressurization complete. Hatch opening...");
            // Generate and enter station
            int seed = selectedContract != null ?
                selectedContract.contractID.GetHashCode() : Random.Range(0, 99999);
            int depth = selectedContract?.minDepth ?? 1;
            MissionType type = selectedContract?.missionType ?? MissionType.StandardSalvage;

            StationGenerator.Instance?.GenerateStation(seed, depth, type);
            GameManager.Instance?.StartMission(selectedContract);
        }

        // ====================================================================
        // CORE EXTRACTION
        // ====================================================================

        public void BeginCoreExtraction(SignalCore core, PlayerCharacter player)
        {
            if (isExtracting) return;

            isExtracting = true;
            extractionProgress = 0f;
            extractionPhase = ExtractionPhase.Scanning;
            extractingCore = core;
            extractingPlayer = player;

            Debug.Log($"[Mission] Extracting core: {core.coreID} (Value: {core.creditValue}, Corruption: {core.corruptionCost}%)");
        }

        private void TickExtraction(float deltaTime)
        {
            if (extractingPlayer == null || extractingPlayer.CurrentState != PlayerState.Alive)
            {
                CancelExtraction();
                return;
            }

            bool hasAccelerator = extractingPlayer.CurrentRole == PlayerRole.Technician;
            float duration = hasAccelerator ? acceleratedExtractionTime : baseExtractionTime;

            extractionProgress += deltaTime / duration;

            if (extractionProgress < 0.33f) extractionPhase = ExtractionPhase.Scanning;
            else if (extractionProgress < 0.66f) extractionPhase = ExtractionPhase.Disconnecting;
            else extractionPhase = ExtractionPhase.Pulling;

            if (extractionProgress >= 1f)
            {
                CompleteExtraction();
            }
        }

        private void CompleteExtraction()
        {
            isExtracting = false;
            extractedCores.Add(extractingCore);

            // Add corruption
            CorruptionSystem.Instance?.AddCorruption(extractingCore.corruptionCost);

            // Give core to player
            extractingPlayer?.PickUpCore(extractingCore);

            GameEvents.RaiseCoreExtracted(extractingCore);
            Debug.Log($"[Mission] Core extracted! Total: {extractedCores.Count}");

            extractingCore = null;
            extractingPlayer = null;
            extractionProgress = 0f;
        }

        public void CancelExtraction()
        {
            isExtracting = false;
            extractionProgress = 0f;
            extractingCore = null;
            extractingPlayer = null;
            Debug.LogWarning("[Mission] Extraction cancelled");
        }

        // ====================================================================
        // AIRLOCK & EXIT
        // ====================================================================

        public bool CanExtract()
        {
            return GameManager.Instance != null && GameManager.Instance.AreAllPlayersAccountedFor();
        }

        public void ForceExtraction()
        {
            Debug.LogWarning("[Mission] FORCE EXTRACTION - leaving players behind!");
            ProcessDebrief(true);
        }

        public void ProcessDebrief(bool success)
        {
            missionSuccess = success;

            if (ProgressionManager.Instance != null && selectedContract != null)
            {
                // Find the local player's role
                PlayerRole playedRole = PlayerRole.None;
                var players = GameManager.Instance?.GetAlivePlayers();
                if (players != null && players.Count > 0)
                    playedRole = players[0].CurrentRole;

                creditsEarned = ProgressionManager.Instance.ProcessMissionRewards(
                    extractedCores, selectedContract.payoutMultiplier, playedRole, success);
            }

            GameManager.Instance?.CompleteMission();
        }

        // ====================================================================
        // DYNAMIC EVENTS
        // ====================================================================

        private void TickDynamicEvents(float deltaTime)
        {
            eventCheckTimer += deltaTime;
            if (eventCheckTimer < eventCheckInterval) return;
            eventCheckTimer = 0f;

            float roll = Random.value;

            if (roll < 0.15f)
            {
                activeEvents.Add("HullBreach");
                GameEvents.RaiseDynamicEvent("HullBreach");
                Debug.LogWarning("[Event] HULL BREACH - section flooding!");
            }
            else if (roll < 0.25f)
            {
                activeEvents.Add("PowerSurge");
                GameEvents.RaiseDynamicEvent("PowerSurge");
                Debug.LogWarning("[Event] POWER SURGE - station illuminated!");
            }
            else if (roll < 0.35f)
            {
                activeEvents.Add("DistressSignal");
                GameEvents.RaiseDynamicEvent("DistressSignal");
                Debug.LogWarning("[Event] Distress signal from another crew...");
            }
            else if (roll < 0.45f)
            {
                activeEvents.Add("EquipmentCache");
                GameEvents.RaiseDynamicEvent("EquipmentCache");
                Debug.Log("[Event] Previous crew equipment cache found");
            }
        }
    }
}
