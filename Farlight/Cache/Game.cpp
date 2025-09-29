#define NOMINMAX
#include <Cache/Game.h>
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
//updatecore spam 

void Game::Loop() {
    UpdateCore(true);


    std::thread(Game::UpdateActorsLoop).detach();

    ItemESP::Start(true);
     
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto processFrame = [&]() -> bool {

        UpdateLocal(false);

        UpdateAttributes(false);

        if (!util::IsValidVA(cache.LocalPawn.load()) || !util::IsValidVA(cache.PersistentLevel.load()))
            return false;


        ItemESP::Render(cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);


        std::vector<uintptr_t> actorsCopy;
        {
            std::unique_lock cacheLock(cache.cacheMutex);
            actorsCopy = cache.Actors;
        }

        if (actorsCopy.size() <= 2) return false;

       
        auto scatter = mem.CreateScatterHandle();

        struct PlayerFrameData {
            uintptr_t mesh;
            TArray<FTransform> boneArray;
            FTransform componentToWorld;
            FTransform headBone; 
            FTransform rootBone;
        };

        std::vector<PlayerFrameData> frameData(actorsCopy.size());

      
        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (!util::IsValidVA(actorsCopy[i]) || actorsCopy[i] == cache.LocalPawn.load()) continue;
            mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.mesh, &frameData[i].mesh, sizeof(uintptr_t));
        }
        mem.ExecuteReadScatter(scatter);

      
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
            mem.AddScatterReadRequest(scatter, boneArrayAddr + (7 * sizeof(FTransform)), &frameData[i].headBone, sizeof(FTransform));
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

            Vector2 head = doMatrix(frameData[i].headBone, frameData[i].componentToWorld, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
            Vector2 root = doMatrix(frameData[i].rootBone, frameData[i].componentToWorld, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);

            PlayerRender pr{};
            pr.Actor = actorsCopy[i];
            pr.mesh = frameData[i].mesh;
            pr.headW2S = head;
            pr.bottomW2S = root;
            pr.distance = calcDistance(frameData[i].componentToWorld.translation);

            playerSnapshot.push_back(std::move(pr));
        }

       
        {
            std::unique_lock lock(Globals.playerMutex);

            
            std::unordered_set<uintptr_t> currentSet(actorsCopy.begin(), actorsCopy.end());
            Globals.renderPlayers.erase(
                std::remove_if(Globals.renderPlayers.begin(), Globals.renderPlayers.end(),
                    [&](const PlayerRender& p) {
                        return p.Actor && currentSet.find(p.Actor) == currentSet.end();
                    }),
                Globals.renderPlayers.end()
            );

          
            std::unordered_map<uintptr_t, size_t> idx;
            idx.reserve(Globals.renderPlayers.size() + playerSnapshot.size());
            for (size_t i = 0; i < Globals.renderPlayers.size(); ++i)
                idx.emplace(Globals.renderPlayers[i].Actor, i);

          
            std::shared_lock attrLock(AttributeMutex);

            for (auto& pr : playerSnapshot) {
                auto it = idx.find(pr.Actor);
                if (it != idx.end()) {
                    PlayerRender& existing = Globals.renderPlayers[it->second];
                  
                    existing.mesh = pr.mesh;
                    existing.headW2S = pr.headW2S;
                    existing.bottomW2S = pr.bottomW2S;
                    existing.distance = pr.distance;

                    
                    auto ait = AttributeCache.find(pr.Actor);
                    if (ait != AttributeCache.end()) {
                        existing.TeamId = ait->second.TeamId;
                        existing.AliveDeadorKnocked = ait->second.AliveDeadorKnocked;
                        existing.Health = ait->second.Health;
                        existing.Name = ait->second.Name;
                    }
                }
                else {
                   
                    PlayerRender combined = pr;
                    auto ait = AttributeCache.find(pr.Actor);
                    if (ait != AttributeCache.end()) {
                        combined.TeamId = ait->second.TeamId;
                        combined.AliveDeadorKnocked = ait->second.AliveDeadorKnocked;
                        combined.Health = ait->second.Health;
                        combined.Name = ait->second.Name;
                    }//f
                    Globals.renderPlayers.push_back(std::move(combined));
                    idx.emplace(Globals.renderPlayers.back().Actor, Globals.renderPlayers.size() - 1);
                }
            }
        }

        Vector2 best{ 0.f, 0.f };
        float bestDist = FLT_MAX;
        uintptr_t bestMesh = 0;

        uintptr_t localPawn = cache.LocalPawn.load();

        for (const auto& pr : Globals.renderPlayers) {
            if (pr.Actor == localPawn) continue;                              
            if (!util::IsValidVA(pr.mesh)) continue;
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
            }
            else {
                const float t = 0.5f;
                targetPoint.x = pr.bottomW2S.x * (1.0f - t) + pr.headW2S.x * t;
                targetPoint.y = pr.bottomW2S.y * (1.0f - t) + pr.headW2S.y * t;
            }

           
            if (!std::isfinite(targetPoint.x) || !std::isfinite(targetPoint.y)) continue;

            float d = GetCrossDistance(targetPoint.x, targetPoint.y,
                Globals.screenWidth / 2.0f, Globals.screenHeight / 2.0f);

            if (d < Globals.settings.fov && d < bestDist) {
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

    auto readCoreData = [&]() -> std::optional<std::tuple<uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t>> {
        uintptr_t newUWorld = mem.Read<uintptr_t>(mem.baseAddress + Globals.offsets.uworld);
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
    if (!util::IsValidVA(uWorldCheck) || uWorldCheck != cache.UWorld.load()) {
        UpdateCore(false);
        return;
    }

    uintptr_t uLevelCheck = mem.Read<uintptr_t>(uWorldCheck + Globals.offsets.persistentlevel);
    if (!util::IsValidVA(uLevelCheck) || uLevelCheck != cache.PersistentLevel.load()) {
        UpdateCore(false);
        return;
    }

    TArray<uintptr_t> actorArray = mem.Read<TArray<uintptr_t>>(cache.PersistentLevel + Globals.offsets.AActors);
    if (!actorArray.isValid() || actorArray.Num() == 0) return;

    std::vector<uintptr_t> currentActors(actorArray.Num());
    if (!mem.Read(actorArray.getAddress(), currentActors.data(), currentActors.size() * sizeof(uintptr_t))) return;

    
    std::unordered_set<uintptr_t> currentSet(currentActors.begin(), currentActors.end());
    std::unordered_set<uintptr_t> lastSet(cache.lastActorList.begin(), cache.lastActorList.end());

    std::vector<uintptr_t> removed, added;
    std::set_difference(lastSet.begin(), lastSet.end(), currentSet.begin(), currentSet.end(), std::back_inserter(removed));
    std::set_difference(currentSet.begin(), currentSet.end(), lastSet.begin(), lastSet.end(), std::back_inserter(added));

  
    for (uintptr_t actor : removed) {
        cache.cachedPlayerActors.erase(actor);
        cache.cachedItemActors.erase(actor);
    }

   
    auto processNewActor = [&](uintptr_t actor) -> std::pair<bool, bool> {
        if (!util::IsValidVA(actor) || actor == cache.LocalPawn.load()) return { false, false };


        int nameID = mem.Read<int>(actor + 0x18);
        if (nameID <= 0) return { false, false };

        std::string name = CacheFNAME(nameID);
        if (name.empty()) return { false, false };

        bool isPlayer = name.find("BP_Character") != std::string::npos ||
            name.find("SolarCharacter") != std::string::npos;
        bool isItem = false;

        if (Globals.itemsEnabled && !isPlayer) {
           
                cache.cachedItemActors.insert(actor);
                isItem = true;
            }

        if (isPlayer) cache.cachedPlayerActors.insert(actor);
        return { isPlayer, isItem };
        };

    for (uintptr_t actor : added) processNewActor(actor);
 
    {
        {
            std::unique_lock lock(cache.cacheMutex);
            cache.Actors.assign(cache.cachedPlayerActors.begin(), cache.cachedPlayerActors.end());
           
            cache.lastActorList = std::move(currentActors);
        }
    }

    if (debug && (!added.empty() || !removed.empty())) {
        std::cout << "Actor changes - Added: " << added.size() << ", Removed: " << removed.size()
            << ", Players: " << cache.cachedPlayerActors.size() << std::endl;
    }
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
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 3000) return;
    lastUpdate = now;

     
    static uintptr_t lastValidWorld = 0;
    uintptr_t currentWorld = cache.UWorld.load();
    if (currentWorld != lastValidWorld) {
        uintptr_t uWorldCheck = mem.Read<uintptr_t>(mem.baseAddress + Globals.offsets.uworld);
        if (!util::IsValidVA(uWorldCheck) || uWorldCheck != currentWorld) {
            UpdateCore(false);
            return;
        }
        lastValidWorld = currentWorld;
    }

	 
    std::vector<uintptr_t> actorsCopy;
    {
        std::unique_lock lock(cache.cacheMutex);
        actorsCopy = cache.Actors;
    }

    if (actorsCopy.size() <= 1)
    {
		UpdateActors(false);
		return;
    }

    auto scatter = mem.CreateScatterHandle();
    std::vector<uintptr_t> meshes(actorsCopy.size()), states(actorsCopy.size());
    std::vector<uint64_t> abilities(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(actorsCopy[i])) continue;
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.mesh, &meshes[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.Actor_State, &states[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.AbilitySystemComponent, &abilities[i], sizeof(uint64_t));
		//we can get SolarAbilitySystemComponent from ASolarCharacterBase->ACharacter->APawn->AActor
    }
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);

    std::vector<PlayerRender> renders;
    renders.reserve(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(states[i]) || !util::IsValidVA(abilities[i])) continue;
		//ASolarAbilitySystemComponent -> SpawnedAttributes[TArray of UattributeSet Pointers]
        auto SpawnedAttributes = mem.Read<TArray<uintptr_t>>(abilities[i] + 0x150);
        if (!SpawnedAttributes.isValid() || SpawnedAttributes.Num() == 0) continue;

        PlayerRender pr{};
        pr.Actor = actorsCopy[i];
        pr.mesh = meshes[i];
        pr.TeamId = mem.ReadChain(states[i], { Globals.offsets.ASolarTeamInfo, Globals.offsets.teamState });
        pr.AliveDeadorKnocked = mem.Read<ECharacterHealthState>(states[i] + Globals.offsets.CharacterHealthState);
        pr.Health = mem.Read<float>(mem.Read<uintptr_t>(SpawnedAttributes.getAddress()) + 0x54); 

		// SpawnedAttributes[0]->FGamePlayAttributeData[CurrentHealth{0x48}]->CurrentValue{0x54}]
		//if(debug)std::cout << "Health: " << pr.Health << std::endl;
        FString fName = mem.Read<FString>(states[i] + Globals.offsets.PlayerName);
        if (fName.IsValid() && fName.Count < 32) {
            std::vector<wchar_t> buf(fName.Count + 1, 0);
            if (mem.Read((uintptr_t)fName.Data, buf.data(), fName.Count * sizeof(wchar_t))) {
                std::wstring ws(buf.data());
                pr.Name = std::string(ws.begin(), ws.end());
            }
        }

        renders.push_back(std::move(pr));
    }
    {
        std::unique_lock attrLock(AttributeMutex);
        AttributeCache.clear();
        AttributeCache.reserve(renders.size());
        for (auto& pr : renders) {
            if (pr.Actor) AttributeCache.emplace(pr.Actor, pr);
        }
        }
    }