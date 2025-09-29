 
#pragma once

#include <Utils/Utils.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <cstdint>
#include <chrono>      

 
class Camera;
 
enum class EWeaponType : uint8_t {
    AssualtRifle = 0,
    Submachinegun = 1,
    Shotgun = 2,
    Sniper = 3,
    VehicleMounted = 4,
    ItemWeapon = 5,
    SummonWeapon = 6,
    AntiVehicle = 7,
    ShooterRifle = 8,
    HandGun = 9,
    LightMachineGun = 10,
    Unarm = 11,
    Unknown = 12,
    EWeaponType_MAX = 13
};


 


enum class EItemType : uint32_t {
    NONE = 0,
    SHIELD = 101,
    ARMOR = 102,
    BULLET = 103,
    CARIRIDGE_BAG = 104,
    ARMOR_MATERIAL = 105,
    ENERGY_MODULE = 106,
    EXTRA_ENERGY = 107,
    RADAR_OPERATOR = 108,
    BACKPACK_ENERGY = 109,
    BACKUP_ENERGY = 110,
    SHIELD_RECHARGER = 112,
    BACKPACK_ITEM = 113,
    WEAPON_PARTS = 114,
    JETPACK_MODULE_HORIZONTAL = 111,
    JETPACK_MODULE_VERTICAL = 115,
    REVIVE_ITEM = 116,
    SELF_RESCUE = 118,
    TREASUREBOX = 120,
    AIRDROPBOX = 121,
    DEATHBOX = 122,
    HOTSPRINTBOX = 123,
    TACTICALBOX = 124,
    NEUTRAL_CARD = 130,
    COLLECTION_ITEM = 131,
    DRAGONBALL = 132,
    TALENT_POINT = 133,
    COLLECTION_CHEST = 134,
    KEY_CARD = 135,
    HELMET = 140,
    ENHANCER_AMMO = 141,
    ENHANCER_MEDIC = 142,
    ENHANCER_SHIELD_RECHARGER = 143,
    ENHANCER_BACKPACK = 144,
    TACTICAL_EQUIPMENTS = 145,
    SHIELD_UPGRADE_MATERIAL = 148,
    EXP_ITEM = 149,
    EXP_TOKEN = 150,
    WEAPON = 151,
    WEAPON_SKIN = 171,
    MISSIONSPAWN = 160,
    MISSIONCHEST = 161,
    MISSIONWORSHIP = 162,
    BACKPACK = 201,
    TAILFLAME = 202,
    CARDPOSE = 203,
    CARDBACKGROUND = 204,
    CAPSULE = 251,
    CHAR_SKIN_MIN = 301,
    CHAR_ANIMATION_MVP = 302,
    CHAR_SKIN_MAX = 350,
    CHARACTER = 351,
    EXPERIENCE = 401,
    GIFTBAG = 404,
    CHARACTER_TRIALCARD = 405,
    CHARACTERSKIN_TRIALCARD = 406,
    ACTIVENESS = 411,
    WEAPONSKIN_TRIALCARD = 412,
    GIFTBAG_ONBACKPACK = 414,
    BACKPACK_TRIALCARD = 415,
    TAILFLAME_TRIALCARD = 416,
    COUPON = 417,
    LOTCOIN = 421,
    ZOMBORG = 422,
    WISHCOIN = 423,
    SURPRISECOIN = 424,
    STARCOIN = 428,
    TOKEN = 430,
    BUSINESSCARDFRAME = 432,
    AVATARFRAME = 434,
    CHARACTER_SHARD = 435,
    CHARACTER_SKIN_SHARD = 436,
    WEAPON_SKIN_SHARD = 437,
    BACKPACK_SHARD = 438,
    TAILFLAME_SHARD = 439,
    CAPSULE_SHARD = 440,
    VEHICLE_SKIN_SHARD = 441,
    ACCOUNT_AVATAR = 443,
    EMOTE = 444,
    SIGNIN_CARD = 447,
    RAFFLE_TICKET = 448,
    BUDDYBALL = 449,
    VEHICLE_SKIN = 701,
    SUPPLYBOX = 801,
    RANDOM_PACK = 901,
    DISPLAY_ITEM = 999,
    EItemType_MAX = 1000
};

enum class EItemCategory {
    NONE,
    SHIELDS,
    ARMORS,
    AMMO,
    ENERGY,
    BACKPACKS,
    WEAPONS,
    ENHANCERS,
    REVIVE,
    LOOTBOXES,
    QUEST,
    MISC,
    MAX
};
inline EItemCategory GetItemCategory(EItemType itemType) {
    switch (itemType) {
    case EItemType::SHIELD:
    case EItemType::SHIELD_RECHARGER:
    case EItemType::SHIELD_UPGRADE_MATERIAL:
        return EItemCategory::SHIELDS;

    case EItemType::ARMOR:
    case EItemType::ARMOR_MATERIAL:
    case EItemType::HELMET:
        return EItemCategory::ARMORS;

    case EItemType::BULLET:
    case EItemType::CARIRIDGE_BAG:
        return EItemCategory::AMMO;

    case EItemType::ENERGY_MODULE:
    case EItemType::EXTRA_ENERGY:
    case EItemType::BACKPACK_ENERGY:
    case EItemType::BACKUP_ENERGY:
    case EItemType::JETPACK_MODULE_HORIZONTAL:
    case EItemType::JETPACK_MODULE_VERTICAL:
        return EItemCategory::ENERGY;

    case EItemType::BACKPACK:
    case EItemType::BACKPACK_ITEM:
        return EItemCategory::BACKPACKS;

    case EItemType::WEAPON:
    case EItemType::WEAPON_PARTS:
    case EItemType::WEAPON_SKIN:
        return EItemCategory::WEAPONS;

    case EItemType::ENHANCER_AMMO:
    case EItemType::ENHANCER_MEDIC:
    case EItemType::ENHANCER_SHIELD_RECHARGER:
    case EItemType::ENHANCER_BACKPACK:
        return EItemCategory::ENHANCERS;

    case EItemType::REVIVE_ITEM:
    case EItemType::SELF_RESCUE:
        return EItemCategory::REVIVE;

    case EItemType::TREASUREBOX:
    case EItemType::AIRDROPBOX:
    case EItemType::DEATHBOX:
    case EItemType::HOTSPRINTBOX:
    case EItemType::TACTICALBOX:
    case EItemType::MISSIONCHEST:
    case EItemType::SUPPLYBOX:
        return EItemCategory::LOOTBOXES;

    case EItemType::MISSIONSPAWN:
    case EItemType::MISSIONWORSHIP:
    case EItemType::NEUTRAL_CARD:
    case EItemType::COLLECTION_ITEM:
    case EItemType::DRAGONBALL:
    case EItemType::TALENT_POINT:
    case EItemType::COLLECTION_CHEST:
    case EItemType::KEY_CARD:
        return EItemCategory::QUEST;

    default:
        return EItemCategory::MISC;
    }
}
#pragma pack(push, 1)
struct FSolarItemData {
    char      _pad0[0x0C];
    int32_t   ItemID;
    int64_t   ThisID;
    FString   Name;
    FString   Icon;
    FString   Info;
    int32_t   Count;
    EItemType ItemType;
    char      _padEnd[3];
};
#pragma pack(pop)



struct ItemRenderer {
    uintptr_t Actor = 0;
    Vector2   W2S;          
    Vector3   Location;     
    float     distance = 0.0f;
    EItemType Item = EItemType::NONE;
    EWeaponType Weapon = EWeaponType::Unknown;
    int32_t   weaponid = 0;
    std::string Name;
};

 
class ItemESP {
public:
    static void Start(bool dbg = false);
    static void Stop();
    static void Render(const Camera& cam, int screenW, int screenH); 
private:
    static void RefreshLoop();     
    static void RefreshList();      
    static std::unordered_set<uintptr_t> oldItemSet;
    static std::unordered_map<uintptr_t, ItemRenderer> cachedItems;
    static std::thread refreshThread;
    static bool running;
    static bool debug;
    static constexpr int REFRESH_MS = 1000; 
};