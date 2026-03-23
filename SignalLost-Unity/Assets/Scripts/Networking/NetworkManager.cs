using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Network manager for Signal Lost multiplayer.
    /// Designed for Unity Netcode for GameObjects or Mirror integration.
    /// Handles session management, player spawning, and state synchronization.
    /// For production: replace with actual Netcode/Mirror NetworkManager subclass.
    /// </summary>
    public class SLNetworkManager : MonoBehaviour
    {
        public static SLNetworkManager Instance { get; private set; }

        [Header("Settings")]
        [SerializeField] private int maxPlayers = 6;
        [SerializeField] private int minPlayers = 2;
        [SerializeField] private GameObject playerPrefab;

        [Header("Session State")]
        [SerializeField] private bool isHost = false;
        [SerializeField] private bool isConnected = false;
        [SerializeField] private string sessionID;
        [SerializeField] private List<PlayerInfo> connectedPlayers = new List<PlayerInfo>();

        [System.Serializable]
        public class PlayerInfo
        {
            public string playerID;
            public string displayName;
            public PlayerRole selectedRole;
            public bool isReady;
            public bool isHost;
        }

        // Properties
        public bool IsHost => isHost;
        public bool IsConnected => isConnected;
        public int MaxPlayers => maxPlayers;
        public int ConnectedPlayerCount => connectedPlayers.Count;
        public List<PlayerInfo> ConnectedPlayers => connectedPlayers;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        // ====================================================================
        // SESSION MANAGEMENT
        // ====================================================================

        public void HostSession(string playerName)
        {
            isHost = true;
            isConnected = true;
            sessionID = System.Guid.NewGuid().ToString().Substring(0, 8);

            var hostInfo = new PlayerInfo
            {
                playerID = "Host",
                displayName = playerName,
                isHost = true
            };
            connectedPlayers.Add(hostInfo);

            Debug.Log($"[Network] Hosting session: {sessionID}");
        }

        public void JoinSession(string hostAddress, string playerName)
        {
            isHost = false;
            isConnected = true;

            var playerInfo = new PlayerInfo
            {
                playerID = System.Guid.NewGuid().ToString().Substring(0, 8),
                displayName = playerName
            };
            connectedPlayers.Add(playerInfo);

            Debug.Log($"[Network] Joined session at {hostAddress}");
        }

        public void LeaveSession()
        {
            isConnected = false;
            connectedPlayers.Clear();
            Debug.Log("[Network] Left session");
        }

        // ====================================================================
        // ROLE SELECTION (Pre-mission lobby)
        // ====================================================================

        public void SelectRole(string playerID, PlayerRole role)
        {
            var player = connectedPlayers.Find(p => p.playerID == playerID);
            if (player != null)
            {
                player.selectedRole = role;
                Debug.Log($"[Network] {player.displayName} selected role: {role}");
            }
        }

        public void SetReady(string playerID, bool ready)
        {
            var player = connectedPlayers.Find(p => p.playerID == playerID);
            if (player != null)
            {
                player.isReady = ready;
            }
        }

        public bool AreAllPlayersReady()
        {
            return connectedPlayers.Count >= minPlayers &&
                   connectedPlayers.TrueForAll(p => p.isReady);
        }

        // ====================================================================
        // GAME STATE SYNC
        // ====================================================================

        /// <summary>Sync corruption index to all clients</summary>
        public void SyncCorruption(float value, CorruptionPhase phase)
        {
            // In production: NetworkVariable or RPC
            Debug.Log($"[Network] Syncing corruption: {value:F1}% ({phase})");
        }

        /// <summary>Sync entity spawn to all clients</summary>
        public void SyncEntitySpawn(EntityType type, Vector3 position)
        {
            Debug.Log($"[Network] Syncing entity spawn: {type} at {position}");
        }

        /// <summary>Sync player state change</summary>
        public void SyncPlayerState(string playerID, PlayerState state)
        {
            Debug.Log($"[Network] Syncing player state: {playerID} -> {state}");
        }

        /// <summary>Sync core extraction</summary>
        public void SyncCoreExtraction(string coreID, string playerID)
        {
            Debug.Log($"[Network] Syncing extraction: {coreID} by {playerID}");
        }

        /// <summary>Sync cascade trigger</summary>
        public void SyncCascade(float countdown)
        {
            Debug.Log($"[Network] Syncing CASCADE: {countdown}s");
        }
    }
}
