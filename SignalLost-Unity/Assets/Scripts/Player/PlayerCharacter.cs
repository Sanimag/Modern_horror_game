using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Player character for Signal Lost.
    /// Handles movement, role abilities, health, perception drift,
    /// signal core carrying, and all player interactions.
    /// </summary>
    [RequireComponent(typeof(CharacterController))]
    [RequireComponent(typeof(EquipmentManager))]
    public class PlayerCharacter : MonoBehaviour
    {
        [Header("Role")]
        [SerializeField] private PlayerRole currentRole = PlayerRole.None;
        private RolePassives passives;

        [Header("Health")]
        [SerializeField] private float health = 100f;
        [SerializeField] private float maxHealth = 100f;
        [SerializeField] private PlayerState currentState = PlayerState.Alive;
        [SerializeField] private float signalFadeTimer = 60f;
        private const float SIGNAL_FADE_DURATION = 60f;

        [Header("Perception Drift")]
        [SerializeField] private float perceptionDrift = 0f;
        private const float ISOLATION_DRIFT_RATE = 0.5f;

        [Header("Movement")]
        [SerializeField] private float baseWalkSpeed = 5f;
        [SerializeField] private float crouchSpeed = 2f;
        [SerializeField] private float sprintSpeed = 8f;
        [SerializeField] private float mouseSensitivity = 2f;
        private bool isSprinting = false;
        private bool isCrouching = false;
        private float verticalRotation = 0f;

        [Header("Carrying")]
        [SerializeField] private List<SignalCore> carriedCores = new List<SignalCore>();

        [Header("Interaction")]
        [SerializeField] private float interactRange = 3f;
        [SerializeField] private LayerMask interactMask;

        [Header("References")]
        [SerializeField] private Transform cameraTransform;
        [SerializeField] private Light flashlight;

        // Components
        private CharacterController controller;
        private EquipmentManager equipment;

        // Properties
        public PlayerRole CurrentRole => currentRole;
        public RolePassives Passives => passives;
        public float Health => health;
        public float MaxHealth => maxHealth;
        public PlayerState CurrentState => currentState;
        public float SignalFadeTimer => signalFadeTimer;
        public float PerceptionDrift => perceptionDrift;
        public List<SignalCore> CarriedCores => carriedCores;
        public bool IsCarryingCore => carriedCores.Count > 0;
        public bool IsSprinting => isSprinting;
        public bool IsCrouching => isCrouching;
        public int MaxCarryCores => passives?.maxCarryCores ?? 1;
        public EquipmentManager Equipment => equipment;
        public Transform CameraTransform => cameraTransform;

        private void Awake()
        {
            controller = GetComponent<CharacterController>();
            equipment = GetComponent<EquipmentManager>();
            passives = new RolePassives();
        }

        private void Start()
        {
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
            GameManager.Instance?.RegisterPlayer(this);
        }

        private void OnDestroy()
        {
            GameManager.Instance?.UnregisterPlayer(this);
        }

        private void Update()
        {
            if (currentState != PlayerState.Alive)
            {
                if (currentState == PlayerState.SignalFade)
                    TickSignalFade(Time.deltaTime);
                return;
            }

            HandleMouseLook();
            HandleMovement();
            HandleInput();
            TickPerceptionDrift(Time.deltaTime);
        }

        // ====================================================================
        // ROLE SYSTEM
        // ====================================================================

        public void SetRole(PlayerRole role)
        {
            currentRole = role;
            passives = RolePassives.ForRole(role);
            Debug.Log($"[Player] Role set to: {role}");
        }

        // ====================================================================
        // MOVEMENT
        // ====================================================================

        private void HandleMouseLook()
        {
            float mouseX = Input.GetAxis("Mouse X") * mouseSensitivity;
            float mouseY = Input.GetAxis("Mouse Y") * mouseSensitivity;

            transform.Rotate(Vector3.up * mouseX);
            verticalRotation -= mouseY;
            verticalRotation = Mathf.Clamp(verticalRotation, -90f, 90f);

            if (cameraTransform)
                cameraTransform.localRotation = Quaternion.Euler(verticalRotation, 0f, 0f);
        }

        private void HandleMovement()
        {
            float moveX = Input.GetAxis("Horizontal");
            float moveZ = Input.GetAxis("Vertical");

            Vector3 move = transform.right * moveX + transform.forward * moveZ;

            float speed = GetCurrentMoveSpeed();
            controller.Move(move * speed * Time.deltaTime);

            // Gravity
            if (!controller.isGrounded)
                controller.Move(Vector3.down * 9.81f * Time.deltaTime);
        }

        private float GetCurrentMoveSpeed()
        {
            float baseSpeed;
            if (isCrouching) baseSpeed = crouchSpeed;
            else if (isSprinting) baseSpeed = sprintSpeed;
            else baseSpeed = baseWalkSpeed;

            return baseSpeed * GetCarrySpeedMultiplier();
        }

        public float GetCarrySpeedMultiplier()
        {
            if (!IsCarryingCore) return 1f;

            float weightPenalty = 0f;
            foreach (var core in carriedCores)
                weightPenalty += core.weight * 0.15f;

            float multiplier = (1f - weightPenalty) * (passives?.carrySpeedMultiplier ?? 1f);
            return Mathf.Max(multiplier, 0.4f);
        }

        /// <summary>Noise level: 0 = silent (crouching), 0.5 = walking, 1.0 = sprinting</summary>
        public float GetNoiseLevel()
        {
            if (isCrouching) return 0f;
            if (isSprinting) return 1f;
            return 0.5f;
        }

        // ====================================================================
        // INPUT
        // ====================================================================

        private void HandleInput()
        {
            // Sprint
            if (Input.GetKeyDown(KeyCode.LeftShift) && !isCrouching)
                isSprinting = true;
            if (Input.GetKeyUp(KeyCode.LeftShift))
                isSprinting = false;

            // Crouch (toggle)
            if (Input.GetKeyDown(KeyCode.LeftControl))
            {
                isCrouching = !isCrouching;
                isSprinting = false;
                // Adjust controller height
                controller.height = isCrouching ? 1f : 2f;
                controller.center = new Vector3(0, isCrouching ? 0.5f : 1f, 0);
            }

            // Interact
            if (Input.GetKeyDown(KeyCode.E))
                TryInteract();

            // Drop core
            if (Input.GetKeyDown(KeyCode.G) && IsCarryingCore)
                DropCore(0);

            // Use equipment
            if (Input.GetMouseButtonDown(0))
                equipment?.UseActiveEquipment();

            // Flashlight toggle
            if (Input.GetKeyDown(KeyCode.F))
                ToggleFlashlight();

            // Equipment slots
            if (Input.GetKeyDown(KeyCode.Alpha1)) equipment?.SwitchSlot(0);
            if (Input.GetKeyDown(KeyCode.Alpha2)) equipment?.SwitchSlot(1);
            if (Input.GetKeyDown(KeyCode.Alpha3)) equipment?.SwitchSlot(2);
            if (Input.GetKeyDown(KeyCode.Alpha4)) equipment?.SwitchSlot(3);

            // Wrist device
            if (Input.GetKeyDown(KeyCode.Tab))
                SignalLostHUD.Instance?.ToggleWristDevice();

            // Broadcast sacrifice (only when in Signal Fade)
            if (Input.GetKeyDown(KeyCode.F) && currentState == PlayerState.SignalFade)
                BroadcastSacrifice();
        }

        private void TryInteract()
        {
            if (!cameraTransform) return;

            if (Physics.Raycast(cameraTransform.position, cameraTransform.forward,
                out RaycastHit hit, interactRange, interactMask))
            {
                // Check for signal core
                var core = hit.collider.GetComponent<SignalCorePickup>();
                if (core != null)
                {
                    PickUpCore(core.CoreData);
                    Destroy(core.gameObject);
                    return;
                }

                // Check for mimic
                var mimic = hit.collider.GetComponent<EntityMimic>();
                if (mimic != null && mimic.IsDisguised)
                {
                    mimic.OnPlayerInteract(this);
                    return;
                }

                // Check for extraction point
                var extractionPoint = hit.collider.GetComponent<ExtractionPoint>();
                if (extractionPoint != null)
                {
                    MissionManager.Instance?.BeginCoreExtraction(extractionPoint.CoreData, this);
                    return;
                }

                // Check for other interactables
                var interactable = hit.collider.GetComponent<IInteractable>();
                interactable?.Interact(this);
            }
        }

        private void ToggleFlashlight()
        {
            if (flashlight != null)
                flashlight.enabled = !flashlight.enabled;
        }

        // ====================================================================
        // HEALTH & STATE
        // ====================================================================

        public void TakeDamage(float amount, EntityType source)
        {
            if (currentState != PlayerState.Alive) return;

            float adjusted = amount * (passives?.corruptionDamageMultiplier ?? 1f);
            health = Mathf.Clamp(health - adjusted, 0f, maxHealth);

            AddPerceptionDrift(amount * 0.3f);

            if (health <= 0f)
                EnterSignalFade();

            Debug.Log($"[Player] Took {adjusted:F1} damage from {source}. Health: {health:F1}");
        }

        public void Heal(float amount)
        {
            health = Mathf.Clamp(health + amount, 0f, maxHealth);
        }

        public void EnterSignalFade()
        {
            currentState = PlayerState.SignalFade;
            signalFadeTimer = SIGNAL_FADE_DURATION;
            DropAllCores();
            GameEvents.RaisePlayerStateChanged(PlayerState.SignalFade);
            Debug.LogWarning("[Player] Entered Signal Fade - 60 seconds to revive");
        }

        public void Revive(float healthAmount = 50f)
        {
            if (currentState != PlayerState.SignalFade) return;
            currentState = PlayerState.Alive;
            health = healthAmount;
            signalFadeTimer = SIGNAL_FADE_DURATION;
            GameEvents.RaisePlayerStateChanged(PlayerState.Alive);
            Debug.Log($"[Player] Revived with {healthAmount} health");
        }

        public void BroadcastSacrifice()
        {
            if (currentState != PlayerState.SignalFade) return;
            currentState = PlayerState.Broadcasting;
            GameEvents.RaisePlayerStateChanged(PlayerState.Broadcasting);

            // Reveal all entities for 15 seconds
            var positions = GameManager.Instance?.RevealAllEntityPositions();
            Debug.LogWarning($"[Player] BROADCAST SACRIFICE - {positions?.Count ?? 0} entities revealed!");

            // Die after 15 seconds
            Invoke(nameof(Die), 15f);
        }

        private void Die()
        {
            currentState = PlayerState.Dead;
            GameEvents.RaisePlayerStateChanged(PlayerState.Dead);
            Debug.LogError("[Player] Permanent death");
        }

        private void TickSignalFade(float deltaTime)
        {
            signalFadeTimer -= deltaTime;
            if (signalFadeTimer <= 0f)
            {
                Die();
            }
        }

        // ====================================================================
        // PERCEPTION DRIFT
        // ====================================================================

        public void AddPerceptionDrift(float amount)
        {
            float bonus = IsIsolated() ? amount * 0.5f : 0f;
            perceptionDrift = Mathf.Clamp(perceptionDrift + amount + bonus, 0f, 100f);
            GameEvents.RaisePerceptionDrift(perceptionDrift);
        }

        public void ResetPerceptionDrift()
        {
            perceptionDrift = 0f;
            GameEvents.RaisePerceptionDrift(0f);
        }

        public bool IsPerceptionCompromised => perceptionDrift > 40f;

        /// <summary>Flashlight flicker visible to other players indicating drift</summary>
        public float GetDriftFlickerIntensity()
        {
            if (perceptionDrift < 30f) return 0f;
            return Mathf.Lerp(0f, 1f, (perceptionDrift - 30f) / 70f);
        }

        private void TickPerceptionDrift(float deltaTime)
        {
            if (IsIsolated() && currentState == PlayerState.Alive)
            {
                AddPerceptionDrift(deltaTime * ISOLATION_DRIFT_RATE);
            }
        }

        // ====================================================================
        // CARRYING
        // ====================================================================

        public bool PickUpCore(SignalCore core)
        {
            if (carriedCores.Count >= (passives?.maxCarryCores ?? 1)) return false;
            if (currentState != PlayerState.Alive) return false;

            carriedCores.Add(core);
            Debug.Log($"[Player] Picked up core worth {core.creditValue} credits");
            return true;
        }

        public SignalCore DropCore(int index)
        {
            if (index < 0 || index >= carriedCores.Count) return null;
            var core = carriedCores[index];
            carriedCores.RemoveAt(index);
            // Spawn dropped core in world
            return core;
        }

        public void DropAllCores()
        {
            carriedCores.Clear();
        }

        // ====================================================================
        // LOCATION CHECKS
        // ====================================================================

        public bool IsAtAirlock()
        {
            if (GameManager.Instance == null) return false;
            return Vector3.Distance(transform.position, GameManager.Instance.AirlockPosition) < 5f;
        }

        public bool IsIsolated()
        {
            return GetDistanceToNearestTeammate() > 15f;
        }

        public float GetDistanceToNearestTeammate()
        {
            float nearest = float.MaxValue;
            var players = GameManager.Instance?.GetAlivePlayers();
            if (players == null) return nearest;

            foreach (var other in players)
            {
                if (other == this) continue;
                float dist = Vector3.Distance(transform.position, other.transform.position);
                if (dist < nearest) nearest = dist;
            }
            return nearest;
        }
    }

    // ========================================================================
    // HELPER INTERFACES & COMPONENTS
    // ========================================================================

    public interface IInteractable
    {
        void Interact(PlayerCharacter player);
        string GetInteractPrompt();
    }

    public class SignalCorePickup : MonoBehaviour
    {
        public SignalCore CoreData = new SignalCore();
    }

    public class ExtractionPoint : MonoBehaviour, IInteractable
    {
        public SignalCore CoreData = new SignalCore();
        public bool isExtracted = false;

        public void Interact(PlayerCharacter player)
        {
            if (!isExtracted)
                MissionManager.Instance?.BeginCoreExtraction(CoreData, player);
        }

        public string GetInteractPrompt() =>
            isExtracted ? "" : $"[E] Extract Core ({CoreData.creditValue} cr)";
    }
}
