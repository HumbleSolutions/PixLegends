#include "DatabaseSQLite.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <algorithm>

// Robust file-based database implementation with versioning, backups, and atomic operations
// This provides much better reliability than the original CSV system

DatabaseSQLite::DatabaseSQLite() : db(nullptr) {}

DatabaseSQLite::~DatabaseSQLite() {
    // No cleanup needed for file-based system
}

bool DatabaseSQLite::initialize(const std::string& dbPath) {
    this->dbPath = dbPath;
    
    // Create data directory structure
    std::filesystem::path dbDir = std::filesystem::path(dbPath).parent_path();
    if (!dbDir.empty()) {
        std::filesystem::create_directories(dbDir);
        std::filesystem::create_directories(dbDir / "backups");
        std::filesystem::create_directories(dbDir / "users");
        std::filesystem::create_directories(dbDir / "saves");
    }
    
    // Initialize users file if it doesn't exist
    std::filesystem::path usersPath = dbDir / "users.json";
    if (!std::filesystem::exists(usersPath)) {
        std::ofstream ofs(usersPath);
        ofs << "{\"users\":[]}" << std::endl;
    }
    
    std::cout << "Initialized robust database at: " << dbPath << std::endl;
    return true;
}

std::optional<UserRecord> DatabaseSQLite::registerUser(const std::string& username,
                                                       const std::string& password,
                                                       UserRole role,
                                                       std::string* outError) {
    // Check if user already exists
    if (getUserByName(username)) {
        if (outError) *outError = "Username already exists";
        return std::nullopt;
    }
    
    // Generate new user ID
    int userId = generateUserId();
    
    // Generate salt and hash
    std::string salt = generateSalt();
    std::string hash = saltedPasswordHashHex(salt, password);
    
    // Create user record
    std::filesystem::path userDir = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId);
    std::filesystem::create_directories(userDir);
    
    // Save user data
    std::ofstream userFile(userDir / "user.json");
    userFile << "{\n";
    userFile << "  \"id\": " << userId << ",\n";
    userFile << "  \"username\": \"" << username << "\",\n";
    userFile << "  \"password_salt\": \"" << salt << "\",\n";
    userFile << "  \"password_hash\": \"" << hash << "\",\n";
    userFile << "  \"role\": \"" << roleToString(role) << "\",\n";
    userFile << "  \"created_at\": \"" << getCurrentTimestamp() << "\"\n";
    userFile << "}" << std::endl;
    
    if (!userFile.good()) {
        if (outError) *outError = "Failed to save user data";
        return std::nullopt;
    }
    
    // Create default save state
    PlayerSave defaultSave;
    defaultSave.spawnX = 608.0f;
    defaultSave.spawnY = 328.0f;
    defaultSave.x = defaultSave.spawnX;
    defaultSave.y = defaultSave.spawnY;
    defaultSave.healthPotionCharges = 10;
    defaultSave.manaPotionCharges = 10;
    
    // Set default equipment
    defaultSave.equipNames[0] = "copper_ring";
    defaultSave.equipNames[1] = "cloth_cap";
    defaultSave.equipNames[2] = "string_necklace";
    defaultSave.equipNames[3] = "rusty_sword";
    defaultSave.equipNames[4] = "torn_shirt";
    defaultSave.equipNames[5] = "wooden_shield";
    defaultSave.equipNames[6] = "fabric_gloves";
    defaultSave.equipNames[7] = "frayed_rope";
    defaultSave.equipNames[8] = "canvas_shoes";
    
    // Give starting items
    defaultSave.equipPlus[3] = 1; // Sword +1
    defaultSave.invKey[1][0] = "upgrade_scroll";
    defaultSave.invCnt[1][0] = 3;
    defaultSave.invKey[1][1] = "fire_scroll";
    defaultSave.invCnt[1][1] = 2;
    
    savePlayerState(userId, defaultSave);
    
    std::cout << "Registered new user: " << username << " (ID: " << userId << ")" << std::endl;
    return UserRecord{userId, username, role};
}

std::optional<UserRecord> DatabaseSQLite::authenticate(const std::string& username,
                                                       const std::string& password,
                                                       std::string* outError) {
    std::cout << "Authenticating user: '" << username << "'" << std::endl;
    // Find user by username
    auto user = getUserByName(username);
    if (!user) {
        std::cout << "User not found: " << username << std::endl;
        if (outError) *outError = "User not found";
        return std::nullopt;
    }
    std::cout << "Found user: " << user->username << " (ID: " << user->userId << ")" << std::endl;
    
    // Load user authentication data
    std::filesystem::path userFile = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(user->userId) / "user.json";
    std::ifstream ifs(userFile);
    if (!ifs.good()) {
        if (outError) *outError = "Failed to load user data";
        return std::nullopt;
    }
    
    std::string line, content;
    while (std::getline(ifs, line)) {
        content += line;
    }
    
    // Parse authentication data (simple JSON parsing)
    std::string salt = extractJsonString(content, "password_salt");
    std::string storedHash = extractJsonString(content, "password_hash");
    
    if (salt.empty() || storedHash.empty()) {
        if (outError) *outError = "Corrupted user data";
        return std::nullopt;
    }
    
    // Verify password
    std::string computedHash = saltedPasswordHashHex(salt, password);
    if (computedHash != storedHash) {
        if (outError) *outError = "Invalid password";
        return std::nullopt;
    }
    
    // Update last login
    std::ofstream lastLoginFile(std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(user->userId) / "last_login.txt");
    lastLoginFile << getCurrentTimestamp() << std::endl;
    
    std::cout << "Authenticated user: " << username << " (ID: " << user->userId << ")" << std::endl;
    return user;
}

std::optional<UserRecord> DatabaseSQLite::getUserByName(const std::string& username) {
    std::filesystem::path usersDir = std::filesystem::path(dbPath).parent_path() / "users";
    std::cout << "Searching for user '" << username << "' in: " << usersDir << std::endl;
    
    for (const auto& userDir : std::filesystem::directory_iterator(usersDir)) {
        if (!userDir.is_directory()) continue;
        
        std::ifstream userFile(userDir.path() / "user.json");
        if (!userFile.good()) continue;
        
        std::string line, content;
        while (std::getline(userFile, line)) {
            content += line;
        }
        
        std::string fileUsername = extractJsonString(content, "username");
        std::cout << "Checking user file: " << userDir.path() << " - username: '" << fileUsername << "'" << std::endl;
        if (toLower(fileUsername) == toLower(username)) {
            int userId = extractJsonInt(content, "id");
            std::string roleStr = extractJsonString(content, "role");
            std::cout << "Match found! User: " << fileUsername << ", ID: " << userId << ", Role: " << roleStr << std::endl;
            
            return UserRecord{userId, fileUsername, roleFromString(roleStr)};
        }
    }
    
    return std::nullopt;
}

std::optional<UserRecord> DatabaseSQLite::getUserById(int userId) {
    std::filesystem::path userFile = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId) / "user.json";
    
    std::ifstream ifs(userFile);
    if (!ifs.good()) return std::nullopt;
    
    std::string line, content;
    while (std::getline(ifs, line)) {
        content += line;
    }
    
    std::string username = extractJsonString(content, "username");
    std::string roleStr = extractJsonString(content, "role");
    
    if (username.empty()) return std::nullopt;
    
    return UserRecord{userId, username, roleFromString(roleStr)};
}

bool DatabaseSQLite::savePlayerState(int userId, const PlayerSave& state, std::string* outError) {
    std::filesystem::path saveDir = std::filesystem::path(dbPath).parent_path() / "saves" / std::to_string(userId);
    std::filesystem::create_directories(saveDir);
    
    // Create backup of current save
    std::filesystem::path currentSave = saveDir / "current.json";
    if (std::filesystem::exists(currentSave)) {
        std::string timestamp = getCurrentTimestamp();
        std::replace(timestamp.begin(), timestamp.end(), ':', '-');
        std::replace(timestamp.begin(), timestamp.end(), ' ', '_');
        std::filesystem::path backupPath = saveDir / ("backup_" + timestamp + ".json");
        std::filesystem::copy_file(currentSave, backupPath);
    }
    
    // Write new save atomically (write to temp file, then rename)
    std::filesystem::path tempSave = saveDir / "current.tmp";
    std::ofstream ofs(tempSave);
    
    ofs << "{\n";
    ofs << "  \"version\": 1,\n";
    ofs << "  \"timestamp\": \"" << getCurrentTimestamp() << "\",\n";
    ofs << "  \"position\": {\n";
    ofs << "    \"x\": " << state.x << ",\n";
    ofs << "    \"y\": " << state.y << ",\n";
    ofs << "    \"spawnX\": " << state.spawnX << ",\n";
    ofs << "    \"spawnY\": " << state.spawnY << "\n";
    ofs << "  },\n";
    ofs << "  \"stats\": {\n";
    ofs << "    \"level\": " << state.level << ",\n";
    ofs << "    \"experience\": " << state.experience << ",\n";
    ofs << "    \"maxHealth\": " << state.maxHealth << ",\n";
    ofs << "    \"health\": " << state.health << ",\n";
    ofs << "    \"maxMana\": " << state.maxMana << ",\n";
    ofs << "    \"mana\": " << state.mana << ",\n";
    ofs << "    \"strength\": " << state.strength << ",\n";
    ofs << "    \"intelligence\": " << state.intelligence << ",\n";
    ofs << "    \"gold\": " << state.gold << "\n";
    ofs << "  },\n";
    ofs << "  \"consumables\": {\n";
    ofs << "    \"healthPotionCharges\": " << state.healthPotionCharges << ",\n";
    ofs << "    \"manaPotionCharges\": " << state.manaPotionCharges << ",\n";
    ofs << "    \"upgradeScrolls\": " << state.upgradeScrolls << "\n";
    ofs << "  },\n";
    ofs << "  \"audio\": {\n";
    ofs << "    \"masterVolume\": " << state.masterVolume << ",\n";
    ofs << "    \"musicVolume\": " << state.musicVolume << ",\n";
    ofs << "    \"soundVolume\": " << state.soundVolume << ",\n";
    ofs << "    \"monsterVolume\": " << state.monsterVolume << ",\n";
    ofs << "    \"playerMeleeVolume\": " << state.playerMeleeVolume << "\n";
    ofs << "  },\n";
    ofs << "  \"equipment\": [\n";
    
    for (int i = 0; i < 9; i++) {
        ofs << "    {\n";
        ofs << "      \"slot\": " << i << ",\n";
        ofs << "      \"name\": \"" << state.equipNames[i] << "\",\n";
        ofs << "      \"plus\": " << state.equipPlus[i] << ",\n";
        ofs << "      \"fire\": " << state.equipFire[i] << ",\n";
        ofs << "      \"ice\": " << state.equipIce[i] << ",\n";
        ofs << "      \"lightning\": " << state.equipLightning[i] << ",\n";
        ofs << "      \"poison\": " << state.equipPoison[i] << ",\n";
        ofs << "      \"rarity\": " << state.equipRarity[i] << "\n";
        ofs << "    }" << (i < 8 ? "," : "") << "\n";
    }
    
    ofs << "  ],\n";
    ofs << "  \"inventory\": [\n";
    
    std::cout << "DatabaseSQLite: Writing inventory to JSON..." << std::endl;
    for (int bag = 0; bag < 2; bag++) {
        for (int slot = 0; slot < 9; slot++) {
            if (!state.invKey[bag][slot].empty()) {
                std::cout << "DB saving inv[" << bag << "][" << slot << "]: " << state.invKey[bag][slot] << " (count: " << state.invCnt[bag][slot] << ")" << std::endl;
            }
            ofs << "    {\n";
            ofs << "      \"bag\": " << bag << ",\n";
            ofs << "      \"slot\": " << slot << ",\n";
            ofs << "      \"key\": \"" << state.invKey[bag][slot] << "\",\n";
            ofs << "      \"count\": " << state.invCnt[bag][slot] << ",\n";
            ofs << "      \"rarity\": " << state.invRarity[bag][slot] << ",\n";
            ofs << "      \"plusLevel\": " << state.invPlusLevel[bag][slot] << "\n";
            ofs << "    },\n";
        }
    }
    
    // Add resource inventory as separate entries
    for (int slot = 0; slot < 9; slot++) {
        if (!state.resourceKey[slot].empty()) {
            std::cout << "DB saving resource[" << slot << "]: " << state.resourceKey[slot] << " (count: " << state.resourceCnt[slot] << ")" << std::endl;
        }
        ofs << "    {\n";
        ofs << "      \"bag\": 2,\n";
        ofs << "      \"slot\": " << slot << ",\n";
        ofs << "      \"key\": \"" << state.resourceKey[slot] << "\",\n";
        ofs << "      \"count\": " << state.resourceCnt[slot] << ",\n";
        ofs << "      \"rarity\": " << state.resourceRarity[slot] << ",\n";
        ofs << "      \"plusLevel\": " << state.resourcePlusLevel[slot] << "\n";
        ofs << "    }" << (slot < 8 ? "," : "") << "\n";
    }
    
    ofs << "  ]\n";
    ofs << "}" << std::endl;
    
    if (!ofs.good()) {
        if (outError) *outError = "Failed to write save file";
        return false;
    }
    
    ofs.close();
    
    // Atomic rename
    std::error_code ec;
    std::filesystem::rename(tempSave, currentSave, ec);
    if (ec) {
        if (outError) *outError = "Failed to finalize save: " + ec.message();
        return false;
    }
    
    std::cout << "Saved player state for user " << userId << " (level " << state.level << ")" << std::endl;
    return true;
}

std::optional<PlayerSave> DatabaseSQLite::loadPlayerState(int userId, std::string* outError) {
    std::filesystem::path saveFile = std::filesystem::path(dbPath).parent_path() / "saves" / std::to_string(userId) / "current.json";
    
    std::ifstream ifs(saveFile);
    if (!ifs.good()) {
        if (outError) *outError = "No save file found for user";
        return std::nullopt;
    }
    
    std::string line, content;
    while (std::getline(ifs, line)) {
        content += line;
    }
    
    PlayerSave save;
    
    // Parse position
    save.x = extractJsonFloat(content, "x");
    save.y = extractJsonFloat(content, "y");
    save.spawnX = extractJsonFloat(content, "spawnX");
    save.spawnY = extractJsonFloat(content, "spawnY");
    
    // Parse stats
    save.level = extractJsonInt(content, "level");
    save.experience = extractJsonInt(content, "experience");
    save.maxHealth = extractJsonInt(content, "maxHealth");
    save.health = extractJsonInt(content, "health");
    save.maxMana = extractJsonInt(content, "maxMana");
    save.mana = extractJsonInt(content, "mana");
    save.strength = extractJsonInt(content, "strength");
    save.intelligence = extractJsonInt(content, "intelligence");
    save.gold = extractJsonInt(content, "gold");
    
    // Parse consumables
    save.healthPotionCharges = extractJsonInt(content, "healthPotionCharges");
    save.manaPotionCharges = extractJsonInt(content, "manaPotionCharges");
    save.upgradeScrolls = extractJsonInt(content, "upgradeScrolls");
    
    // Parse audio
    save.masterVolume = extractJsonInt(content, "masterVolume");
    save.musicVolume = extractJsonInt(content, "musicVolume");
    save.soundVolume = extractJsonInt(content, "soundVolume");
    save.monsterVolume = extractJsonInt(content, "monsterVolume");
    save.playerMeleeVolume = extractJsonInt(content, "playerMeleeVolume");
    
    // Parse equipment and inventory (simplified parsing)
    parseEquipmentAndInventory(content, save);
    
    std::cout << "Loaded player state for user " << userId << " (level " << save.level << ")" << std::endl;
    return save;
}

bool DatabaseSQLite::createBackup(int userId, const std::string& description) {
    auto save = loadPlayerState(userId);
    if (!save) return false;
    
    std::filesystem::path backupDir = std::filesystem::path(dbPath).parent_path() / "backups" / std::to_string(userId);
    std::filesystem::create_directories(backupDir);
    
    std::string backupId = generateBackupId();
    std::filesystem::path backupFile = backupDir / (backupId + ".json");
    
    std::ofstream ofs(backupFile);
    ofs << "{\n";
    ofs << "  \"backup_id\": \"" << backupId << "\",\n";
    ofs << "  \"description\": \"" << description << "\",\n";
    ofs << "  \"created_at\": \"" << getCurrentTimestamp() << "\",\n";
    ofs << "  \"level\": " << save->level << ",\n";
    ofs << "  \"experience\": " << save->experience << ",\n";
    ofs << "  \"health\": " << save->health << ",\n";
    ofs << "  \"maxHealth\": " << save->maxHealth << ",\n";
    ofs << "  \"x\": " << save->x << ",\n";
    ofs << "  \"y\": " << save->y << "\n";
    ofs << "}" << std::endl;
    
    if (ofs.good()) {
        std::cout << "Created backup " << backupId << " for user " << userId << std::endl;
        return true;
    }
    
    return false;
}

std::vector<std::string> DatabaseSQLite::listBackups(int userId) {
    std::vector<std::string> backups;
    std::filesystem::path backupDir = std::filesystem::path(dbPath).parent_path() / "backups" / std::to_string(userId);
    
    if (!std::filesystem::exists(backupDir)) return backups;
    
    for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
        if (entry.path().extension() == ".json") {
            backups.push_back(entry.path().stem().string());
        }
    }
    
    std::sort(backups.rbegin(), backups.rend()); // Sort newest first
    return backups;
}

bool DatabaseSQLite::saveAudioSettings(int userId, int master, int music, int sound, int monster, int playerMelee) {
    std::filesystem::path userDir = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId);
    std::filesystem::create_directories(userDir);
    
    std::ofstream ofs(userDir / "audio.json");
    ofs << "{\n";
    ofs << "  \"masterVolume\": " << master << ",\n";
    ofs << "  \"musicVolume\": " << music << ",\n";
    ofs << "  \"soundVolume\": " << sound << ",\n";
    ofs << "  \"monsterVolume\": " << monster << ",\n";
    ofs << "  \"playerMeleeVolume\": " << playerMelee << "\n";
    ofs << "}" << std::endl;
    
    return ofs.good();
}

bool DatabaseSQLite::loadAudioSettings(int userId, int& master, int& music, int& sound, int& monster, int& playerMelee) {
    std::filesystem::path audioFile = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId) / "audio.json";
    
    std::ifstream ifs(audioFile);
    if (!ifs.good()) return false;
    
    std::string line, content;
    while (std::getline(ifs, line)) {
        content += line;
    }
    
    master = extractJsonInt(content, "masterVolume");
    music = extractJsonInt(content, "musicVolume");
    sound = extractJsonInt(content, "soundVolume");
    monster = extractJsonInt(content, "monsterVolume");
    playerMelee = extractJsonInt(content, "playerMeleeVolume");
    
    return true;
}

bool DatabaseSQLite::saveTheme(int userId, const std::string& themeName) {
    std::filesystem::path userDir = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId);
    std::filesystem::create_directories(userDir);
    
    std::ofstream ofs(userDir / "theme.txt");
    ofs << themeName << std::endl;
    
    return ofs.good();
}

bool DatabaseSQLite::loadTheme(int userId, std::string& outThemeName) {
    std::filesystem::path themeFile = std::filesystem::path(dbPath).parent_path() / "users" / std::to_string(userId) / "theme.txt";
    
    std::ifstream ifs(themeFile);
    if (!ifs.good()) return false;
    
    std::getline(ifs, outThemeName);
    return true;
}

bool DatabaseSQLite::saveRememberState(const RememberState& state) {
    std::filesystem::path rememberFile = std::filesystem::path(dbPath).parent_path() / "remember.json";
    
    std::ofstream ofs(rememberFile);
    ofs << "{\n";
    ofs << "  \"username\": \"" << state.username << "\",\n";
    ofs << "  \"password\": \"" << state.password << "\",\n";
    ofs << "  \"remember\": " << (state.remember ? "true" : "false") << "\n";
    ofs << "}" << std::endl;
    
    return ofs.good();
}

DatabaseSQLite::RememberState DatabaseSQLite::loadRememberState() {
    RememberState state;
    
    std::filesystem::path rememberFile = std::filesystem::path(dbPath).parent_path() / "remember.json";
    std::ifstream ifs(rememberFile);
    
    if (!ifs.good()) return state;
    
    std::string line, content;
    while (std::getline(ifs, line)) {
        content += line;
    }
    
    state.username = extractJsonString(content, "username");
    state.password = extractJsonString(content, "password");
    
    // Parse boolean remember field (not quoted in JSON)
    std::string searchKey = "\"remember\"";
    size_t pos = content.find(searchKey);
    if (pos != std::string::npos) {
        pos = content.find(":", pos);
        if (pos != std::string::npos) {
            pos++;
            while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
            if (pos + 4 <= content.length() && content.substr(pos, 4) == "true") {
                state.remember = true;
            } else {
                state.remember = false;
            }
        }
    }
    
    return state;
}

bool DatabaseSQLite::clearRememberState() {
    std::filesystem::path rememberFile = std::filesystem::path(dbPath).parent_path() / "remember.json";
    return std::filesystem::remove(rememberFile);
}

// Helper methods implementation
int DatabaseSQLite::generateUserId() {
    std::filesystem::path usersDir = std::filesystem::path(dbPath).parent_path() / "users";
    
    int maxId = 0;
    if (std::filesystem::exists(usersDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(usersDir)) {
            if (entry.is_directory()) {
                try {
                    int id = std::stoi(entry.path().filename().string());
                    maxId = std::max(maxId, id);
                } catch (...) {
                    // Ignore non-numeric directory names
                }
            }
        }
    }
    
    return maxId + 1;
}

std::string DatabaseSQLite::generateSalt(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string salt;
    for (size_t i = 0; i < length; ++i) {
        salt += chars[dis(gen)];
    }
    return salt;
}

std::string DatabaseSQLite::sha256Hex(const std::string& data) {
    // Simplified hash - in production use proper SHA256
    std::hash<std::string> hasher;
    size_t hash = hasher(data);
    
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

std::string DatabaseSQLite::saltedPasswordHashHex(const std::string& salt, const std::string& password) {
    return sha256Hex(salt + password + "pixlegends_robust_salt_2024");
}

std::string DatabaseSQLite::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string DatabaseSQLite::roleToString(UserRole role) {
    return role == UserRole::ADMIN ? "ADMIN" : "PLAYER";
}

UserRole DatabaseSQLite::roleFromString(const std::string& s) {
    return s == "ADMIN" ? UserRole::ADMIN : UserRole::PLAYER;
}

std::string DatabaseSQLite::generateBackupId() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << "backup_" << time_t << "_" << (rand() % 1000);
    return oss.str();
}

std::string DatabaseSQLite::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Simple JSON parsing helpers
std::string DatabaseSQLite::extractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";
    pos++;
    
    size_t endPos = json.find("\"", pos);
    if (endPos == std::string::npos) return "";
    
    return json.substr(pos, endPos - pos);
}

int DatabaseSQLite::extractJsonInt(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0;
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return 0;
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    size_t endPos = pos;
    while (endPos < json.length() && (std::isdigit(json[endPos]) || json[endPos] == '-' || json[endPos] == '.')) {
        endPos++;
    }
    
    if (endPos == pos) return 0;
    
    try {
        return std::stoi(json.substr(pos, endPos - pos));
    } catch (...) {
        return 0;
    }
}

float DatabaseSQLite::extractJsonFloat(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0.0f;
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return 0.0f;
    pos++;
    
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    size_t endPos = pos;
    while (endPos < json.length() && (std::isdigit(json[endPos]) || json[endPos] == '-' || json[endPos] == '.')) {
        endPos++;
    }
    
    if (endPos == pos) return 0.0f;
    
    try {
        return std::stof(json.substr(pos, endPos - pos));
    } catch (...) {
        return 0.0f;
    }
}

void DatabaseSQLite::parseEquipmentAndInventory(const std::string& json, PlayerSave& save) {
    // Simple equipment parsing - look for equipment array
    size_t equipPos = json.find("\"equipment\":");
    if (equipPos != std::string::npos) {
        for (int i = 0; i < 9; i++) {
            // Find the slot data (simplified)
            std::string slotSearch = "\"slot\": " + std::to_string(i);
            size_t slotPos = json.find(slotSearch, equipPos);
            if (slotPos != std::string::npos) {
                // Extract equipment data for this slot
                size_t namePos = json.find("\"name\":", slotPos);
                if (namePos != std::string::npos) {
                    save.equipNames[i] = extractJsonString(json.substr(namePos - 50, 200), "name");
                    save.equipPlus[i] = extractJsonInt(json.substr(namePos - 50, 200), "plus");
                    save.equipFire[i] = extractJsonInt(json.substr(namePos - 50, 200), "fire");
                    save.equipIce[i] = extractJsonInt(json.substr(namePos - 50, 200), "ice");
                    save.equipLightning[i] = extractJsonInt(json.substr(namePos - 50, 200), "lightning");
                    save.equipPoison[i] = extractJsonInt(json.substr(namePos - 50, 200), "poison");
                    save.equipRarity[i] = extractJsonInt(json.substr(namePos - 50, 200), "rarity");
                }
            }
        }
    }
    
    // Proper inventory parsing
    std::cout << "Parsing inventory from JSON..." << std::endl;
    size_t invPos = json.find("\"inventory\":");
    if (invPos != std::string::npos) {
        // Find the start of the inventory array
        size_t arrayStart = json.find("[", invPos);
        if (arrayStart != std::string::npos) {
            size_t currentPos = arrayStart + 1;
            
            // Parse each inventory item
            while (currentPos < json.length()) {
                // Find next item object
                size_t itemStart = json.find("{", currentPos);
                if (itemStart == std::string::npos) break;
                
                size_t itemEnd = json.find("}", itemStart);
                if (itemEnd == std::string::npos) break;
                
                // Extract the item JSON
                std::string itemJson = json.substr(itemStart, itemEnd - itemStart + 1);
                
                // Parse bag and slot
                int bag = extractJsonInt(itemJson, "bag");
                int slot = extractJsonInt(itemJson, "slot");
                
                if (bag >= 0 && bag < 2 && slot >= 0 && slot < 9) {
                    // Regular inventory bags (0 and 1)
                    save.invKey[bag][slot] = extractJsonString(itemJson, "key");
                    save.invCnt[bag][slot] = extractJsonInt(itemJson, "count");
                    save.invRarity[bag][slot] = extractJsonInt(itemJson, "rarity");
                    save.invPlusLevel[bag][slot] = extractJsonInt(itemJson, "plusLevel");
                    
                    if (!save.invKey[bag][slot].empty()) {
                        std::cout << "Parsed inv[" << bag << "][" << slot << "]: " << save.invKey[bag][slot] 
                                  << " (count: " << save.invCnt[bag][slot] << ")" << std::endl;
                    }
                } else if (bag == 2 && slot >= 0 && slot < 9) {
                    // Resource inventory (bag 2)
                    save.resourceKey[slot] = extractJsonString(itemJson, "key");
                    save.resourceCnt[slot] = extractJsonInt(itemJson, "count");
                    save.resourceRarity[slot] = extractJsonInt(itemJson, "rarity");
                    save.resourcePlusLevel[slot] = extractJsonInt(itemJson, "plusLevel");
                    
                    if (!save.resourceKey[slot].empty()) {
                        std::cout << "Parsed resource[" << slot << "]: " << save.resourceKey[slot] 
                                  << " (count: " << save.resourceCnt[slot] << ")" << std::endl;
                    }
                }
                
                currentPos = itemEnd + 1;
                
                // Check if we've reached the end of the array
                size_t nextComma = json.find(",", currentPos);
                size_t arrayEnd = json.find("]", currentPos);
                if (arrayEnd != std::string::npos && (nextComma == std::string::npos || arrayEnd < nextComma)) {
                    break;
                }
            }
        }
    }
    std::cout << "Inventory parsing complete" << std::endl;
}