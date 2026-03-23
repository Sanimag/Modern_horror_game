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

        private const float DOORWAY_WIDTH = 3f;
        private const float CORRIDOR_WIDTH = 3f;
        private const float CORRIDOR_HEIGHT = 3.5f;

        // Determine which wall side a connected room is on relative to this room
        private enum WallSide { East, West, North, South }

        private WallSide GetWallSide(Vector3 roomPos, Vector3 connectedPos)
        {
            Vector3 delta = connectedPos - roomPos;
            if (Mathf.Abs(delta.x) > Mathf.Abs(delta.z))
                return delta.x > 0 ? WallSide.East : WallSide.West;
            else
                return delta.z > 0 ? WallSide.North : WallSide.South;
        }

        private void SpawnRoomGeometry()
        {
            // Track which corridors we've already created (avoid duplicates)
            var createdCorridors = new HashSet<string>();

            foreach (var node in roomGraph.Values)
            {
                // Figure out which walls need doorways
                var doorwaySides = new HashSet<WallSide>();
                foreach (string connectedID in node.connectedRoomIDs)
                {
                    if (connectedID == node.roomID) continue; // Skip self-loops
                    if (roomGraph.ContainsKey(connectedID))
                    {
                        var other = roomGraph[connectedID];
                        doorwaySides.Add(GetWallSide(node.worldPosition, other.worldPosition));
                    }
                }

                GameObject room = CreateProceduralRoom(node, doorwaySides);
                node.gameObject = room;
                spawnedRoomObjects.Add(room);

                // Spawn corridor connections (only once per pair)
                foreach (string connectedID in node.connectedRoomIDs)
                {
                    if (connectedID == node.roomID) continue;
                    if (!roomGraph.ContainsKey(connectedID)) continue;

                    string pairKey = string.Compare(node.roomID, connectedID) < 0
                        ? $"{node.roomID}_{connectedID}" : $"{connectedID}_{node.roomID}";

                    if (!createdCorridors.Contains(pairKey))
                    {
                        createdCorridors.Add(pairKey);
                        var other = roomGraph[connectedID];
                        GameObject corridor = CreateCorridor(
                            node.worldPosition, node.module.roomSize,
                            other.worldPosition, other.module.roomSize);
                        spawnedRoomObjects.Add(corridor);
                    }
                }
            }
        }

        private GameObject CreateProceduralRoom(RoomNode node, HashSet<WallSide> doorwaySides)
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
            ceiling.transform.localScale = new Vector3(size.x + 0.4f, 0.2f, size.z + 0.4f);
            ceiling.name = "Ceiling";

            // Walls with doorway gaps
            // East wall (X+)
            if (doorwaySides.Contains(WallSide.East))
                CreateWallWithDoorway(room.transform, size, WallSide.East);
            else
                CreateSolidWall(room.transform, new Vector3(size.x / 2, size.y / 2, 0), new Vector3(0.2f, size.y, size.z), "WallEast");

            // West wall (X-)
            if (doorwaySides.Contains(WallSide.West))
                CreateWallWithDoorway(room.transform, size, WallSide.West);
            else
                CreateSolidWall(room.transform, new Vector3(-size.x / 2, size.y / 2, 0), new Vector3(0.2f, size.y, size.z), "WallWest");

            // North wall (Z+)
            if (doorwaySides.Contains(WallSide.North))
                CreateWallWithDoorway(room.transform, size, WallSide.North);
            else
                CreateSolidWall(room.transform, new Vector3(0, size.y / 2, size.z / 2), new Vector3(size.x, size.y, 0.2f), "WallNorth");

            // South wall (Z-)
            if (doorwaySides.Contains(WallSide.South))
                CreateWallWithDoorway(room.transform, size, WallSide.South);
            else
                CreateSolidWall(room.transform, new Vector3(0, size.y / 2, -size.z / 2), new Vector3(size.x, size.y, 0.2f), "WallSouth");

            // Emergency lighting — dim point light on ceiling
            if (node.module.hasEmergencyLighting)
            {
                var lightObj = new GameObject("EmergencyLight");
                lightObj.transform.SetParent(room.transform);
                lightObj.transform.localPosition = new Vector3(0, size.y - 0.3f, 0);
                var light = lightObj.AddComponent<Light>();
                light.type = LightType.Point;
                light.color = new Color(1f, 0.4f, 0.15f);
                light.intensity = 0.3f;
                light.range = Mathf.Max(size.x, size.z) * 0.6f;
            }

            room.layer = LayerMask.NameToLayer("Default");
            return room;
        }

        private void CreateSolidWall(Transform parent, Vector3 localPos, Vector3 scale, string name)
        {
            var wall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            wall.transform.SetParent(parent);
            wall.transform.localPosition = localPos;
            wall.transform.localScale = scale;
            wall.name = name;
        }

        /// <summary>
        /// Creates a wall with a centered doorway gap.
        /// Splits the wall into: left segment, right segment, and a header above the doorway.
        /// </summary>
        private void CreateWallWithDoorway(Transform parent, Vector3 roomSize, WallSide side)
        {
            float h = roomSize.y;
            float doorH = Mathf.Min(CORRIDOR_HEIGHT, h - 0.3f);
            float doorW = DOORWAY_WIDTH;

            bool isXWall = (side == WallSide.East || side == WallSide.West);
            float wallLength = isXWall ? roomSize.z : roomSize.x;
            float wallThickness = 0.2f;

            float xSign = 0f;
            float zSign = 0f;
            if (side == WallSide.East) xSign = 1f;
            else if (side == WallSide.West) xSign = -1f;
            else if (side == WallSide.North) zSign = 1f;
            else if (side == WallSide.South) zSign = -1f;

            float wallOffset = isXWall ? roomSize.x / 2f : roomSize.z / 2f;

            // Left segment
            float leftLen = (wallLength - doorW) / 2f;
            if (leftLen > 0.1f)
            {
                Vector3 leftPos, leftScale;
                if (isXWall)
                {
                    leftPos = new Vector3(xSign * wallOffset, h / 2f, -(leftLen / 2f + doorW / 2f));
                    leftScale = new Vector3(wallThickness, h, leftLen);
                }
                else
                {
                    leftPos = new Vector3(-(leftLen / 2f + doorW / 2f), h / 2f, zSign * wallOffset);
                    leftScale = new Vector3(leftLen, h, wallThickness);
                }
                CreateSolidWall(parent, leftPos, leftScale, $"Wall{side}_Left");
            }

            // Right segment
            float rightLen = (wallLength - doorW) / 2f;
            if (rightLen > 0.1f)
            {
                Vector3 rightPos, rightScale;
                if (isXWall)
                {
                    rightPos = new Vector3(xSign * wallOffset, h / 2f, (rightLen / 2f + doorW / 2f));
                    rightScale = new Vector3(wallThickness, h, rightLen);
                }
                else
                {
                    rightPos = new Vector3((rightLen / 2f + doorW / 2f), h / 2f, zSign * wallOffset);
                    rightScale = new Vector3(rightLen, h, wallThickness);
                }
                CreateSolidWall(parent, rightPos, rightScale, $"Wall{side}_Right");
            }

            // Header above doorway
            float headerH = h - doorH;
            if (headerH > 0.1f)
            {
                Vector3 headerPos, headerScale;
                if (isXWall)
                {
                    headerPos = new Vector3(xSign * wallOffset, doorH + headerH / 2f, 0);
                    headerScale = new Vector3(wallThickness, headerH, doorW);
                }
                else
                {
                    headerPos = new Vector3(0, doorH + headerH / 2f, zSign * wallOffset);
                    headerScale = new Vector3(doorW, headerH, wallThickness);
                }
                CreateSolidWall(parent, headerPos, headerScale, $"Wall{side}_Header");
            }
        }

        /// <summary>
        /// Creates an enclosed corridor between two rooms.
        /// Corridor has floor, ceiling, and two side walls.
        /// Start/end points are at the room wall edges so it connects flush to doorways.
        /// </summary>
        private GameObject CreateCorridor(Vector3 fromPos, Vector3 fromSize, Vector3 toPos, Vector3 toSize)
        {
            GameObject corridor = new GameObject("Corridor");

            // Determine connection points at room edges
            Vector3 delta = toPos - fromPos;
            WallSide fromSide = GetWallSide(fromPos, toPos);
            WallSide toSide = GetWallSide(toPos, fromPos);

            Vector3 startPoint = GetWallEdgePoint(fromPos, fromSize, fromSide);
            Vector3 endPoint = GetWallEdgePoint(toPos, toSize, toSide);

            Vector3 midpoint = (startPoint + endPoint) / 2f;
            corridor.transform.position = midpoint;

            Vector3 direction = (endPoint - startPoint);
            float length = direction.magnitude;
            direction.Normalize();

            // Orient corridor along connection direction
            if (direction != Vector3.zero)
                corridor.transform.rotation = Quaternion.LookRotation(direction);

            float width = CORRIDOR_WIDTH;
            float height = CORRIDOR_HEIGHT;

            // Floor
            var floor = GameObject.CreatePrimitive(PrimitiveType.Cube);
            floor.transform.SetParent(corridor.transform);
            floor.transform.localPosition = new Vector3(0, -0.1f, 0);
            floor.transform.localScale = new Vector3(width, 0.2f, length);
            floor.name = "CorridorFloor";

            // Ceiling
            var ceilObj = GameObject.CreatePrimitive(PrimitiveType.Cube);
            ceilObj.transform.SetParent(corridor.transform);
            ceilObj.transform.localPosition = new Vector3(0, height, 0);
            ceilObj.transform.localScale = new Vector3(width + 0.4f, 0.2f, length);
            ceilObj.name = "CorridorCeiling";

            // Left wall
            var leftWall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            leftWall.transform.SetParent(corridor.transform);
            leftWall.transform.localPosition = new Vector3(-width / 2f, height / 2f, 0);
            leftWall.transform.localScale = new Vector3(0.2f, height, length);
            leftWall.name = "CorridorWallLeft";

            // Right wall
            var rightWall = GameObject.CreatePrimitive(PrimitiveType.Cube);
            rightWall.transform.SetParent(corridor.transform);
            rightWall.transform.localPosition = new Vector3(width / 2f, height / 2f, 0);
            rightWall.transform.localScale = new Vector3(0.2f, height, length);
            rightWall.name = "CorridorWallRight";

            // Dim corridor light at midpoint
            if (length > 5f)
            {
                var lightObj = new GameObject("CorridorLight");
                lightObj.transform.SetParent(corridor.transform);
                lightObj.transform.localPosition = new Vector3(0, height - 0.3f, 0);
                var light = lightObj.AddComponent<Light>();
                light.type = LightType.Point;
                light.color = new Color(0.6f, 0.8f, 1f);
                light.intensity = 0.15f;
                light.range = length * 0.5f;
            }

            return corridor;
        }

        /// <summary>Get the world position at the edge of a room's wall for corridor connection.</summary>
        private Vector3 GetWallEdgePoint(Vector3 roomPos, Vector3 roomSize, WallSide side)
        {
            switch (side)
            {
                case WallSide.East:  return roomPos + new Vector3(roomSize.x / 2f, 0, 0);
                case WallSide.West:  return roomPos + new Vector3(-roomSize.x / 2f, 0, 0);
                case WallSide.North: return roomPos + new Vector3(0, 0, roomSize.z / 2f);
                case WallSide.South: return roomPos + new Vector3(0, 0, -roomSize.z / 2f);
                default: return roomPos;
            }
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
