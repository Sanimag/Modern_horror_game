using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Scene bootstrap - automatically sets up all manager objects
    /// and creates a playable test scene with procedural station.
    /// Attach this to an empty GameObject in your scene to auto-initialize.
    /// </summary>
    public class SceneBootstrap : MonoBehaviour
    {
        [Header("Test Settings")]
        [SerializeField] private int stationSeed = 12345;
        [SerializeField] private int stationDepth = 2;
        [SerializeField] private MissionType testMissionType = MissionType.StandardSalvage;
        [SerializeField] private PlayerRole testRole = PlayerRole.Scanner;
        [SerializeField] private bool autoStartMission = true;

        private void Awake()
        {
            CreateManagers();
        }

        private void Start()
        {
            if (autoStartMission)
            {
                Invoke(nameof(StartTestMission), 0.5f);
            }
        }

        private void CreateManagers()
        {
            // Game Manager
            if (GameManager.Instance == null)
            {
                var go = new GameObject("[GameManager]");
                go.AddComponent<GameManager>();
            }

            // Corruption System
            if (CorruptionSystem.Instance == null)
            {
                var go = new GameObject("[CorruptionSystem]");
                go.AddComponent<CorruptionSystem>();
            }

            // Station Generator
            if (StationGenerator.Instance == null)
            {
                var go = new GameObject("[StationGenerator]");
                go.AddComponent<StationGenerator>();
            }

            // Mission Manager
            if (MissionManager.Instance == null)
            {
                var go = new GameObject("[MissionManager]");
                go.AddComponent<MissionManager>();
            }

            // Progression
            if (ProgressionManager.Instance == null)
            {
                var go = new GameObject("[ProgressionManager]");
                go.AddComponent<ProgressionManager>();
            }

            // Network
            if (SLNetworkManager.Instance == null)
            {
                var go = new GameObject("[NetworkManager]");
                go.AddComponent<SLNetworkManager>();
            }

            // Voice
            if (ProximityVoice.Instance == null)
            {
                var go = new GameObject("[ProximityVoice]");
                go.AddComponent<ProximityVoice>();
            }

            // Audio
            if (HorrorAudioManager.Instance == null)
            {
                var go = new GameObject("[AudioManager]");
                var audio = go.AddComponent<HorrorAudioManager>();
                // Add audio sources
                var ambient = go.AddComponent<AudioSource>();
                ambient.loop = true;
                ambient.playOnAwake = false;
                go.AddComponent<AudioSource>(); // stinger
                go.AddComponent<AudioSource>(); // heartbeat
                go.AddComponent<AudioSource>(); // environment
                go.AddComponent<AudioSource>(); // entity proximity
            }

            // HUD
            if (SignalLostHUD.Instance == null)
            {
                var go = new GameObject("[HUD]");
                go.AddComponent<SignalLostHUD>();
            }

            // Create Player
            CreatePlayer();

            // Create airlock marker
            CreateAirlock();

            Debug.Log("[Bootstrap] All systems initialized");
        }

        private void CreatePlayer()
        {
            // Create player with camera
            var player = new GameObject("Player");
            player.transform.position = new Vector3(0, 1, 0);
            player.layer = LayerMask.NameToLayer("Default");

            // CharacterController
            var cc = player.AddComponent<CharacterController>();
            cc.height = 2f;
            cc.radius = 0.4f;
            cc.center = new Vector3(0, 1f, 0);

            // Camera
            var cameraObj = new GameObject("PlayerCamera");
            cameraObj.transform.SetParent(player.transform);
            cameraObj.transform.localPosition = new Vector3(0, 1.6f, 0);
            var cam = cameraObj.AddComponent<Camera>();
            cam.nearClipPlane = 0.1f;
            cam.farClipPlane = 500f;
            cam.fieldOfView = 70f;
            cam.backgroundColor = new Color(0.02f, 0.02f, 0.05f); // Near black
            cam.clearFlags = CameraClearFlags.SolidColor;

            // Audio listener on camera
            cameraObj.AddComponent<AudioListener>();

            // Flashlight
            var flashlightObj = new GameObject("Flashlight");
            flashlightObj.transform.SetParent(cameraObj.transform);
            flashlightObj.transform.localPosition = Vector3.zero;
            var flashlight = flashlightObj.AddComponent<Light>();
            flashlight.type = LightType.Spot;
            flashlight.color = new Color(0.95f, 0.9f, 0.8f);
            flashlight.intensity = 2f;
            flashlight.range = 25f;
            flashlight.spotAngle = 45f;
            flashlight.innerSpotAngle = 25f;
            flashlight.enabled = true;

            // PlayerCharacter component
            var playerComp = player.AddComponent<PlayerCharacter>();

            // Equipment
            player.AddComponent<EquipmentManager>();

            // Set camera reference via reflection (since it's serialized)
            var cameraField = typeof(PlayerCharacter).GetField("cameraTransform",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            cameraField?.SetValue(playerComp, cameraObj.transform);

            var flashlightField = typeof(PlayerCharacter).GetField("flashlight",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            flashlightField?.SetValue(playerComp, flashlight);

            // Set role
            playerComp.SetRole(testRole);

            // Give starting equipment
            var equip = player.GetComponent<EquipmentManager>();
            equip.AddEquipment(new EquipmentData(EquipmentType.Flashlight, "Flashlight", 0));
            equip.AddEquipment(new EquipmentData(EquipmentType.GlowStick, "Glow Sticks", 0, 6));
            equip.AddEquipment(new EquipmentData(EquipmentType.ResyncKit, "Resync Kit", 0, 2));

            // Remove default audio listener if any
            var defaultListener = FindObjectOfType<AudioListener>();
            if (defaultListener != null && defaultListener.gameObject != cameraObj)
                Destroy(defaultListener);

            Debug.Log($"[Bootstrap] Player created with role: {testRole}");
        }

        private void CreateAirlock()
        {
            var airlock = new GameObject("Airlock");
            airlock.transform.position = Vector3.zero;

            // Visual marker
            var marker = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            marker.transform.SetParent(airlock.transform);
            marker.transform.localPosition = Vector3.zero;
            marker.transform.localScale = new Vector3(3f, 0.1f, 3f);
            marker.name = "AirlockMarker";

            // Light
            var lightObj = new GameObject("AirlockLight");
            lightObj.transform.SetParent(airlock.transform);
            lightObj.transform.localPosition = new Vector3(0, 3f, 0);
            var light = lightObj.AddComponent<Light>();
            light.type = LightType.Point;
            light.color = new Color(1f, 0.3f, 0.1f);
            light.intensity = 1f;
            light.range = 8f;

            // Set in GameManager
            var airlockField = typeof(GameManager).GetField("airlockTransform",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
            airlockField?.SetValue(GameManager.Instance, airlock.transform);
        }

        private void StartTestMission()
        {
            // Generate station
            StationGenerator.Instance?.GenerateStation(stationSeed, stationDepth, testMissionType);

            // Create and start mission
            var contract = new MissionContract
            {
                contractID = "TestMission",
                missionType = testMissionType,
                threatRating = stationDepth,
                payoutMultiplier = 1.5f,
                baseCorruption = 0f,
                description = "Test Mission"
            };

            MissionManager.Instance?.SelectContract(contract);
            GameManager.Instance?.StartMission(contract);

            // Spawn some test cores in the station
            SpawnTestCores();

            // Set ambient lighting
            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
            RenderSettings.ambientLight = new Color(0.02f, 0.02f, 0.05f);
            RenderSettings.fog = true;
            RenderSettings.fogColor = new Color(0.01f, 0.01f, 0.03f);
            RenderSettings.fogMode = FogMode.Exponential;
            RenderSettings.fogDensity = 0.03f;

            Debug.Log("[Bootstrap] Test mission started!");
        }

        private void SpawnTestCores()
        {
            var spawnPoints = StationGenerator.Instance?.GetCoreSpawnPoints();
            if (spawnPoints == null || spawnPoints.Count == 0) return;

            int coreCount = Mathf.Min(8, spawnPoints.Count);
            for (int i = 0; i < coreCount; i++)
            {
                var coreObj = GameObject.CreatePrimitive(PrimitiveType.Sphere);
                coreObj.transform.position = spawnPoints[i] + Vector3.up * 0.5f;
                coreObj.transform.localScale = Vector3.one * 0.3f;
                coreObj.name = $"SignalCore_{i}";

                // Glowing material
                var renderer = coreObj.GetComponent<Renderer>();
                if (renderer != null)
                {
                    renderer.material.color = new Color(0.2f, 0.8f, 1f);
                    renderer.material.EnableKeyword("_EMISSION");
                    renderer.material.SetColor("_EmissionColor", new Color(0.1f, 0.4f, 0.6f));
                }

                // Add pickup component
                var pickup = coreObj.AddComponent<SignalCorePickup>();
                pickup.CoreData = new SignalCore(
                    $"Core_{i}",
                    Random.Range(50, 300),
                    Random.Range(5f, 15f),
                    Random.Range(0.5f, 2f)
                );

                // Light
                var light = coreObj.AddComponent<Light>();
                light.color = new Color(0.2f, 0.8f, 1f);
                light.intensity = 0.5f;
                light.range = 3f;

                // Make interactable
                coreObj.layer = LayerMask.NameToLayer("Default");
            }

            Debug.Log($"[Bootstrap] Spawned {coreCount} signal cores");
        }
    }
}
