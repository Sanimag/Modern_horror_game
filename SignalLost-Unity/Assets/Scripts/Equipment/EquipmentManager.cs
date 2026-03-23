using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Equipment manager component for player characters.
    /// Manages inventory, active equipment, battery/charges, durability,
    /// and corruption-based failure states.
    /// Design philosophy: tools solve problems but always create new ones.
    /// </summary>
    public class EquipmentManager : MonoBehaviour
    {
        [Header("Inventory")]
        [SerializeField] private List<EquipmentData> inventory = new List<EquipmentData>();
        [SerializeField] private int activeSlot = -1;
        [SerializeField] private int maxSlots = 4;

        [Header("State")]
        [SerializeField] private bool isJammed = false;
        [SerializeField] private float failureRate = 0f;

        [Header("Flashlight")]
        [SerializeField] private bool flashlightOn = false;
        [SerializeField] private float flashlightDrainRate = 2f; // % per second

        [Header("Resync")]
        [SerializeField] private bool isResyncing = false;
        [SerializeField] private float resyncProgress = 0f;
        [SerializeField] private float resyncDuration = 5f;
        private PlayerCharacter resyncTarget;

        // Properties
        public List<EquipmentData> Inventory => inventory;
        public int ActiveSlot => activeSlot;
        public bool IsJammed => isJammed;
        public bool IsResyncing => isResyncing;
        public float ResyncProgress => resyncProgress;
        public bool FlashlightOn => flashlightOn;

        public EquipmentData ActiveEquipment =>
            (activeSlot >= 0 && activeSlot < inventory.Count) ? inventory[activeSlot] : null;

        private void Update()
        {
            TickBatteryDrain(Time.deltaTime);
            TickFailureRate();

            if (isResyncing)
                TickResync(Time.deltaTime);

            // Check for Broadcast jam
            CheckJamStatus();
        }

        // ====================================================================
        // INVENTORY
        // ====================================================================

        public bool AddEquipment(EquipmentData equip)
        {
            if (inventory.Count >= maxSlots) return false;
            inventory.Add(equip);
            if (activeSlot < 0) activeSlot = 0;
            return true;
        }

        public bool RemoveEquipment(int index)
        {
            if (index < 0 || index >= inventory.Count) return false;
            inventory.RemoveAt(index);
            if (activeSlot >= inventory.Count) activeSlot = inventory.Count - 1;
            return true;
        }

        public void SwitchSlot(int index)
        {
            if (index >= 0 && index < inventory.Count)
                activeSlot = index;
        }

        public bool HasEquipment(EquipmentType type)
        {
            return inventory.Exists(e => e.type == type);
        }

        // ====================================================================
        // USAGE
        // ====================================================================

        public bool UseActiveEquipment()
        {
            if (ActiveEquipment == null || isJammed) return false;
            if (IsEquipmentMalfunctioning())
            {
                Debug.LogWarning("[Equipment] Malfunction! Equipment failed to activate.");
                return false;
            }

            var equip = ActiveEquipment;
            switch (equip.type)
            {
                case EquipmentType.Flashlight:
                    ToggleFlashlight();
                    break;
                case EquipmentType.PulseRadar:
                    PulseRadarScan();
                    break;
                case EquipmentType.SignalFlareGun:
                    FireSignalFlare();
                    break;
                case EquipmentType.NoiseMaker:
                    ThrowNoiseMaker();
                    break;
                case EquipmentType.SignalJammer:
                    ActivateSignalJammer();
                    break;
                case EquipmentType.GlowStick:
                    DropGlowStick();
                    break;
                case EquipmentType.EmergencyBeacon:
                    ActivateEmergencyBeacon();
                    break;
                case EquipmentType.ResyncKit:
                    TryStartResync();
                    break;
                default:
                    return false;
            }
            return true;
        }

        private bool ConsumeCharge()
        {
            if (ActiveEquipment == null) return false;
            if (ActiveEquipment.maxCharges < 0) return true; // Battery-based

            if (ActiveEquipment.currentCharges <= 0) return false;
            ActiveEquipment.currentCharges--;

            if (ActiveEquipment.currentCharges <= 0)
                RemoveEquipment(activeSlot);

            return true;
        }

        public bool IsEquipmentMalfunctioning()
        {
            return failureRate > 0f && Random.value < failureRate;
        }

        // ====================================================================
        // SPECIFIC EQUIPMENT
        // ====================================================================

        public void ToggleFlashlight()
        {
            flashlightOn = !flashlightOn;
            var player = GetComponent<PlayerCharacter>();
            // Toggle the physical flashlight light component via PlayerCharacter
        }

        public void PulseRadarScan()
        {
            // Scan for entities and rooms within range
            float range = 20f;
            var entities = GameManager.Instance?.ActiveEntities;
            if (entities == null) return;

            int detected = 0;
            foreach (var entity in entities)
            {
                if (entity == null) continue;
                float dist = Vector3.Distance(transform.position, entity.transform.position);
                if (dist < range)
                {
                    detected++;
                    // Show on scanner UI
                }
            }

            // At high corruption, show false positives
            var corruption = CorruptionSystem.Instance;
            if (corruption != null && corruption.CorruptionIndex > 50f)
            {
                int falsePositives = Random.Range(0, 3);
                detected += falsePositives;
            }

            Debug.Log($"[Scanner] Pulse scan: {detected} contacts detected");
        }

        public void FireSignalFlare()
        {
            if (!ConsumeCharge()) return;

            // Stun all entities within radius for 4 seconds
            float stunRadius = 15f;
            foreach (var entity in GameManager.Instance.ActiveEntities)
            {
                if (entity == null) continue;
                float dist = Vector3.Distance(transform.position, entity.transform.position);
                if (dist < stunRadius)
                {
                    entity.Stun(4f);
                }
            }
            Debug.Log("[Equipment] Signal flare fired - entities stunned for 4 seconds");
        }

        public void ThrowNoiseMaker()
        {
            if (!ConsumeCharge()) return;
            // Spawn noise maker that produces sound for 10 seconds
            Debug.Log("[Equipment] Noise maker thrown");
        }

        public void ActivateSignalJammer()
        {
            if (!ConsumeCharge()) return;
            // Create 10m dead zone for 30 seconds (blocks entities AND player equipment)
            Debug.Log("[Equipment] Signal jammer active - 30 second dead zone");
        }

        public void DropGlowStick()
        {
            if (!ConsumeCharge()) return;
            // Spawn permanent glow stick light
            GameObject glow = new GameObject("GlowStick");
            glow.transform.position = transform.position;
            var light = glow.AddComponent<Light>();
            light.color = new Color(0.2f, 1f, 0.3f); // Green glow
            light.intensity = 1.5f;
            light.range = 8f;
            Debug.Log("[Equipment] Glow stick dropped");
        }

        public void ActivateEmergencyBeacon()
        {
            if (!ConsumeCharge()) return;
            // Reveal airlock direction + alert all entities
            Debug.LogWarning("[Equipment] EMERGENCY BEACON - Airlock revealed, entities alerted!");
        }

        // ====================================================================
        // RESYNC KIT
        // ====================================================================

        private void TryStartResync()
        {
            var player = GetComponent<PlayerCharacter>();
            if (player == null) return;

            // Find nearest downed or drifting player
            PlayerCharacter target = null;
            float nearestDist = 3f;

            var allPlayers = GameManager.Instance?.GetAlivePlayers();
            if (allPlayers == null) return;

            // Also check downed players
            foreach (var other in GameManager.Instance.GetAlivePlayers())
            {
                if (other == player) continue;
                float dist = Vector3.Distance(transform.position, other.transform.position);
                if (dist < nearestDist)
                {
                    if (other.CurrentState == PlayerState.SignalFade || other.IsPerceptionCompromised)
                    {
                        nearestDist = dist;
                        target = other;
                    }
                }
            }

            if (target != null)
                StartResync(target);
        }

        public void StartResync(PlayerCharacter target)
        {
            isResyncing = true;
            resyncTarget = target;
            resyncProgress = 0f;
        }

        public void CancelResync()
        {
            isResyncing = false;
            resyncTarget = null;
            resyncProgress = 0f;
            ConsumeCharge(); // Kit consumed even if interrupted
        }

        private void TickResync(float deltaTime)
        {
            if (resyncTarget == null) { CancelResync(); return; }

            float dist = Vector3.Distance(transform.position, resyncTarget.transform.position);
            if (dist > 3f) { CancelResync(); return; }

            resyncProgress += deltaTime / resyncDuration;

            if (resyncProgress >= 1f)
            {
                if (resyncTarget.CurrentState == PlayerState.SignalFade)
                    resyncTarget.Revive(50f);
                else
                    resyncTarget.ResetPerceptionDrift();

                ConsumeCharge();
                isResyncing = false;
                resyncTarget = null;
                resyncProgress = 0f;
                Debug.Log("[Equipment] Resync complete");
            }
        }

        // ====================================================================
        // PASSIVE TICKS
        // ====================================================================

        private void TickBatteryDrain(float deltaTime)
        {
            if (!flashlightOn || ActiveEquipment == null) return;
            if (ActiveEquipment.type != EquipmentType.Flashlight) return;

            ActiveEquipment.batteryLife -= flashlightDrainRate * deltaTime;
            if (ActiveEquipment.batteryLife <= 0f)
            {
                ActiveEquipment.batteryLife = 0f;
                flashlightOn = false;
            }
        }

        private void TickFailureRate()
        {
            var corruption = CorruptionSystem.Instance;
            if (corruption == null) return;
            failureRate = corruption.GetEquipmentFailureRate();
        }

        private void CheckJamStatus()
        {
            isJammed = false;
            // Check for Broadcast entity jam radius
            if (GameManager.Instance == null) return;
            foreach (var entity in GameManager.Instance.ActiveEntities)
            {
                var broadcast = entity as EntityBroadcast;
                if (broadcast != null && broadcast.IsLocationJammed(transform.position))
                {
                    isJammed = true;
                    break;
                }
            }
        }

        /// <summary>Repair equipment (Technician ability, once per run)</summary>
        public bool RepairEquipment(int index)
        {
            var player = GetComponent<PlayerCharacter>();
            if (player == null || !player.Passives.canRepairEquipment) return false;
            if (index < 0 || index >= inventory.Count) return false;

            inventory[index].durability = 100f;
            inventory[index].batteryLife = 100f;
            // Disable repair for rest of run
            Debug.Log("[Equipment] Repaired (Technician ability used)");
            return true;
        }
    }
}
