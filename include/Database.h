#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

// Lightweight file-backed database facade.
// Provides: account registration/auth, player save state, and item inventory scaffolding.

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

    // Consumables
    int healthPotionCharges = 0;
    int manaPotionCharges = 0;
    // Upgrades (minimal persistence)
    int upgradeScrolls = 0;
    // Equipment +levels per slot (0..8 = ring..feet)
    int equipPlus[9] = {};
    // Elemental mods (fire/ice/lightning/poison) per slot
    int equipFire[9] = {};
    int equipIce[9] = {};
    int equipLightning[9] = {};
    int equipPoison[9] = {};
    // Inventory stacks per bag slot: invKey[b][i], invCnt[b][i]
    std::string invKey[2][9];
    int         invCnt[2][9] = {};
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

class Database {
public:
    Database();
    ~Database();

    // Initialize storage (create data directory, files if needed)
    bool initialize(const std::string& dataRootDir = "data");

    // Accounts
    std::optional<UserRecord> registerUser(const std::string& username,
                                           const std::string& password,
                                           UserRole role = UserRole::PLAYER,
                                           std::string* outError = nullptr);

    std::optional<UserRecord> authenticate(const std::string& username,
                                           const std::string& password,
                                           std::string* outError = nullptr) const;

    std::optional<UserRecord> getUserByName(const std::string& username) const;
    std::optional<UserRecord> getUserById(int userId) const;

    // Player state
    bool savePlayerState(int userId, const PlayerSave& state, std::string* outError = nullptr) const;
    std::optional<PlayerSave> loadPlayerState(int userId, std::string* outError = nullptr) const;

    // Items (basic scaffolding)
    std::optional<ItemRecord> upsertItemByName(const std::string& name,
                                               const std::string& description = "");
    bool grantItemToUser(int userId, const std::string& itemName, int quantity, std::string* outError = nullptr);
    std::vector<UserItemRecord> getUserItems(int userId) const;

    // Utilities
    const std::string& getDataRoot() const { return dataRoot; }

    // Remembered login (dev convenience; stores username and password in plaintext on disk)
    struct RememberState {
        std::string username;
        std::string password;
        bool remember = false;
    };
    bool saveRememberState(const RememberState& state) const;
    RememberState loadRememberState() const;
    bool clearRememberState() const;

private:
    std::string dataRoot;

    // In-memory cache of users (username -> (id, role)) for quick lookups
    struct InternalUser {
        int id;
        std::string username;
        std::string passwordSalt;
        std::string passwordHashHex;
        UserRole role;
    };

    std::unordered_map<std::string, InternalUser> usernameToUser; // key: lowercase username
    std::unordered_map<int, InternalUser> idToUser;

    // Internal helpers
    static std::string toLower(const std::string& s);
    static std::string roleToString(UserRole role);
    static UserRole roleFromString(const std::string& s);

    static std::string generateSalt(size_t length = 16);
    static std::string sha256Hex(const std::string& data);
    static std::string saltedPasswordHashHex(const std::string& salt, const std::string& password);

    bool ensureOnDiskLayout(std::string* outError);
    bool loadUsersFromDisk(std::string* outError);
    bool appendUserToDisk(const InternalUser& user, std::string* outError);

    // File paths
    std::string usersCsvPath() const;
    std::string itemsCsvPath() const;
    std::string userItemsCsvPath() const;
    std::string playerStatePath(int userId) const;
    std::string rememberFilePath() const;
};


