#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

// Forward declaration for SQLite
struct sqlite3;

// Reuse existing structs from Database.h
enum class UserRole {
    PLAYER,
    ADMIN
};

struct UserRecord {
    int userId;
    std::string username;
    UserRole role;
};

struct PlayerSave {
    // Position
    float x = 0.0f;
    float y = 0.0f;
    float spawnX = 0.0f;
    float spawnY = 0.0f;

    // Stats
    int level = 1;
    int experience = 0;
    int maxHealth = 100;
    int health = 100;
    int maxMana = 50;
    int mana = 50;
    int strength = 10;
    int intelligence = 15;
    int gold = 0;

    // Audio settings
    int masterVolume = 100;
    int musicVolume = 100;
    int soundVolume = 100;
    int monsterVolume = 100;
    int playerMeleeVolume = 100;

    // Consumables
    int healthPotionCharges = 0;
    int manaPotionCharges = 0;
    int upgradeScrolls = 0;

    // Equipment arrays
    int equipPlus[9] = {};
    int equipFire[9] = {};
    int equipIce[9] = {};
    int equipLightning[9] = {};
    int equipPoison[9] = {};
    std::string equipNames[9];
    int equipRarity[9] = {};

    // Inventory arrays
    std::string invKey[2][9];
    int invCnt[2][9] = {};
    int invRarity[2][9] = {};
    int invPlusLevel[2][9] = {};
    
    // Resource inventory (separate from main bags)
    std::string resourceKey[9];
    int resourceCnt[9] = {};
    int resourceRarity[9] = {};
    int resourcePlusLevel[9] = {};
};

struct ItemRecord {
    int itemId;
    std::string name;
    std::string description;
};

struct UserItemRecord {
    int itemId;
    std::string itemName;
    int quantity;
};

class DatabaseSQLite {
public:
    DatabaseSQLite();
    ~DatabaseSQLite();

    // Initialize database (create tables if needed)
    bool initialize(const std::string& dbPath = "data/pixlegends.db");

    // Accounts
    std::optional<UserRecord> registerUser(const std::string& username,
                                           const std::string& password,
                                           UserRole role = UserRole::PLAYER,
                                           std::string* outError = nullptr);

    std::optional<UserRecord> authenticate(const std::string& username,
                                           const std::string& password,
                                           std::string* outError = nullptr);

    std::optional<UserRecord> getUserByName(const std::string& username);
    std::optional<UserRecord> getUserById(int userId);

    // Player state with versioning and backup
    bool savePlayerState(int userId, const PlayerSave& state, std::string* outError = nullptr);
    std::optional<PlayerSave> loadPlayerState(int userId, std::string* outError = nullptr);
    
    // Backup and recovery
    bool createBackup(int userId, const std::string& description = "");
    std::vector<std::string> listBackups(int userId);
    bool restoreFromBackup(int userId, const std::string& backupId);

    // Audio and theme settings
    bool saveAudioSettings(int userId, int master, int music, int sound, int monster, int playerMelee);
    bool loadAudioSettings(int userId, int& master, int& music, int& sound, int& monster, int& playerMelee);
    bool saveTheme(int userId, const std::string& themeName);
    bool loadTheme(int userId, std::string& outThemeName);

    // Items
    std::optional<ItemRecord> upsertItemByName(const std::string& name,
                                               const std::string& description = "");
    bool grantItemToUser(int userId, const std::string& itemName, int quantity, std::string* outError = nullptr);
    std::vector<UserItemRecord> getUserItems(int userId);

    // Remember login
    struct RememberState {
        std::string username;
        std::string password;
        bool remember = false;
    };
    bool saveRememberState(const RememberState& state);
    RememberState loadRememberState();
    bool clearRememberState();

    // Database maintenance
    bool vacuum();
    bool backup(const std::string& backupPath);
    bool verifyIntegrity();

private:
    sqlite3* db;
    std::string dbPath;

    // Internal helpers
    bool executeSQL(const std::string& sql, std::string* outError = nullptr);
    bool createTables(std::string* outError = nullptr);
    
    static std::string generateSalt(size_t length = 16);
    static std::string sha256Hex(const std::string& data);
    static std::string saltedPasswordHashHex(const std::string& salt, const std::string& password);
    static std::string toLower(const std::string& s);
    static std::string roleToString(UserRole role);
    static UserRole roleFromString(const std::string& s);
    static std::string generateBackupId();
    
    // Additional helper methods
    int generateUserId();
    std::string getCurrentTimestamp();
    std::string extractJsonString(const std::string& json, const std::string& key);
    int extractJsonInt(const std::string& json, const std::string& key);
    float extractJsonFloat(const std::string& json, const std::string& key);
    void parseEquipmentAndInventory(const std::string& json, PlayerSave& save);
};