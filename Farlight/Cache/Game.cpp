#include <Cache/Game.h>
#include "ItemESP.h"  
#include <Utils/Utils.h>
#include <DMALibrary/Memory/Memory.h>
#include <Utils/Globals.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "Aimbot/Aimbot.h"

static std::unordered_map<int, std::string> fnameCache;
_GameCache cache;


auto SafeGetName = [](int id) -> std::string {
    return fnameCache.count(id) ? fnameCache[id] : (fnameCache[id] = GetNameFromFName(id));
    };

bool Get_Uworld() {
    uint64_t base = mem.GetBaseDaddy("SolarlandClient-Win64-Shipping.exe");
    uint64_t size = mem.GetBaseSize("SolarlandClient-Win64-Shipping.exe");

    auto findSig = [&](const char* pattern) { return mem.FindSignature(pattern, base, base + size); };
    auto calcOffset = [&](uint64_t addr, uint64_t base) { return (addr + 7 + mem.Read<int32_t>(addr + 3)) - base; };

    uint64_t uworldAddr = findSig("48 8B 1D ? ? ? ? 48 85 DB 74 ? 41 B0 ");
    uint64_t gnamesAddr = findSig("48 8D 05 ? ? ? ? EB ? 48 8D 0D ? ? ? ? E8");
    uint64_t gobjectsAddr = findSig("48 8B 05 ? ? ? ? 4A 8B ");

    if (!uworldAddr || !gnamesAddr) return false;

    Globals.offsets.uworld = calcOffset(uworldAddr, base);
    Globals.offsets.GNAMES = calcOffset(gnamesAddr, base);
    Globals.offsets.GOBJECTS = calcOffset(gobjectsAddr, base);

    return util::IsValidVA(Globals.offsets.uworld) && util::IsValidVA(Globals.offsets.GNAMES);
}

//updatecore spam 

void Game::Loop() {
    UpdateCore(true);
    UpdateActors(true);
    std::thread(Game::UpdateActorsLoop).detach();
    ItemESP::Start(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto processFrame = [&]() {
        UpdateLocal(false);
        UpdateAttributes(false);
        if (!util::IsValidVA(cache.LocalPawn.load()) || !util::IsValidVA(cache.PersistentLevel.load())) return false;

        std::vector<uintptr_t> actorsCopy;
        std::vector<PlayerRender> playerSnapshot;
        {
            std::unique_lock cacheLock(cache.cacheMutex);
            actorsCopy = cache.Actors;
        }
        {
            std::unique_lock playerLock(Globals.playerMutex);
            playerSnapshot = Globals.renderPlayers;
        }
        if (actorsCopy.size() <= 1) return false;

        std::vector<uintptr_t> meshes(actorsCopy.size());
        std::vector<TArray<FTransform>> boneArrays(actorsCopy.size());
        std::vector<FTransform> compToWorlds(actorsCopy.size());

        auto bone_scatter = mem.CreateScatterHandle();
        if (!bone_scatter) return false;

        for (size_t i = 0; i < actorsCopy.size(); i++) {
            if (actorsCopy[i] == cache.LocalPawn || !util::IsValidVA(actorsCopy[i])) continue;
            uintptr_t mesh = mem.Read<uintptr_t>(actorsCopy[i] + Globals.offsets.mesh);
            if (!util::IsValidVA(mesh)) continue;
            meshes[i] = mesh;
            mem.AddScatterReadRequest(bone_scatter, mesh + Globals.offsets.boneArray, &boneArrays[i], sizeof(TArray<FTransform>));
            mem.AddScatterReadRequest(bone_scatter, mesh + Globals.offsets.componentToWorld, &compToWorlds[i], sizeof(FTransform));
        }
        mem.ExecuteReadScatter(bone_scatter);
        mem.CloseScatterHandle(bone_scatter);

        for (size_t i = 0; i < actorsCopy.size() && i < playerSnapshot.size(); i++) {
            if (!util::IsValidVA(meshes[i]) || !boneArrays[i].isValid() || boneArrays[i].Num() <= 7 || boneArrays[i].Num() > 200) continue;
            std::vector<FTransform> bones(boneArrays[i].Num());
            if (!mem.Read(boneArrays[i].getAddress(), bones.data(), bones.size() * sizeof(FTransform))) continue;

            auto calcDistance = [&](const Vector3& pos) {
                float dx = pos.x - cache.LocalCamera.Location.x;
                float dy = pos.y - cache.LocalCamera.Location.y;
                float dz = pos.z - cache.LocalCamera.Location.z;
                return sqrt(dx * dx + dy * dy + dz * dz) / 100.0f;
                };

            Vector2 head = doMatrix(bones[7], compToWorlds[i], cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
            Vector2 root = doMatrix(bones[0], compToWorlds[i], cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);

            playerSnapshot[i].headW2S = head;
            playerSnapshot[i].bottomW2S = root;
            playerSnapshot[i].distance = calcDistance(compToWorlds[i].translation);
        }

        {
            std::unique_lock lock(Globals.playerMutex);
            Globals.renderPlayers = std::move(playerSnapshot);
        }

        Vector2 best{ 0.f, 0.f };
        float bestDist = FLT_MAX;
        uintptr_t bestMesh = 0;

        for (const auto& pr : Globals.renderPlayers) {
            if (pr.Actor == cache.LocalPawn) continue;
            if (!util::IsValidVA(pr.mesh)) continue;
            if (Globals.settings.IgnoreKnocked && pr.AliveDeadorKnocked != ECharacterHealthState::ECHS_Alive) continue;
            if (pr.distance > Globals.settings.AimbotMaxDistance) continue;

            Vector2 targetPoint;
            if (Globals.TargetPriority == ETargetPriority::Head ) {
                targetPoint = pr.headW2S;
            }
            else { 
                const float t = 0.15f;
                targetPoint.x = pr.bottomW2S.x * (1.0f - t) + pr.headW2S.x * t;
                targetPoint.y = pr.bottomW2S.y * (1.0f - t) + pr.headW2S.y * t;
            }

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
    while (true) 
    { if (Globals.refreshcheat)
    { Globals.refreshcheat = false; 
    mem.FullRefresh(); 
    UpdateCore(true); continue; } 
    auto start = std::chrono::steady_clock::now(); processFrame(); 
    auto end = std::chrono::steady_clock::now(); 
    static double avgMs = 16.0; double ms = std::chrono::duration<double, std::milli>(end - start).count();
    avgMs = avgMs * 0.9 + ms * 0.1; Globals.readFPS = static_cast<int>(1000.0 / avgMs + 0.5); 
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); }
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
                << " GameInstance: 0x" << newGameInstance << " LocalPawn: 0x" << newPawn << std::dec << std::endl;
        }
        break;
    }
    UpdateActors(true);
}

void Game::UpdateActorsLoop() {
    while (true) {
        UpdateActors(false); // Reduced debug spam
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void Game::UpdateActors(bool debug) {
    // Validate core pointers first - PREVENTS FLICKERING
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

    TArray<uintptr_t> actorArray = mem.Read<TArray<uintptr_t>>(cache.PersistentLevel + 0x98);
    if (!actorArray.isValid() || actorArray.Num() == 0) return;

    std::vector<uintptr_t> currentActors(actorArray.Num());
    if (!mem.Read(actorArray.getAddress(), currentActors.data(), currentActors.size() * sizeof(uintptr_t))) return;

    // Differential updates only - MAJOR FLICKERING FIX
    std::unordered_set<uintptr_t> currentSet(currentActors.begin(), currentActors.end());
    std::unordered_set<uintptr_t> lastSet(cache.lastActorList.begin(), cache.lastActorList.end());

    std::vector<uintptr_t> removed, added;
    std::set_difference(lastSet.begin(), lastSet.end(), currentSet.begin(), currentSet.end(), std::back_inserter(removed));
    std::set_difference(currentSet.begin(), currentSet.end(), lastSet.begin(), lastSet.end(), std::back_inserter(added));

    // Remove deleted actors
    for (uintptr_t actor : removed) {
        cache.cachedPlayerActors.erase(actor);
        cache.cachedItemActors.erase(actor);
    }

    // Process only new actors
    auto processNewActor = [&](uintptr_t actor) -> std::pair<bool, bool> {
        if (!util::IsValidVA(actor) || actor == cache.LocalPawn.load()) return { false, false };

        int nameID = mem.Read<int>(actor + 0x18);
        if (nameID <= 0) return { false, false };

        std::string name = SafeGetName(nameID);
        if (name.empty()) return { false, false };

        bool isPlayer = name.find("BP_Character") != std::string::npos ||
            name.find("SolarCharacter") != std::string::npos;
        bool isItem = false;

        if (Globals.itemsEnabled && !isPlayer) {
            FSolarItemData itemData = mem.Read<FSolarItemData>(actor + 0x360);
            if (itemData.ItemType != EItemType::NONE) {
                ItemEntry e{};
                e.actor = actor;
                e.ItemID = itemData.ItemID;
                e.ThisID = itemData.ThisID;
                e.ItemType = itemData.ItemType;
                e.Count = itemData.Count;
                e.WeaponType = mem.Read<EWeaponType>(actor + 0x56c);
                e.Name = "";
                e.Icon = "";
                e.Info = "";
                cache.cachedItemActors[actor] = std::move(e);
                isItem = true;
            }
        }

        if (isPlayer) cache.cachedPlayerActors.insert(actor);
        return { isPlayer, isItem };
        };

    for (uintptr_t actor : added) processNewActor(actor);
 
    {
        std::unique_lock lock(cache.cacheMutex);
        cache.Actors.assign(cache.cachedPlayerActors.begin(), cache.cachedPlayerActors.end());
        cache.Items.clear();
        cache.Items.reserve(cache.cachedItemActors.size());
        for (const auto& [actor, item] : cache.cachedItemActors) {
            cache.Items.push_back(item);
        }
        cache.lastActorList = std::move(currentActors);
    }

    if (debug && (!added.empty() || !removed.empty())) {
        std::cout << "Actor changes - Added: " << added.size() << ", Removed: " << removed.size()
            << ", Players: " << cache.cachedPlayerActors.size() << std::endl;
    }
}

void Game::UpdateLocal(bool debug) {
    if (!cache.LocalPawn.load()) return;

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
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 200) return;
    lastUpdate = now;

    // Skip validation if recently updated - REDUCES FLICKERING
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

    if (actorsCopy.size() < 2) return;

    auto scatter = mem.CreateScatterHandle();
    std::vector<uintptr_t> meshes(actorsCopy.size()), states(actorsCopy.size());
    std::vector<uint64_t> abilities(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(actorsCopy[i])) continue;
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + Globals.offsets.mesh, &meshes[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + 0x2f8, &states[i], sizeof(uintptr_t));
        mem.AddScatterReadRequest(scatter, actorsCopy[i] + 0x650, &abilities[i], sizeof(uint64_t));
    }
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);

    std::vector<PlayerRender> renders;
    renders.reserve(actorsCopy.size());

    for (size_t i = 0; i < actorsCopy.size(); ++i) {
        if (!util::IsValidVA(states[i]) || !util::IsValidVA(abilities[i])) continue;

        PlayerRender pr{};
        pr.Actor = actorsCopy[i];
        pr.mesh = meshes[i];
        pr.TeamId = mem.ReadChain(states[i], { Globals.offsets.ASolarTeamInfo, Globals.offsets.teamState });
        pr.AliveDeadorKnocked = mem.Read<ECharacterHealthState>(states[i] + Globals.offsets.CharacterHealthState);

        float hp = mem.ReadChain(abilities[i], { 0x150, 0x0, 0x54 });
        pr.Health = (hp > 0 && hp <= 150) ? hp : 150.0f;

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

    // Single atomic update - ELIMINATES FLICKERING
    {
        std::unique_lock lock(Globals.playerMutex);
        Globals.renderPlayers = std::move(renders);
    }
}