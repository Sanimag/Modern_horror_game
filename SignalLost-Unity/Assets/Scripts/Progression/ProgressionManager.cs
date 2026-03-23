using System;
using System.Collections.Generic;
using UnityEngine;

namespace SignalLost
{
    /// <summary>
    /// Save data for persistent progression.
    /// </summary>
    [Serializable]
    public class SaveData
    {
        public int companyRank = 1;
        public int totalCreditsEarned = 0;
        public int currentCredits = 0;
        public int totalMissionsCompleted = 0;
        public int totalMissionsFailed = 0;

        // Role mastery: serialized as parallel arrays
        public List<PlayerRole> masteryRoles = new List<PlayerRole>();
        public List<int> masteryXP = new List<int>();
        public List<int> masteryLevels = new List<int>();

        public List<SubmarineUpgrade> submarineUpgrades = new List<SubmarineUpgrade>();
        public List<string> unlockedCosmetics = new List<string>();
        public List<string> equipUpgradeIDs = new List<string>();
        public List<int> equipUpgradeLevels = new List<int>();

        public int GetMasteryXP(PlayerRole role)
        {
            int idx = masteryRoles.IndexOf(role);
            return idx >= 0 ? masteryXP[idx] : 0;
        }

        public int GetMasteryLevel(PlayerRole role)
        {
            int idx = masteryRoles.IndexOf(role);
            return idx >= 0 ? masteryLevels[idx] : 0;
        }

        public void SetMastery(PlayerRole role, int xp, int level)
        {
            int idx = masteryRoles.IndexOf(role);
            if (idx >= 0)
            {
                masteryXP[idx] = xp;
                masteryLevels[idx] = level;
            }
            else
            {
                masteryRoles.Add(role);
                masteryXP.Add(xp);
                masteryLevels.Add(level);
            }
        }
    }

    /// <summary>
    /// Progression Manager.
    /// Handles company rank, credits, role mastery, submarine upgrades,
    /// equipment shop, and all persistent progression.
    /// </summary>
    public class ProgressionManager : MonoBehaviour
    {
        public static ProgressionManager Instance { get; private set; }

        [SerializeField] private SaveData saveData = new SaveData();

        private const int BASE_CREDITS_PER_RANK = 1000;
        private const float RANK_SCALING = 1.5f;
        private const int MASTERY_XP_PER_LEVEL = 500;
        private const int MAX_MASTERY_LEVEL = 10;
        private const string SAVE_KEY = "SignalLostSave";

        // Properties
        public int CompanyRank => saveData.companyRank;
        public int CurrentCredits => saveData.currentCredits;
        public int TotalMissions => saveData.totalMissionsCompleted + saveData.totalMissionsFailed;
        public SaveData Data => saveData;

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
            LoadProgress();
        }

        // ====================================================================
        // SAVE / LOAD
        // ====================================================================

        public void SaveProgress()
        {
            string json = JsonUtility.ToJson(saveData);
            PlayerPrefs.SetString(SAVE_KEY, json);
            PlayerPrefs.Save();
            Debug.Log($"[Save] Rank: {saveData.companyRank}, Credits: {saveData.currentCredits}");
        }

        public void LoadProgress()
        {
            if (PlayerPrefs.HasKey(SAVE_KEY))
            {
                string json = PlayerPrefs.GetString(SAVE_KEY);
                saveData = JsonUtility.FromJson<SaveData>(json);
                Debug.Log($"[Load] Rank: {saveData.companyRank}, Credits: {saveData.currentCredits}");
            }
            else
            {
                saveData = new SaveData();
                InitializeDefaultUpgrades();
            }
        }

        private void InitializeDefaultUpgrades()
        {
            saveData.submarineUpgrades = new List<SubmarineUpgrade>
            {
                new SubmarineUpgrade { upgradeID = "DescentSpeed", displayName = "Descent Speed", cost = 300, maxLevel = 5 },
                new SubmarineUpgrade { upgradeID = "Hull", displayName = "Reinforced Hull", cost = 500, maxLevel = 5 },
                new SubmarineUpgrade { upgradeID = "Scanner", displayName = "Onboard Scanner", cost = 400, maxLevel = 3 },
                new SubmarineUpgrade { upgradeID = "MedBay", displayName = "Medical Bay", cost = 600, maxLevel = 3 },
                new SubmarineUpgrade { upgradeID = "CargoBay", displayName = "Expanded Cargo", cost = 350, maxLevel = 5 }
            };
        }

        // ====================================================================
        // COMPANY RANK
        // ====================================================================

        public int GetCreditsForNextRank()
        {
            return Mathf.RoundToInt(BASE_CREDITS_PER_RANK *
                Mathf.Pow(RANK_SCALING, saveData.companyRank - 1));
        }

        public float GetRankProgress()
        {
            return (float)saveData.totalCreditsEarned / GetCreditsForNextRank();
        }

        public int GetMaxUnlockedThreatRating()
        {
            return Mathf.Clamp(1 + saveData.companyRank / 3, 1, 5);
        }

        private void CheckRankUp()
        {
            int needed = GetCreditsForNextRank();
            while (saveData.totalCreditsEarned >= needed)
            {
                saveData.totalCreditsEarned -= needed;
                saveData.companyRank++;
                needed = GetCreditsForNextRank();
                Debug.LogWarning($"[Progression] RANK UP! New rank: {saveData.companyRank}");
            }
        }

        // ====================================================================
        // CREDITS
        // ====================================================================

        public void AddCredits(int amount)
        {
            if (amount <= 0) return;
            saveData.currentCredits += amount;
            saveData.totalCreditsEarned += amount;
            CheckRankUp();
        }

        public bool SpendCredits(int amount)
        {
            if (amount <= 0 || saveData.currentCredits < amount) return false;
            saveData.currentCredits -= amount;
            return true;
        }

        // ====================================================================
        // ROLE MASTERY
        // ====================================================================

        public void AddRoleMasteryXP(PlayerRole role, int xp)
        {
            if (role == PlayerRole.None) return;

            int currentXP = saveData.GetMasteryXP(role) + xp;
            int currentLevel = saveData.GetMasteryLevel(role);

            while (currentXP >= MASTERY_XP_PER_LEVEL && currentLevel < MAX_MASTERY_LEVEL)
            {
                currentXP -= MASTERY_XP_PER_LEVEL;
                currentLevel++;
                Debug.Log($"[Mastery] {role} leveled up to {currentLevel}!");
            }

            saveData.SetMastery(role, currentXP, currentLevel);
        }

        public int GetMasteryLevel(PlayerRole role) => saveData.GetMasteryLevel(role);

        public List<string> GetUnlockedMasteryPerks(PlayerRole role)
        {
            var perks = new List<string>();
            int level = GetMasteryLevel(role);

            switch (role)
            {
                case PlayerRole.Scanner:
                    if (level >= 3) perks.Add("Extended radar range (+30%)");
                    if (level >= 5) perks.Add("Passive Mimic detection");
                    if (level >= 8) perks.Add("Corruption phase prediction (10s warning)");
                    break;
                case PlayerRole.Technician:
                    if (level >= 3) perks.Add("Faster extraction (-20% time)");
                    if (level >= 5) perks.Add("Second equipment repair per run");
                    if (level >= 8) perks.Add("Core integrity preservation (+15%)");
                    break;
                case PlayerRole.Warden:
                    if (level >= 3) perks.Add("Extra flare charge (+1)");
                    if (level >= 5) perks.Add("Corruption resistance (+40% total)");
                    if (level >= 8) perks.Add("Flare stun extended (+2 sec)");
                    break;
                case PlayerRole.Courier:
                    if (level >= 3) perks.Add("Carry speed bonus (+15% total)");
                    if (level >= 5) perks.Add("Third core carry slot");
                    if (level >= 8) perks.Add("Core protection on drop");
                    break;
                case PlayerRole.Operator:
                    if (level >= 3) perks.Add("Extended drone range (+50%)");
                    if (level >= 5) perks.Add("Drone noise maker ability");
                    if (level >= 8) perks.Add("Remote equipment activation");
                    break;
            }
            return perks;
        }

        // ====================================================================
        // SUBMARINE UPGRADES
        // ====================================================================

        public bool PurchaseSubUpgrade(string upgradeID)
        {
            var upgrade = saveData.submarineUpgrades.Find(u => u.upgradeID == upgradeID);
            if (upgrade == null || upgrade.currentLevel >= upgrade.maxLevel) return false;

            int cost = upgrade.cost * (upgrade.currentLevel + 1);
            if (!SpendCredits(cost)) return false;

            upgrade.currentLevel++;
            SaveProgress();
            return true;
        }

        public int GetSubUpgradeLevel(string upgradeID)
        {
            var upgrade = saveData.submarineUpgrades.Find(u => u.upgradeID == upgradeID);
            return upgrade?.currentLevel ?? 0;
        }

        // ====================================================================
        // MISSION DEBRIEF
        // ====================================================================

        public int ProcessMissionRewards(List<SignalCore> cores, float payoutMultiplier,
            PlayerRole role, bool success)
        {
            int payout = 0;
            foreach (var core in cores)
            {
                float integrity = core.integrity / 100f;
                payout += Mathf.RoundToInt(core.creditValue * integrity * payoutMultiplier);
            }

            if (!success) payout = Mathf.RoundToInt(payout * 0.5f);

            AddCredits(payout);

            int baseXP = success ? 100 : 30;
            baseXP += cores.Count * 20;
            AddRoleMasteryXP(role, baseXP);

            if (success) saveData.totalMissionsCompleted++;
            else saveData.totalMissionsFailed++;

            SaveProgress();
            Debug.Log($"[Debrief] {cores.Count} cores, {payout} credits, {baseXP} XP");
            return payout;
        }
    }
}
