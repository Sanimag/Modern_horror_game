using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Procedural Station Generator.
    /// Assembles underwater relay stations from hand-crafted room modules
    /// connected by procedurally generated corridors.
    /// Uses a room graph with BFS validation for connectivity.
    /// </summary>
    public class StationGenerator : MonoBehaviour
    {
        public static StationGenerator Instance { get; private set; }

        [Header("Generation Settings")]
        [SerializeField] private int minRooms = 15;
        [SerializeField] private int maxRooms = 40;
        [SerializeField] private float minCorridorLength = 5f;
        [SerializeField] private float maxCorridorLength = 20f;
        [SerializeField] private float minRoomSpacing = 2f;

        [Header("Room Prefabs")]
        public List<GameObject> processingHallPrefabs;
        public List<GameObject> serverCryptPrefabs;
        public List<GameObject> maintenanceTunnelPrefabs;
        public List<GameObject> pressureChamberPrefabs;
        public List<GameObject> observationDomePrefabs;
        public List<GameObject> controlRoomPrefabs;
        public List<GameObject> junctionPrefabs;
        public List<GameObject> airlockPrefabs;
        public List<GameObject> corridorPrefabs;

        [Header("State")]
        [SerializeField] private int stationDepth = 1;

        // Room graph
        private Dictionary<string, RoomNode> roomGraph = new Dictionary<string, RoomNode>();
        private List<RoomModule> generatedRooms = new List<RoomModule>();
        private string airlockRoomID;
        private System.Random rng;

        // Spawn points
        private List<Vector3> coreSpawnPoints = new List<Vector3>();
        private List<Vector3> entitySpawnPoints = new List<Vector3>();
        private List<int> failedLights = new List<int>();
        private int totalLightCount = 0;

        // Spawned room GameObjects
        private List<GameObject> spawnedRoomObjects = new List<GameObject>();

        private class RoomNode
        {
            public string roomID;
            public RoomModule module;
            public Vector3 worldPosition;
            public Quaternion worldRotation = Quaternion.identity;
            public List<string> connectedRoomIDs = new List<string>();
            public bool isModifiedByArchitect = false;
            public int depthFromAirlock = 0;
            public GameObject gameObject;
        }

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
        }

        // ====================================================================
        // GENERATION
        // ====================================================================

        public void GenerateStation(int seed, int depth, MissionType missionType)
        {
            rng = new System.Random(seed);
            stationDepth = depth;

            ClearStation();

            int roomCount = Mathf.Clamp(
                minRooms + (depth * 5) + RandomRange(-3, 3),
                minRooms, maxRooms);

            Debug.Log($"[StationGen] Seed={seed}, Depth={depth}, Rooms={roomCount}");

            PlaceAirlock();

            int mainBranchRooms = Mathf.RoundToInt(roomCount * 0.6f);
            GenerateMainBranch(mainBranchRooms);

            int sideBranchRooms = roomCount - mainBranchRooms - 1;
            int branchCount = Mathf.Max(2, sideBranchRooms / 3);
            GenerateSideBranches(branchCount);

            DistributeLoot(missionType);
            DistributeEntitySpawns();

            if (ValidateLayout())
            {
                SpawnRoomGeometry();
                Debug.Log($"[StationGen] Success: {roomGraph.Count} rooms, {coreSpawnPoints.Count} cores, {entitySpawnPoints.Count} entity spawns");
            }
            else
            {
                Debug.LogError("[StationGen] Validation failed! Regenerating...");
                GenerateStation(seed + 1, depth, missionType);
            }
        }

        public void ClearStation()
        {
            foreach (var go in spawnedRoomObjects)
                if (go != null) Destroy(go);

            spawnedRoomObjects.Clear();
            roomGraph.Clear();
            generatedRooms.Clear();
            coreSpawnPoints.Clear();
            entitySpawnPoints.Clear();
            failedLights.Clear();
            totalLightCount = 0;
        }

        // ====================================================================
        // ROOM PLACEMENT
        // ====================================================================

        private void PlaceAirlock()
        {
            var node = new RoomNode
            {
                roomID = "Airlock_Main",
                module = new RoomModule
                {
                    roomID = "Airlock_Main",
                    roomType = RoomType.Airlock,
                    roomSize = new Vector3(8f, 4f, 6f),
                    maxDoorways = 2,
                    hasEmergencyLighting = true,
                    isArchitectModifiable = false
                },
                worldPosition = Vector3.zero,
                depthFromAirlock = 0
            };

            airlockRoomID = node.roomID;
            roomGraph.Add(node.roomID, node);
            generatedRooms.Add(node.module);
        }

        private void GenerateMainBranch(int count)
        {
            string previousID = airlockRoomID;
            Vector3 currentPos = Vector3.zero;

            for (int i = 0; i < count; i++)
            {
                float corridorLen = RandomRange(minCorridorLength, maxCorridorLength);
                float angle = RandomRange(-45f, 45f);
                Vector3 dir = new Vector3(
                    Mathf.Cos(angle * Mathf.Deg2Rad),
                    0f,
                    Mathf.Sin(angle * Mathf.Deg2Rad)
                ).normalized;

                currentPos += dir * corridorLen;

                int depth = i + 1;
                RoomType roomType = SelectRoomType(depth);

                var node = CreateRoomNode($"Main_{i}", roomType, currentPos, depth);

                // Connect
                node.connectedRoomIDs.Add(previousID);
                roomGraph[previousID].connectedRoomIDs.Add(node.roomID);

                roomGraph.Add(node.roomID, node);
                generatedRooms.Add(node.module);
                previousID = node.roomID;
            }
        }

        private void GenerateSideBranches(int branchCount)
        {
            var mainRooms = roomGraph.Keys.Where(k => k.StartsWith("Main_")).ToList();

            for (int b = 0; b < branchCount && mainRooms.Count > 0; b++)
            {
                string branchPointID = mainRooms[RandomRange(0, mainRooms.Count)];
                var branchPoint = roomGraph[branchPointID];

                if (branchPoint.connectedRoomIDs.Count >= branchPoint.module.maxDoorways) continue;

                int branchLength = RandomRange(2, 5);
                string prevID = branchPointID;
                Vector3 pos = branchPoint.worldPosition;

                for (int r = 0; r < branchLength; r++)
                {
                    float corridorLen = RandomRange(minCorridorLength, maxCorridorLength);
                    float angle = RandomRange(60f, 120f) * (rng.NextDouble() > 0.5 ? 1f : -1f);
                    Vector3 dir = new Vector3(Mathf.Cos(angle * Mathf.Deg2Rad), 0, Mathf.Sin(angle * Mathf.Deg2Rad)).normalized;
                    pos += dir * corridorLen;

                    int depth = branchPoint.depthFromAirlock + r + 1;
                    var node = CreateRoomNode($"Side_{b}_{r}", SelectRoomType(depth), pos, depth);

                    node.connectedRoomIDs.Add(prevID);
                    roomGraph[prevID].connectedRoomIDs.Add(node.roomID);

                    roomGraph.Add(node.roomID, node);
                    generatedRooms.Add(node.module);
                    prevID = node.roomID;
                }
            }
        }

        private RoomNode CreateRoomNode(string id, RoomType type, Vector3 pos, int depth)
        {
            var module = new RoomModule
            {
                roomID = id,
                roomType = type,
                maxDoorways = type == RoomType.MaintenanceTunnel ? 2 : 4,
                hasEmergencyLighting = (type == RoomType.ControlRoom || type == RoomType.PressureChamber || type == RoomType.Airlock)
            };

            // Set room size based on type
            switch (type)
            {
                case RoomType.ProcessingHall: module.roomSize = new Vector3(15, 5, 12); break;
                case RoomType.ServerCrypt: module.roomSize = new Vector3(10, 3, 8); break;
                case RoomType.MaintenanceTunnel: module.roomSize = new Vector3(20, 2.5f, 3); break;
                case RoomType.PressureChamber: module.roomSize = new Vector3(6, 6, 6); break;
                case RoomType.ObservationDome: module.roomSize = new Vector3(12, 8, 12); break;
                case RoomType.ControlRoom: module.roomSize = new Vector3(8, 4, 8); break;
                default: module.roomSize = new Vector3(10, 4, 10); break;
            }

            // Generate spawn points
            int lootCount = RandomRange(1, 4);
            for (int j = 0; j < lootCount; j++)
            {
                module.lootSpawnPoints.Add(pos + new Vector3(
                    RandomRange(-module.roomSize.x * 0.3f, module.roomSize.x * 0.3f),
                    0f,
                    RandomRange(-module.roomSize.z * 0.3f, module.roomSize.z * 0.3f)));
            }

            int entityCount = RandomRange(0, 2);
            for (int j = 0; j < entityCount; j++)
            {
                module.entitySpawnPoints.Add(pos + new Vector3(
                    RandomRange(-module.roomSize.x * 0.4f, module.roomSize.x * 0.4f),
                    0f,
                    RandomRange(-module.roomSize.z * 0.4f, module.roomSize.z * 0.4f)));
            }

            totalLightCount += module.hasEmergencyLighting ? 4 : 1;

            return new RoomNode
            {
                roomID = id,
                module = module,
                worldPosition = pos,
                depthFromAirlock = depth
            };
        }

        // ====================================================================
        // LOOT & ENTITIES
        // ====================================================================

        private void DistributeLoot(MissionType missionType)
        {
            coreSpawnPoints.Clear();
            foreach (var node in roomGraph.Values)
            {
                foreach (var sp in node.module.lootSpawnPoints)
                {
                    if (node.depthFromAirlock > 3 || rng.NextDouble() < 0.5)
                        coreSpawnPoints.Add(sp);
                }
            }
        }

        private void DistributeEntitySpawns()
        {
            entitySpawnPoints.Clear();
            foreach (var node in roomGraph.Values)
            {
                entitySpawnPoints.AddRange(node.module.entitySpawnPoints);
            }
        }

        // ====================================================================
        // VALIDATION
        // ====================================================================

        private bool ValidateLayout()
        {
            if (!roomGraph.ContainsKey(airlockRoomID)) return false;

            var visited = new HashSet<string>();
            var queue = new Queue<string>();
            queue.Enqueue(airlockRoomID);
            visited.Add(airlockRoomID);

            while (queue.Count > 0)
            {
                string current = queue.Dequeue();
                foreach (string connected in roomGraph[current].connectedRoomIDs)
                {
                    if (!visited.Contains(connected) && roomGraph.ContainsKey(connected))
                    {
                        visited.Add(connected);
                        queue.Enqueue(connected);
                    }
                }
            }

            return visited.Count == roomGraph.Count;
        }

        // ====================================================================
        // GEOMETRY SPAWNING
        // ====================================================================

        private void SpawnRoomGeometry()
        {
            foreach (var node in roomGraph.Values)
            {
                // Create room with primitive geometry as placeholder
                GameObject room = CreateProceduralRoom(node);
                node.gameObject = room;
                spawnedRoomObjects.Add(room);

                // Spawn corridor connections
                foreach (string connectedID in node.connectedRoomIDs)
                {
                    if (roomGraph.ContainsKey(connectedID))
                    {
                        var other = roomGraph[connectedID];
                        // Only create corridor from lower-depth to higher-depth to avoid duplicates
                        if (node.depthFromAirlock < other.depthFromAirlock)
                        {
                            GameObject corridor = CreateCorridor(node.worldPosition, other.worldPosition);
                            spawnedRoomObjects.Add(corridor);
                        }
                    }
                }
            }
        }

        private GameObject CreateProceduralRoom(RoomNode node)
        {
            GameObject room = new GameObject($"Room_{node.roomID}");
            room.transform.position = node.worldPosition;

            Vector3 size = node.module.roomSize;

            // Floor
            var floor = GameObject.CreatePrimitive(PrimitiveType.Cube);
            floor.transform.SetParent(room.transform);
            floor.transform.localPosition = new Vector3(0, -0.1f, 0);
            floor.transform.localScale = new Vector3(size.x, 0.2f, size.z);
            floor.name = "Floor";

            // Ceiling
            var ceiling = GameObject.CreatePrimitive(PrimitiveType.Cube);
            ceiling.transform.SetParent(room.transform);
            ceiling.transform.localPosition = new Vector3(0, size.y, 0);
            ceiling.transform.localScale = new Vector3(size.x, 0.2f, size.z);
            ceiling.name = "Ceiling";

            // Walls
            CreateWall(room.transform, new Vector3(size.x / 2, size.y / 2, 0), new Vector3(0.2f, size.y, size.z), "WallEast");
            CreateWall(room.transform, new Vector3(-size.x / 2, size.y / 2, 0), new Vector3(0.2f, size.y, size.z), "WallWest");
            CreateWall(room.transform, new Vector3(0, size.y / 2, size.z / 2), new Vector3(size.x, size.y, 0.2f), "WallNorth");
            CreateWall(room.transform, new Vector3(0, size.y / 2, -size.z / 2), new Vector3(size.x, size.y, 0.2f), "WallSouth");

            // Emergency lighting
            if (node.module.hasEmergencyLighting)
            {
                var lightObj = new GameObject("EmergencyLight");
                lightObj.transform.SetParent(room.transform);
                lightObj.transform.localPosition = new Vector3(0, size.y - 0.5f, 0);
                var light = lightObj.AddComponent<Light>();
                light.type = LightType.Point;
                light.color = new Color(1f, 0.3f, 0.1f); // Warm orange emergency
                light.intensity = 0.5f;
                light.range = Mathf.Max(size.x, size.z) * 0.8f;
            }

            // Set layer for interaction
            room.layer = LayerMask.NameToLayer("Default");

            return room;
        }

        private void CreateWall(Transform parent, Vector3 localPos, Vector3 scale, string name)
        {
            var wall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wall.transform.SetParent(parent);
            wall.transform.localPosition = localPos;
            wall.transform.localScale = scale;
            wall.name = name;
        }

        private GameObject CreateCorridor(Vector3 from, Vector3 to)
        {
            GameObject corridor = new GameObject("Corridor");
            Vector3 midpoint = (from + to) / 2f;
            corridor.transform.position = midpoint;

            float length = Vector3.Distance(from, to);
            float width = 3f;
            float height = 3f;

            // Floor
            var floor = GameObject.CreatePrimitive(PrimitiveType.Cube);
            floor.transform.SetParent(corridor.transform);
            floor.transform.localScale = new Vector3(width, 0.2f, length);
            floor.transform.localPosition = new Vector3(0, -0.1f, 0);

            // Orient toward target
            corridor.transform.LookAt(to);

            return corridor;
        }

        // ====================================================================
        // QUERIES
        // ====================================================================

        public Vector3 GetAirlockPosition()
        {
            return roomGraph.ContainsKey(airlockRoomID)
                ? roomGraph[airlockRoomID].worldPosition
                : Vector3.zero;
        }

        public List<Vector3> GetCoreSpawnPoints() => coreSpawnPoints;
        public List<Vector3> GetEntitySpawnPoints() => entitySpawnPoints;
        public List<RoomModule> GetRooms() => generatedRooms;

        public string GetNearestRoomID(Vector3 position)
        {
            string nearest = "";
            float nearestDist = float.MaxValue;
            foreach (var node in roomGraph.Values)
            {
                float dist = Vector3.Distance(position, node.worldPosition);
                if (dist < nearestDist)
                {
                    nearestDist = dist;
                    nearest = node.roomID;
                }
            }
            return nearest;
        }

        public bool IsRoomModified(string roomID)
        {
            return roomGraph.ContainsKey(roomID) && roomGraph[roomID].isModifiedByArchitect;
        }

        // ====================================================================
        // ARCHITECT MANIPULATION
        // ====================================================================

        public void CreateSpatialLoop(string roomID)
        {
            if (!roomGraph.ContainsKey(roomID)) return;
            var room = roomGraph[roomID];
            room.connectedRoomIDs.Add(roomID); // Self-loop
            room.isModifiedByArchitect = true;
            Debug.LogWarning($"[Architect] Spatial loop: room {roomID}");
        }

        public void SealDoorway(string roomID, int doorwayIndex)
        {
            if (!roomGraph.ContainsKey(roomID)) return;
            var room = roomGraph[roomID];
            if (doorwayIndex < room.connectedRoomIDs.Count)
            {
                string disconnected = room.connectedRoomIDs[doorwayIndex];
                room.connectedRoomIDs.RemoveAt(doorwayIndex);
                if (roomGraph.ContainsKey(disconnected))
                    roomGraph[disconnected].connectedRoomIDs.Remove(roomID);
                room.isModifiedByArchitect = true;
            }
        }

        public void ExtendAirlockPath()
        {
            // Insert an extra room between the last main-branch room and airlock
            Debug.LogWarning("[Architect] Extending path to airlock");
        }

        public void FailRandomLight()
        {
            if (totalLightCount <= 0) return;
            int idx = Random.Range(0, totalLightCount);
            if (!failedLights.Contains(idx))
            {
                failedLights.Add(idx);
                // Find and disable corresponding light in scene
                Debug.Log($"[Station] Light {idx} failed ({failedLights.Count}/{totalLightCount})");
            }
        }

        // ====================================================================
        // HELPERS
        // ====================================================================

        private RoomType SelectRoomType(int depth)
        {
            RoomType[] shallow = { RoomType.ControlRoom, RoomType.Junction, RoomType.Corridor };
            RoomType[] mid = { RoomType.ProcessingHall, RoomType.PressureChamber, RoomType.Junction, RoomType.MaintenanceTunnel };
            RoomType[] deep = { RoomType.ServerCrypt, RoomType.ObservationDome, RoomType.MaintenanceTunnel, RoomType.ProcessingHall };

            RoomType[] pool;
            if (depth <= 3) pool = shallow;
            else if (depth <= 7) pool = mid;
            else pool = deep;

            return pool[RandomRange(0, pool.Length)];
        }

        private int RandomRange(int min, int max) => min + (int)(rng.NextDouble() * (max - min));
        private float RandomRange(float min, float max) => min + (float)(rng.NextDouble() * (max - min));
    }
}
