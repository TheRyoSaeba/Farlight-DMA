#define NOMINMAX
#include <Cache/Game.h>
#include <Cache/bones.h>
#include <algorithm>
#include "ItemESP.h"
#include <Utils/Utils.h>
#include <DMALibrary/Memory/Memory.h>
#include <Utils/Globals.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "Aimbot/Aimbot.h"

static std::unordered_map<int, std::string> FCACHE;
static std::shared_mutex fnameCacheMutex;

static std::unordered_map<uintptr_t, std::string> CLASSCACHE;
static std::shared_mutex classCacheMutex;

static std::unordered_map<uintptr_t, PlayerRender> AttributeCache;
static std::shared_mutex AttributeMutex;


_GameCache cache;


auto CacheFNAME = [](int id) -> std::string {
    return FCACHE.count(id) ? FCACHE[id] : (FCACHE[id] = GetNameFromFName(id));
    };

 

static std::string CacheClassName(uintptr_t objOrUClass) {
    if (!util::IsValidVA(objOrUClass)) return {};
    uintptr_t uclass = mem.Read<uintptr_t>(objOrUClass + 0x10);
    if (!util::IsValidVA(uclass)) uclass = objOrUClass;
    if (!util::IsValidVA(uclass)) return {};
    {
        std::shared_lock lock(classCacheMutex);
        auto it = CLASSCACHE.find(uclass);
        if (it != CLASSCACHE.end()) return it->second;
    }
    int nameId = mem.Read<int>(uclass + 0x18);
    std::string cname = CacheFNAME(nameId);
    {
        std::unique_lock lock(classCacheMutex);
        CLASSCACHE.emplace(uclass, cname);
    }
    return cname;
}

bool Get_Uworld() {
    uint64_t base = mem.GetBaseDaddy("SolarlandClient-Win64-Shipping.exe");
    uint64_t size = mem.GetBaseSize("SolarlandClient-Win64-Shipping.exe");

    uint64_t uworldAddr = mem.FindSignature("48 8B 1D ? ? ? ? 48 85 DB 74 ? 41 B0", base, base + size);

    uint64_t gnamesAddr = mem.FindSignature("48 8D 05 ? ? ? ? EB ? 48 8D 0D ? ? ? ? E8", base, base + size);

    uint64_t gobjectsAddr = mem.FindSignature("48 8B 05 ? ? ? ? 4A 8B ", base, base + size);

    if (!uworldAddr || !gnamesAddr) {
        std::cout << "Signature failed!" << std::endl;
        return false;
    }

    int32_t uworldDisp = mem.Read<int32_t>(uworldAddr + 3);
    int32_t gnamesDisp = mem.Read<int32_t>(gnamesAddr + 3);
    int32_t gobjectsDisp = mem.Read<int32_t>(gobjectsAddr + 3);

    Globals.offsets.uworld = (uworldAddr + 7 + uworldDisp) - base;
    Globals.offsets.GNAMES = (gnamesAddr + 7 + gnamesDisp) - base;
    Globals.offsets.GOBJECTS = (gobjectsAddr + 7 + gobjectsDisp) - base;

    std::cout << "UWorld: 0x" << std::hex << Globals.offsets.uworld << std::endl;
    std::cout << "GNAMES: 0x" << std::hex << Globals.offsets.GNAMES << std::endl;
    //std::cout << "GOBJECTS: 0x" << std::hex << Globals.offsets.GOBJECTS << std::dec << std::endl;

    return (util::IsValidVA(Globals.offsets.uworld) && util::IsValidVA(Globals.offsets.GNAMES));

};


void Game::Loop() {

    UpdateCore(true);

    std::thread(Game::UpdateActorsLoop).detach();

    ItemESP::Start(true);

  
     
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    auto processFrame = [&]() -> bool {
        static bool debugBones = false;

        UpdateLocal(false);
        UpdateAttributes(false);

        if (!util::IsValidVA(cache.LocalPawn.load()) || !util::IsValidVA(cache.PersistentLevel.load()))
            return false;

		 
        std::vector<uintptr_t> actorsCopy;
        {
            std::unique_lock cacheLock(cache.cacheMutex);
            actorsCopy = cache.Actors;
        }

        if (actorsCopy.size() <= 2) return false;

        ItemESP::Render(cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);


        auto scatter = mem.CreateScatterHandle();

        struct PlayerFrameData {
            uintptr_t mesh = 0;
            TArray<FTransform> boneArray{};
            FTransform componentToWorld{};
            FTransform headBone{};
            FTransform rootBone{};
            std::unordered_map<int, FTransform> skeletonBones;
        };

        std::vector<PlayerFrameData> frameData(actorsCopy.size());

      
        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (!util::IsValidVA(actorsCopy[i]) || actorsCopy[i] == cache.LocalPawn.load()) continue;
            mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.mesh, &frameData[i].mesh, sizeof(uintptr_t));
        }
        mem.ExecuteReadScatter(scatter);
        auto scatterEnd = std::chrono::high_resolution_clock::now();
      
        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (!util::IsValidVA(frameData[i].mesh)) continue;
            mem.AddScatterReadRequest(scatter, frameData[i].mesh + Globals.offsets.boneArray, &frameData[i].boneArray, sizeof(TArray<FTransform>));
            mem.AddScatterReadRequest(scatter, frameData[i].mesh + Globals.offsets.componentToWorld, &frameData[i].componentToWorld, sizeof(FTransform));
        }
       
        mem.ExecuteReadScatter(scatter);
      

        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (!frameData[i].boneArray.isValid() || frameData[i].boneArray.Num() <= 7) continue;
            uintptr_t boneArrayAddr = frameData[i].boneArray.getAddress();

            mem.AddScatterReadRequest(scatter, boneArrayAddr + (0 * sizeof(FTransform)), &frameData[i].rootBone, sizeof(FTransform));

            if (Globals.DrawBones && frameData[i].boneArray.Num() >= 70) {
               
                frameData[i].skeletonBones.reserve(SKELETON_BONES.size());
                for (BoneID bone : SKELETON_BONES) {
                    int boneIdx = static_cast<int>(bone);
                    mem.AddScatterReadRequest(scatter, boneArrayAddr + (boneIdx * sizeof(FTransform)),
                        &frameData[i].skeletonBones[boneIdx], sizeof(FTransform));
                }
            } else {
                
                mem.AddScatterReadRequest(scatter, boneArrayAddr + (7 * sizeof(FTransform)), &frameData[i].headBone, sizeof(FTransform));
            }
        }
        mem.ExecuteReadScatter(scatter);
        mem.CloseScatterHandle(scatter);

       
        std::vector<PlayerRender> playerSnapshot;
        playerSnapshot.reserve(actorsCopy.size());

        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (!frameData[i].boneArray.isValid()) continue;

            auto calcDistance = [&](const Vector3& pos) {
                float dx = pos.x - cache.LocalCamera.Location.x;
                float dy = pos.y - cache.LocalCamera.Location.y;
                float dz = pos.z - cache.LocalCamera.Location.z;
                return std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0f;
                };

            Vector2 root = doMatrix(frameData[i].rootBone, frameData[i].componentToWorld, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);

            PlayerRender pr{};
            pr.Actor = actorsCopy[i];
            pr.mesh = frameData[i].mesh;
            pr.bottomW2S = root;
            pr.distance = calcDistance(frameData[i].componentToWorld.translation);

            if (Globals.DrawBones && !frameData[i].skeletonBones.empty()) {
                pr.hasSkeletonData = true;
                PopulateSkeletonBones(pr.skeleton, frameData[i].skeletonBones,
                    [&](const FTransform& bone) {
                        return doMatrix(bone, frameData[i].componentToWorld, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
                    });
                pr.headW2S = pr.skeleton.head;

            } else {

                Vector2 head = doMatrix(frameData[i].headBone, frameData[i].componentToWorld, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
                pr.headW2S = head;
            }

            playerSnapshot.push_back(std::move(pr));
        }
        {
            std::unique_lock lock(Globals.playerMutex);
            std::shared_lock attrLock(AttributeMutex);

            std::unordered_set<uintptr_t> currentSet(actorsCopy.begin(), actorsCopy.end());
            Globals.renderPlayers.erase(
                std::remove_if(Globals.renderPlayers.begin(), Globals.renderPlayers.end(),
                    [&](const PlayerRender& p) { return !currentSet.count(p.Actor); }),
                Globals.renderPlayers.end()
            );

            std::unordered_map<uintptr_t, PlayerRender*> renderMap;
            std::for_each(Globals.renderPlayers.begin(), Globals.renderPlayers.end(),
                [&](PlayerRender& p) { renderMap[p.Actor] = &p; });

            auto applyAttributes = [&](PlayerRender& pr) {
                if (auto ait = AttributeCache.find(pr.Actor); ait != AttributeCache.end()) {
                    pr.TeamId = ait->second.TeamId;
                    pr.AliveDeadorKnocked = ait->second.AliveDeadorKnocked;
                    pr.Health = ait->second.Health;
                    pr.Name = ait->second.Name;
                }
                };

            std::for_each(playerSnapshot.begin(), playerSnapshot.end(), [&](PlayerRender& pr) {
                if (auto it = renderMap.find(pr.Actor); it != renderMap.end()) {
                    it->second->mesh = pr.mesh;
                    it->second->headW2S = pr.headW2S;
                    it->second->bottomW2S = pr.bottomW2S;
                    it->second->distance = pr.distance;
                    it->second->skeleton = pr.skeleton;
                    it->second->hasSkeletonData = pr.hasSkeletonData;
                    applyAttributes(*it->second);
                }
                else {
                    applyAttributes(pr);
                    Globals.renderPlayers.push_back(std::move(pr));
                }
                });
        }

        Vector2 best{ 0.f, 0.f };
        float bestDist = FLT_MAX;
        uintptr_t bestMesh = 0;

        uintptr_t localPawn = cache.LocalPawn.load();

        for (const auto& pr : Globals.renderPlayers) {
            if (pr.Actor == localPawn) continue;
            if (!util::IsValidVA(pr.mesh)) continue;
            if (pr.AliveDeadorKnocked == ECharacterHealthState::ECHS_Dead) continue;
            if (Globals.settings.IgnoreKnocked && pr.AliveDeadorKnocked == ECharacterHealthState::ECHS_Knocked) continue;
            if (pr.distance > Globals.settings.AimbotMaxDistance) continue;
            if (pr.distance <= 0.01f) continue;


            if (!std::isfinite(pr.headW2S.x) || !std::isfinite(pr.headW2S.y)) continue;
            if (pr.headW2S.x < 0.0f || pr.headW2S.x > Globals.screenWidth ||
                pr.headW2S.y < 0.0f || pr.headW2S.y > Globals.screenHeight) {
                continue;
            }

            Vector2 targetPoint;
            if (Globals.TargetPriority == ETargetPriority::Head) {
                targetPoint = pr.headW2S;
                targetPoint.y += 5.0f;
            }
            else {
                const float t = 0.5f;
                targetPoint.x = pr.bottomW2S.x * (1.0f - t) + pr.headW2S.x * t;
                targetPoint.y = pr.bottomW2S.y * (1.0f - t) + pr.headW2S.y * t;
            }

           
            if (!std::isfinite(targetPoint.x) || !std::isfinite(targetPoint.y)) continue;

            float d = GetCrossDistance(targetPoint.x, targetPoint.y,
                Globals.screenWidth / 2.0f, Globals.screenHeight / 2.0f);

            float fovPixels = (Globals.settings.fov / 180.0f) * (Globals.screenWidth * 0.5f);

            if (d < fovPixels && d < bestDist) {
                bestDist = d;
                best = targetPoint;
                bestMesh = pr.mesh;
            }
        }
  
        if (util::IsValidVA(bestMesh))
            Aimbot::aimbot(best);

        

        return true; };

      

    // Main loop
    static double avgMs = 16.0;
    while (true) {
        if (Globals.refreshcheat) {
            Globals.refreshcheat = false;
            mem.FullRefresh();
            UpdateCore(true);
            continue;
        }

        auto start = std::chrono::steady_clock::now();
        processFrame();
        auto end = std::chrono::steady_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        avgMs = avgMs * 0.9 + ms * 0.1;
        Globals.readFPS = static_cast<int>(1000.0 / avgMs + 0.5);

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void Game::UpdateCore(bool debug) {
    Globals.renderPlayers.clear();
    Globals.renderItems.clear();
    auto base = mem.GetBaseDaddy(Globals.processName);
    auto readCoreData = [&]() -> std::optional<std::tuple<uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t>> {
        uintptr_t newUWorld = mem.Read<uintptr_t>(base + Globals.offsets.uworld);
        if (!util::IsValidVA(newUWorld))  return std::nullopt;

        auto core_handle = mem.CreateScatterHandle();
        uintptr_t newGameInstance, persistentlevel;
        mem.AddScatterReadRequest(core_handle, newUWorld + Globals.offsets.gameInstance, &newGameInstance, sizeof(uintptr_t));
        mem.AddScatterReadRequest(core_handle, newUWorld + Globals.offsets.persistentlevel, &persistentlevel, sizeof(uintptr_t));
        mem.ExecuteReadScatter(core_handle);
        mem.CloseScatterHandle(core_handle);

        if (!util::IsValidVA(newGameInstance) || !util::IsValidVA(persistentlevel)) return std::nullopt;

        uintptr_t newLocalPlayers = mem.Read<uintptr_t>(mem.Read<uintptr_t>(newGameInstance + Globals.offsets.playersArray));
        uintptr_t newPlayerController = mem.Read<uintptr_t>(newLocalPlayers + Globals.offsets.playerController);
        uintptr_t newPawn = mem.Read<uintptr_t>(newPlayerController + Globals.offsets.localPawn);
        uintptr_t newCameraManager = mem.Read<uintptr_t>(newPlayerController + Globals.offsets.playerCameraManager);

        if (!util::IsValidVA(newLocalPlayers) || !util::IsValidVA(newPlayerController) ||
            !util::IsValidVA(newPawn) || !util::IsValidVA(newCameraManager)) return std::nullopt;

        return std::make_tuple(newUWorld, newGameInstance, newPlayerController, persistentlevel, newPawn, newCameraManager);
        };

    while (true) {
        auto result = readCoreData();
        if (!result) continue;

        auto [newUWorld, newGameInstance, newPlayerController, persistentlevel, newPawn, newCameraManager] = *result;

        {
            std::unique_lock lock(cache.cacheMutex);
            cache.UWorld.store(newUWorld);
            cache.GameInstance.store(newGameInstance);
            cache.PlayerController.store(newPlayerController);
            cache.PersistentLevel.store(persistentlevel);
            cache.LocalPawn.store(newPawn);
            cache.CameraManager.store(newCameraManager);
        }

        if (debug) {
            std::cout << "Core pointers updated:\nUWorld: 0x" << std::hex << newUWorld
                << " GameInstance: 0x" << newGameInstance << "\n LocalPawn: 0x" << newPawn << std::dec << std::endl;
        }
        break;
    }
    UpdateActors(true);
}

void Game::UpdateActorsLoop() {
    while (true) {
        UpdateActors(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    }
}



void Game::UpdateActors(bool debug) {
    uintptr_t uWorldCheck = mem.Read<uintptr_t>(mem.baseAddress + Globals.offsets.uworld);

    if (util::IsValidVA(uWorldCheck) && uWorldCheck == cache.UWorld.load()) {
        uintptr_t uLevelCheck = mem.Read<uintptr_t>(uWorldCheck + Globals.offsets.persistentlevel);

        if (util::IsValidVA(uLevelCheck) && uLevelCheck == cache.PersistentLevel.load()) {
            TArray<uintptr_t> actorArray = mem.Read<TArray<uintptr_t>>(cache.PersistentLevel + Globals.offsets.AActors);

            if (actorArray.isValid() && actorArray.Num() > 0) {
                std::vector<uintptr_t> rawActors(actorArray.Num());
                if (!mem.Read(actorArray.getAddress(), rawActors.data(), rawActors.size() * sizeof(uintptr_t))) return;

                std::vector<uintptr_t> validPlayers;
                std::unordered_set<uintptr_t> validItems;
                std::vector<int> nameIDs(rawActors.size());

                auto scatter = mem.CreateScatterHandle();
                for (size_t i = 0; i < rawActors.size(); i++) {
                    if (util::IsValidVA(rawActors[i]) && rawActors[i] != cache.LocalPawn.load()) {
                        mem.AddScatterReadRequest(scatter, rawActors[i] + 0x18, &nameIDs[i], sizeof(int));
                    }
                }
                mem.ExecuteReadScatter(scatter);
                mem.CloseScatterHandle(scatter);

                for (size_t i = 0; i < rawActors.size(); i++) {
                    if (nameIDs[i] <= 0) continue;

                    std::string name = CacheFNAME(nameIDs[i]);
                    if (name.empty()) continue;

                    bool isPlayer = (name.find("BP_Character") != std::string::npos ||
                                    name.find("SolarCharacter") != std::string::npos);

                    if (isPlayer) {
                        validPlayers.push_back(rawActors[i]);
                    }
                    else if (Globals.itemsEnabled) {
                        validItems.insert(rawActors[i]);
                    }
                }

                if (debug) {
                    std::cout << "Actor scan - Total: " << rawActors.size()
                              << ", Players: " << validPlayers.size() << std::endl;
                }

                {
                    std::unique_lock lock(cache.cacheMutex);
                    cache.Actors = std::move(validPlayers);
                    cache.cachedItemActors = std::move(validItems);
                    cache.lastActorList = std::move(rawActors);
                }
                return;
            }
        }
    }

    UpdateCore(false);
}

void Game::UpdateLocal(bool debug) {
   
    auto scatter = mem.CreateScatterHandle();
    mem.AddScatterReadRequest(scatter, cache.PlayerController + Globals.offsets.localPawn, &cache.LocalPawn, sizeof(uintptr_t));
    mem.AddScatterReadRequest(scatter, cache.CameraManager.load() + Globals.offsets.playerCameraCache + Globals.offsets.playerCameraPOV,
        &cache.LocalCamera, sizeof(Camera));
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);
}

void Game::UpdateAttributes(bool debug) {
    static auto lastUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 2000) return;
    lastUpdate = now;

    std::vector<uintptr_t> actorsCopy;
    {
        std::unique_lock lock(cache.cacheMutex);
        actorsCopy = cache.Actors;
    }

    if (actorsCopy.size() < 2) {
        UpdateCore(false);
        return;
    }

   
    auto scatter = mem.CreateScatterHandle();
    std::vector<uintptr_t> meshes(actorsCopy.size(), 0);
    std::vector<uintptr_t> states(actorsCopy.size(), 0);
    std::vector<uintptr_t> abilities(actorsCopy.size(), 0);

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.mesh, &meshes[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.Actor_State, &states[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.AbilitySystemComponent, &abilities[i], sizeof(uintptr_t));
    }
    mem.ExecuteReadScatter(scatter);

   
    std::vector<uintptr_t> teamInfos(actorsCopy.size(), 0);
    std::vector<int> teamIds(actorsCopy.size(), 0);
    std::vector<ECharacterHealthState> healthStates(actorsCopy.size());
    std::vector<TArray<uintptr_t>> spawnedAttrs(actorsCopy.size());
    std::vector<FString> playerNames(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(states[i])) continue;
        mem.AddScatterReadRequest(scatter, states[i] + Globals.offsets.ASolarTeamInfo, &teamInfos[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, states[i] + Globals.offsets.CharacterHealthState, &healthStates[i], sizeof(ECharacterHealthState));
        mem.AddScatterReadRequest(scatter, states[i] + Globals.offsets.PlayerName, &playerNames[i], sizeof(FString));
        if (util::IsValidVA(abilities[i])) {
            mem.AddScatterReadRequest(scatter, abilities[i] + 0x150, &spawnedAttrs[i], sizeof(TArray<uintptr_t>));
        }
    }
    mem.ExecuteReadScatter(scatter);

   
    std::vector<uintptr_t> firstAttrs(actorsCopy.size(), 0);
    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (util::IsValidVA(teamInfos[i])) {
            mem.AddScatterReadRequest(scatter, teamInfos[i] + Globals.offsets.teamState, &teamIds[i], sizeof(int));
        }
        if (spawnedAttrs[i].isValid() && spawnedAttrs[i].Num() > 0) {
            mem.AddScatterReadRequest(scatter, spawnedAttrs[i].getAddress(), &firstAttrs[i], sizeof(uintptr_t));
        }
    }
    mem.ExecuteReadScatter(scatter);

   
    std::vector<float> healths(actorsCopy.size(), 0.0f);
    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (util::IsValidVA(firstAttrs[i])) {
            mem.AddScatterReadRequest(scatter, firstAttrs[i] + 0x54, &healths[i], sizeof(float));
        }
    }
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);

    std::vector<PlayerRender> renders;
    renders.reserve(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(states[i])) continue;

        PlayerRender pr{};
        pr.Actor = actorsCopy[i];
        pr.mesh = meshes[i];
        pr.TeamId = teamIds[i];
        pr.AliveDeadorKnocked = healthStates[i];
        pr.Health = healths[i];


        if (playerNames[i].IsValid() && playerNames[i].Count > 0 && playerNames[i].Count < 32) {
            std::vector<wchar_t> buf(playerNames[i].Count + 1, 0);
            if (mem.Read((uintptr_t)playerNames[i].Data, buf.data(), playerNames[i].Count * sizeof(wchar_t))) {
                std::wstring ws(buf.data());
                pr.Name = std::string(ws.begin(), ws.end());
            }
        }

        renders.push_back(std::move(pr));
    }

    {
        std::unique_lock attrLock(AttributeMutex);
        AttributeCache.clear();
        for (auto& pr : renders) {
            AttributeCache[pr.Actor] = pr;
        }
    }

    if (debug) {
        std::cout << "Attributes updated for " << renders.size() << " players" << std::endl;
    }
}