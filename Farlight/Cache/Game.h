#pragma once
#define NOMINMAX
#include <Utils/Utils.h>
#include <cstdint>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "bones.h"
#include "ItemESP.h" 
 
 
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

 

bool Get_Uworld();

 
 

namespace Game {
   void Loop();
   void UpdateCore(bool debug);
   void UpdateActors(bool debug);
   void UpdateLocal(bool debug);
   void UpdateAttributes(bool debug);
   void UpdateActorsLoop();
}

 
enum class ECharacterHealthState : uint8_t
{
	ECHS_Alive = 0,
	ECHS_Knocked = 1,
	ECHS_Dead = 2,
};


 
 
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

	SkeletonBones skeleton;
	bool hasSkeletonData = false;
};

 

struct _GameCache {
	std::atomic<uintptr_t> UWorld{ 0 };
	std::atomic<uintptr_t> GameInstance{ 0 };
	std::atomic<uintptr_t> GameState{ 0 };
	std::atomic<uintptr_t> PlayerController{ 0 };
	std::atomic<uintptr_t> PersistentLevel{ 0 };
	std::atomic<uintptr_t> LocalPawn{ 0 };
	std::atomic<uintptr_t> CameraManager{ 0 };
	Camera LocalCamera{};
	std::vector<uintptr_t> Actors;
	std::vector<uintptr_t> lastActorList;
	std::unordered_set<uintptr_t> cachedItemActors;
	std::mutex cacheMutex;
};
extern _GameCache cache;