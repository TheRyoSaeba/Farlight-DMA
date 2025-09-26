#pragma once
#define NOMINMAX
#include <Utils/Utils.h>
#include <cstdint>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
bool Get_Uworld();

 
#ifdef __INTELLISENSE__
#define DEFINE_MEMBER_0(t, n) t n
#define DEFINE_MEMBER_N(o, t, n) t n
#else
#define STR_MERGE_IMPL(a, b) a##b
#define STR_MERGE(a, b) STR_MERGE_IMPL(a, b)
#define MAKE_PAD(size) STR_MERGE(_pad, __COUNTER__)[size]
#define DEFINE_MEMBER_0(t, n) struct { t n; }
#define DEFINE_MEMBER_N(o, t, n) struct { unsigned char MAKE_PAD(o); t n; }
#endif


namespace Game {
   void Loop();
   void UpdateCore(bool debug);
   void UpdateActors(bool debug);
   void UpdateLocal(bool debug);
   void UpdateAttributes(bool debug);
   void UpdateItems(bool debug, const Camera& cam, int screenWidth, int screenHeight);
   void UpdateActorsLoop();
   void UpdateItemsLoop();
}

 
enum class ECharacterHealthState : uint8_t
{
	ECHS_Alive = 0,
	ECHS_Knocked = 1,
	ECHS_Dead = 2,
};


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

 
#pragma pack(push, 1)
struct FSolarItemData {
    char      _pad0[0x0C];   // 0x00
    int32_t   ItemID;        // 0x0C
    int64_t   ThisID;        // 0x10
    FString   Name;          // 0x18
    FString   Icon;          // 0x28
    FString   Info;          // 0x38
    int32_t   Count;         // 0x48
    EItemType ItemType;      // 0x4C
    char      _padEnd[3];    // 0x4D - 0x4F
};
#pragma pack(pop)

struct ItemEntry {
    uintptr_t actor = 0;
    int32_t ItemID = 0;
    int64_t ThisID = 0;
    EItemType ItemType = EItemType::NONE;
    int32_t Count = 0;
    std::string Name;
    std::string Icon;
    std::string Info;
    EWeaponType WeaponType = EWeaponType::Unknown;
    int32_t weaponid = 0;
};

struct ItemRenderer
{
    uintptr_t Actor = 0;
    Vector2 W2S;          // screen position
    Vector3 Location;     // world position
    float distance;       // distance from player
    EItemType Item;       // item type
    EWeaponType Weapon;   // weapon type if applicable
    int32_t weaponid = 0; // weapon id if applicable
    std::string Name;     // item name
};;

struct  PlayerRender {
	uintptr_t Actor = 0;
	uintptr_t mesh = 0;
	Vector2 headW2S;      
	Vector2 bottomW2S;     
	Vector3 Location;
	float Health;
	float distance;
	bool IsVisible;
	int TeamId;
	ECharacterHealthState AliveDeadorKnocked;

	std::string Name;
};







 
 