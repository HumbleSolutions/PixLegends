#include "Database.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <tuple>
#include <cctype>

// Simple bundled SHA-256 (portable) for password hashing
// This is a minimal implementation to avoid external deps.
namespace {
    // Public domain implementation by Brad Conte (abridged) adapted to a tiny helper
    // For brevity and to keep it self-contained, we embed a compact SHA256 routine.
    // Note: This is sufficient for demo; for production prefer vetted crypto libs.
    typedef unsigned char BYTE; // 8-bit byte
    typedef unsigned int  WORD; // 32-bit word, change to "long" for 16-bit machines

    const WORD K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    inline WORD ROTRIGHT(WORD a, WORD b) { return (((a) >> (b)) | ((a) << (32 - (b)))); }
    inline WORD CH(WORD x, WORD y, WORD z) { return ((x & y) ^ (~x & z)); }
    inline WORD MAJ(WORD x, WORD y, WORD z) { return ((x & y) ^ (x & z) ^ (y & z)); }
    inline WORD EP0(WORD x) { return (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22)); }
    inline WORD EP1(WORD x) { return (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25)); }
    inline WORD SIG0(WORD x) { return (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3)); }
    inline WORD SIG1(WORD x) { return (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10)); }

    struct SHA256_CTX { BYTE data[64]; WORD datalen; unsigned long long bitlen; WORD state[8]; };

    void sha256_transform(SHA256_CTX* ctx, const BYTE data[])
    {
        WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
            m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

        a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
        e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
        for (i = 0; i < 64; ++i) {
            t1 = h + EP1(e) + CH(e,f,g) + K[i] + m[i];
            t2 = EP0(a) + MAJ(a,b,c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
        ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
    }

    void sha256_init(SHA256_CTX* ctx)
    {
        ctx->datalen = 0; ctx->bitlen = 0;
        ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85; ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
        ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c; ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    }

    void sha256_update(SHA256_CTX* ctx, const BYTE data[], size_t len)
    {
        for (size_t i = 0; i < len; ++i) {
            ctx->data[ctx->datalen] = data[i];
            ctx->datalen++;
            if (ctx->datalen == 64) {
                sha256_transform(ctx, ctx->data);
                ctx->bitlen += 512;
                ctx->datalen = 0;
            }
        }
    }

    void sha256_final(SHA256_CTX* ctx, BYTE hash[])
    {
        WORD i = ctx->datalen;
        // Pad whatever data is left in the buffer.
        if (ctx->datalen < 56) {
            ctx->data[i++] = 0x80;
            while (i < 56) ctx->data[i++] = 0x00;
        } else {
            ctx->data[i++] = 0x80;
            while (i < 64) ctx->data[i++] = 0x00;
            sha256_transform(ctx, ctx->data);
            memset(ctx->data, 0, 56);
        }
        ctx->bitlen += ctx->datalen * 8;
        ctx->data[63] = ctx->bitlen;
        ctx->data[62] = ctx->bitlen >> 8;
        ctx->data[61] = ctx->bitlen >> 16;
        ctx->data[60] = ctx->bitlen >> 24;
        ctx->data[59] = ctx->bitlen >> 32;
        ctx->data[58] = ctx->bitlen >> 40;
        ctx->data[57] = ctx->bitlen >> 48;
        ctx->data[56] = ctx->bitlen >> 56;
        sha256_transform(ctx, ctx->data);
        for (i = 0; i < 32; ++i) hash[i] = ctx->state[i >> 2] >> (24 - (i & 0x03) * 8);
    }
}

static std::string bytesToHex(const unsigned char* bytes, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

// Database implementation
Database::Database() {}
Database::~Database() {}

bool Database::initialize(const std::string& dataRootDir) {
    dataRoot = dataRootDir;
    std::string err;
    if (!ensureOnDiskLayout(&err)) {
        return false;
    }
    if (!loadUsersFromDisk(&err)) {
        return false;
    }
    return true;
}

std::string Database::toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string Database::roleToString(UserRole role) {
    switch (role) {
        case UserRole::PLAYER: return "PLAYER";
        case UserRole::ADMIN: return "ADMIN";
    }
    return "PLAYER";
}

UserRole Database::roleFromString(const std::string& s) {
    std::string v = toLower(s);
    if (v == "admin") return UserRole::ADMIN;
    return UserRole::PLAYER;
}

std::string Database::generateSalt(size_t length) {
    static thread_local std::mt19937_64 rng{
        static_cast<unsigned long long>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    };
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<size_t> dist(0, sizeof(alphabet) - 2);
    std::string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i) s.push_back(alphabet[dist(rng)]);
    return s;
}

std::string Database::sha256Hex(const std::string& data) {
    unsigned char hash[32];
    SHA256_CTX ctx; sha256_init(&ctx); sha256_update(&ctx, reinterpret_cast<const unsigned char*>(data.data()), data.size()); sha256_final(&ctx, hash);
    return bytesToHex(hash, 32);
}

std::string Database::saltedPasswordHashHex(const std::string& salt, const std::string& password) {
    return sha256Hex(salt + ":" + password);
}

bool Database::ensureOnDiskLayout(std::string* outError) {
    try {
        std::filesystem::create_directories(dataRoot);
        // Users CSV: id,username,role,salt,passhashhex
        if (!std::filesystem::exists(usersCsvPath())) {
            std::ofstream f(usersCsvPath(), std::ios::out | std::ios::trunc);
            f << "id,username,role,salt,passhash\n";
        }
        // Items CSV: id,name,description
        if (!std::filesystem::exists(itemsCsvPath())) {
            std::ofstream f(itemsCsvPath(), std::ios::out | std::ios::trunc);
            f << "id,name,description\n";
        }
        // User items CSV: userId,itemId,quantity
        if (!std::filesystem::exists(userItemsCsvPath())) {
            std::ofstream f(userItemsCsvPath(), std::ios::out | std::ios::trunc);
            f << "userId,itemId,quantity\n";
        }
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
    return true;
}

bool Database::loadUsersFromDisk(std::string* outError) {
    try {
        usernameToUser.clear();
        idToUser.clear();
        std::ifstream f(usersCsvPath());
        std::string line;
        // skip header
        std::getline(f, line);
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string idStr, username, roleStr, salt, hash;
            std::getline(ss, idStr, ',');
            std::getline(ss, username, ',');
            std::getline(ss, roleStr, ',');
            std::getline(ss, salt, ',');
            std::getline(ss, hash, ',');
            InternalUser u;
            u.id = std::stoi(idStr);
            u.username = username;
            u.role = roleFromString(roleStr);
            u.passwordSalt = salt;
            u.passwordHashHex = hash;
            usernameToUser[toLower(username)] = u;
            idToUser[u.id] = u;
        }
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
    return true;
}

bool Database::appendUserToDisk(const InternalUser& user, std::string* outError) {
    try {
        std::ofstream f(usersCsvPath(), std::ios::out | std::ios::app);
        f << user.id << "," << user.username << "," << roleToString(user.role) << "," << user.passwordSalt << "," << user.passwordHashHex << "\n";
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
    return true;
}

std::optional<UserRecord> Database::registerUser(const std::string& username, const std::string& password, UserRole role, std::string* outError) {
    std::string key = toLower(username);
    if (username.empty() || password.empty()) {
        if (outError) *outError = "Username and password required";
        return std::nullopt;
    }
    if (usernameToUser.find(key) != usernameToUser.end()) {
        if (outError) *outError = "Username already exists";
        return std::nullopt;
    }
    InternalUser u;
    // Simple increasing id: max existing + 1
    int nextId = 1;
    if (!idToUser.empty()) {
        nextId = std::max_element(idToUser.begin(), idToUser.end(), [](auto& a, auto& b){ return a.first < b.first; })->first + 1;
    }
    u.id = nextId;
    u.username = username;
    u.role = role;
    u.passwordSalt = generateSalt();
    u.passwordHashHex = saltedPasswordHashHex(u.passwordSalt, password);
    if (!appendUserToDisk(u, outError)) return std::nullopt;
    usernameToUser[key] = u; idToUser[u.id] = u;
    return UserRecord{u.id, u.username, u.role};
}

std::optional<UserRecord> Database::authenticate(const std::string& username, const std::string& password, std::string* outError) const {
    std::string key = toLower(username);
    auto it = usernameToUser.find(key);
    if (it == usernameToUser.end()) {
        if (outError) *outError = "Invalid credentials";
        return std::nullopt;
    }
    const InternalUser& u = it->second;
    std::string hash = saltedPasswordHashHex(u.passwordSalt, password);
    if (hash != u.passwordHashHex) {
        if (outError) *outError = "Invalid credentials";
        return std::nullopt;
    }
    return UserRecord{u.id, u.username, u.role};
}

std::optional<UserRecord> Database::getUserByName(const std::string& username) const {
    auto it = usernameToUser.find(toLower(username));
    if (it == usernameToUser.end()) return std::nullopt;
    const InternalUser& u = it->second;
    return UserRecord{u.id, u.username, u.role};
}

std::optional<UserRecord> Database::getUserById(int userId) const {
    auto it = idToUser.find(userId);
    if (it == idToUser.end()) return std::nullopt;
    const InternalUser& u = it->second;
    return UserRecord{u.id, u.username, u.role};
}

bool Database::savePlayerState(int userId, const PlayerSave& s, std::string* outError) const {
    try {
        std::ofstream f(playerStatePath(userId), std::ios::out | std::ios::trunc);
        if (!f) { if (outError) *outError = "Failed to write player state"; return false; }
        // Simple key=value format
        f << "x=" << s.x << "\n";
        f << "y=" << s.y << "\n";
        f << "spawnX=" << s.spawnX << "\n";
        f << "spawnY=" << s.spawnY << "\n";
        f << "level=" << s.level << "\n";
        f << "experience=" << s.experience << "\n";
        f << "maxHealth=" << s.maxHealth << "\n";
        f << "health=" << s.health << "\n";
        f << "maxMana=" << s.maxMana << "\n";
        f << "mana=" << s.mana << "\n";
        f << "strength=" << s.strength << "\n";
        f << "intelligence=" << s.intelligence << "\n";
        f << "gold=" << s.gold << "\n";
        // Audio
        f << "masterVolume=" << s.masterVolume << "\n";
        f << "musicVolume=" << s.musicVolume << "\n";
        f << "soundVolume=" << s.soundVolume << "\n";
        f << "monsterVolume=" << s.monsterVolume << "\n";
        f << "playerMeleeVolume=" << s.playerMeleeVolume << "\n";
        f << "healthPotionCharges=" << s.healthPotionCharges << "\n";
        f << "manaPotionCharges=" << s.manaPotionCharges << "\n";
        // Persist equipment +levels and elemental mods
        for (int i = 0; i < 9; ++i) {
            f << "equipPlus_" << i << "=" << s.equipPlus[i] << "\n";
            f << "equipFire_" << i << "=" << s.equipFire[i] << "\n";
            f << "equipIce_" << i << "=" << s.equipIce[i] << "\n";
            f << "equipLightning_" << i << "=" << s.equipLightning[i] << "\n";
            f << "equipPoison_" << i << "=" << s.equipPoison[i] << "\n";
            f << "equipName_" << i << "=" << s.equipNames[i] << "\n";
            f << "equipRarity_" << i << "=" << s.equipRarity[i] << "\n";
        }
        // Persist inventory grid
        for (int b = 0; b < 2; ++b) {
            for (int i = 0; i < 9; ++i) {
                f << "invKey_" << b << "_" << i << "=" << s.invKey[b][i] << "\n";
                f << "invCnt_" << b << "_" << i << "=" << s.invCnt[b][i] << "\n";
                f << "invRarity_" << b << "_" << i << "=" << s.invRarity[b][i] << "\n";
                f << "invPlusLevel_" << b << "_" << i << "=" << s.invPlusLevel[b][i] << "\n";
            }
        }
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
    return true;
}

std::optional<PlayerSave> Database::loadPlayerState(int userId, std::string* outError) const {
    PlayerSave s;
    try {
        std::ifstream f(playerStatePath(userId));
        if (!f) return std::nullopt; // no save yet
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            if (key == "x") s.x = std::stof(val);
            else if (key == "y") s.y = std::stof(val);
            else if (key == "spawnX") s.spawnX = std::stof(val);
            else if (key == "spawnY") s.spawnY = std::stof(val);
            else if (key == "level") s.level = std::stoi(val);
            else if (key == "experience") s.experience = std::stoi(val);
            else if (key == "maxHealth") s.maxHealth = std::stoi(val);
            else if (key == "health") s.health = std::stoi(val);
            else if (key == "maxMana") s.maxMana = std::stoi(val);
            else if (key == "mana") s.mana = std::stoi(val);
            else if (key == "strength") s.strength = std::stoi(val);
            else if (key == "intelligence") s.intelligence = std::stoi(val);
            else if (key == "gold") s.gold = std::stoi(val);
            else if (key == "masterVolume") s.masterVolume = std::stoi(val);
            else if (key == "musicVolume") s.musicVolume = std::stoi(val);
            else if (key == "soundVolume") s.soundVolume = std::stoi(val);
            else if (key == "monsterVolume") s.monsterVolume = std::stoi(val);
            else if (key == "playerMeleeVolume") s.playerMeleeVolume = std::stoi(val);
            else if (key == "healthPotionCharges") s.healthPotionCharges = std::stoi(val);
            else if (key == "manaPotionCharges") s.manaPotionCharges = std::stoi(val);
            else if (key.rfind("equipPlus_",0)==0) { int i = key[10]-'0'; if (i>=0&&i<9) s.equipPlus[i] = std::stoi(val); }
            else if (key.rfind("equipFire_",0)==0) { int i = key[10]-'0'; if (i>=0&&i<9) s.equipFire[i] = std::stoi(val); }
            else if (key.rfind("equipIce_",0)==0) { int i = key[9]-'0'; if (i>=0&&i<9) s.equipIce[i] = std::stoi(val); }
            else if (key.rfind("equipLightning_",0)==0) { int i = key[15]-'0'; if (i>=0&&i<9) s.equipLightning[i] = std::stoi(val); }
            else if (key.rfind("equipPoison_",0)==0) { int i = key[12]-'0'; if (i>=0&&i<9) s.equipPoison[i] = std::stoi(val); }
            else if (key.rfind("equipName_",0)==0) { int i = key[10]-'0'; if (i>=0&&i<9) s.equipNames[i] = val; }
            else if (key.rfind("equipRarity_",0)==0) { int i = key[12]-'0'; if (i>=0&&i<9) s.equipRarity[i] = std::stoi(val); }
            else if (key.rfind("invKey_",0)==0) {
                int b = key[7]-'0'; int i = key[9]-'0'; if (b>=0&&b<2&&i>=0&&i<9) s.invKey[b][i] = val;
            } else if (key.rfind("invCnt_",0)==0) {
                int b = key[7]-'0'; int i = key[9]-'0'; if (b>=0&&b<2&&i>=0&&i<9) s.invCnt[b][i] = std::stoi(val);
            } else if (key.rfind("invRarity_",0)==0) {
                int b = key[10]-'0'; int i = key[12]-'0'; if (b>=0&&b<2&&i>=0&&i<9) s.invRarity[b][i] = std::stoi(val);
            } else if (key.rfind("invPlusLevel_",0)==0) {
                int b = key[13]-'0'; int i = key[15]-'0'; if (b>=0&&b<2&&i>=0&&i<9) s.invPlusLevel[b][i] = std::stoi(val);
            }
        }
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return std::nullopt;
    }
    return s;
}

std::optional<ItemRecord> Database::upsertItemByName(const std::string& name, const std::string& description) {
    // First scan for existing
    int maxId = 0;
    int foundId = -1;
    try {
        std::ifstream in(itemsCsvPath());
        std::string line;
        std::getline(in, line); // header
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string idStr, itemName, desc;
            std::getline(ss, idStr, ',');
            std::getline(ss, itemName, ',');
            std::getline(ss, desc, ',');
            int id = std::stoi(idStr);
            if (id > maxId) maxId = id;
            if (itemName == name) {
                foundId = id; break;
            }
        }
    } catch (...) {}

    if (foundId >= 0) {
        return ItemRecord{foundId, name, description};
    }
    int newId = maxId + 1;
    try {
        std::ofstream out(itemsCsvPath(), std::ios::out | std::ios::app);
        out << newId << "," << name << "," << description << "\n";
    } catch (...) { return std::nullopt; }
    return ItemRecord{newId, name, description};
}

bool Database::grantItemToUser(int userId, const std::string& itemName, int quantity, std::string* outError) {
    if (quantity <= 0) {
        if (outError) *outError = "Quantity must be positive";
        return false;
    }
    auto item = upsertItemByName(itemName, "");
    if (!item) { if (outError) *outError = "Failed to upsert item"; return false; }
    try {
        // Read all, update or append
        std::vector<std::tuple<int,int,int>> rows; // userId,itemId,qty
        {
            std::ifstream in(userItemsCsvPath());
            std::string line; std::getline(in, line);
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                std::stringstream ss(line);
                std::string uStr, iStr, qStr; std::getline(ss, uStr, ','); std::getline(ss, iStr, ','); std::getline(ss, qStr, ',');
                rows.emplace_back(std::stoi(uStr), std::stoi(iStr), std::stoi(qStr));
            }
        }
        bool updated = false;
        for (auto& t : rows) {
            if (std::get<0>(t) == userId && std::get<1>(t) == item->itemId) {
                std::get<2>(t) += quantity; updated = true; break;
            }
        }
        if (!updated) rows.emplace_back(userId, item->itemId, quantity);
        // Write back
        std::ofstream out(userItemsCsvPath(), std::ios::out | std::ios::trunc);
        out << "userId,itemId,quantity\n";
        for (auto& t : rows) {
            out << std::get<0>(t) << "," << std::get<1>(t) << "," << std::get<2>(t) << "\n";
        }
    } catch (const std::exception& e) {
        if (outError) *outError = e.what();
        return false;
    }
    return true;
}

std::vector<UserItemRecord> Database::getUserItems(int userId) const {
    std::vector<UserItemRecord> result;
    try {
        // Build id->name table
        std::unordered_map<int,std::string> idToName;
        {
            std::ifstream in(itemsCsvPath()); std::string line; std::getline(in, line);
            while (std::getline(in, line)) {
                if (line.empty()) continue; std::stringstream ss(line); std::string idStr, name, desc;
                std::getline(ss, idStr, ','); std::getline(ss, name, ','); std::getline(ss, desc, ',');
                idToName[std::stoi(idStr)] = name;
            }
        }
        std::ifstream in(userItemsCsvPath()); std::string line; std::getline(in, line);
        while (std::getline(in, line)) {
            if (line.empty()) continue; std::stringstream ss(line); std::string uStr, iStr, qStr;
            std::getline(ss, uStr, ','); std::getline(ss, iStr, ','); std::getline(ss, qStr, ',');
            int u = std::stoi(uStr), itemId = std::stoi(iStr), qty = std::stoi(qStr);
            if (u == userId) {
                auto it = idToName.find(itemId);
                result.push_back(UserItemRecord{ itemId, it != idToName.end() ? it->second : std::string("unknown"), qty });
            }
        }
    } catch (...) {}
    return result;
}

// Paths
std::string Database::usersCsvPath() const { return dataRoot + "/users.csv"; }
std::string Database::itemsCsvPath() const { return dataRoot + "/items.csv"; }
std::string Database::userItemsCsvPath() const { return dataRoot + "/user_items.csv"; }
std::string Database::playerStatePath(int userId) const { return dataRoot + "/player_" + std::to_string(userId) + ".save"; }
std::string Database::userAudioPath(int userId) const { return dataRoot + "/audio_" + std::to_string(userId) + ".cfg"; }
std::string Database::userThemePath(int userId) const { return dataRoot + "/theme_" + std::to_string(userId) + ".cfg"; }
std::string Database::rememberFilePath() const { return dataRoot + "/remember.txt"; }

bool Database::saveRememberState(const RememberState& state) const {
    try {
        std::ofstream f(rememberFilePath(), std::ios::out | std::ios::trunc);
        if (!f) return false;
        // INSECURE: plaintext for dev convenience
        f << (state.remember ? "1" : "0") << "\n" << state.username << "\n" << state.password << "\n";
        return true;
    } catch (...) { return false; }
}

Database::RememberState Database::loadRememberState() const {
    RememberState r;
    try {
        std::ifstream f(rememberFilePath());
        if (!f) return r;
        std::string line;
        std::getline(f, line); r.remember = (line == "1");
        std::getline(f, r.username);
        std::getline(f, r.password);
    } catch (...) {}
    return r;
}

bool Database::clearRememberState() const {
    try {
        if (std::filesystem::exists(rememberFilePath())) std::filesystem::remove(rememberFilePath());
        return true;
    } catch (...) { return false; }
}

bool Database::saveAudioSettings(int userId, int master, int music, int sound, int monster, int playerMelee) const {
    try {
        std::ofstream f(userAudioPath(userId), std::ios::out | std::ios::trunc);
        if (!f) return false;
        f << master << "\n" << music << "\n" << sound << "\n" << monster << "\n" << playerMelee << "\n";
        return true;
    } catch (...) { return false; }
}

bool Database::loadAudioSettings(int userId, int& master, int& music, int& sound, int& monster, int& playerMelee) const {
    master = music = sound = monster = playerMelee = 100;
    try {
        std::ifstream f(userAudioPath(userId));
        if (!f) return false;
        std::string line;
        if (std::getline(f, line)) master = std::stoi(line);
        if (std::getline(f, line)) music = std::stoi(line);
        if (std::getline(f, line)) sound = std::stoi(line);
        if (std::getline(f, line)) monster = std::stoi(line);
        if (std::getline(f, line)) playerMelee = std::stoi(line);
        return true;
    } catch (...) { return false; }
}

bool Database::saveTheme(int userId, const std::string& themeName) const {
    try {
        std::ofstream f(userThemePath(userId), std::ios::out | std::ios::trunc);
        if (!f) return false; f << themeName << "\n"; return true;
    } catch (...) { return false; }
}

bool Database::loadTheme(int userId, std::string& outThemeName) const {
    outThemeName.clear();
    try {
        std::ifstream f(userThemePath(userId)); if (!f) return false; std::getline(f, outThemeName); return !outThemeName.empty();
    } catch (...) { return false; }
}


