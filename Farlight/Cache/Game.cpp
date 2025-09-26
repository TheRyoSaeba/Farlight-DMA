#include <Cache/Game.h>
#include <Utils/Utils.h>
#include <DMALibrary/Memory/Memory.h>
#include <Utils/Globals.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "Aimbot/Aimbot.h"


struct _GameCache {
	uintptr_t UWorld = 0;
	uintptr_t GameInstance = 0;
	uintptr_t GameState = 0;
	uintptr_t PersistentLevel = 0;
	uintptr_t LocalPawn = 0;
	uintptr_t CameraManager = 0;
	uintptr_t Array_Address;
	Camera LocalCamera;
	std::vector<uintptr_t> Actors;
	std::vector<ItemEntry> Items;
	std::mutex cacheMutex;
	std::chrono::steady_clock::time_point LastCoreUpdate{};
	std::chrono::steady_clock::time_point LastPlayerUpdate{};
}
cache;

static std::unordered_map<int, std::string> fnameCache;

std::string SafeGetName(int id) {
	auto it = fnameCache.find(id);
	if (it != fnameCache.end()) return it->second;
	std::string n = GetNameFromFName(id);
	fnameCache[id] = n;
	return n;
}



bool Get_Uworld() {
	uint64_t base = mem.GetBaseDaddy("SolarlandClient-Win64-Shipping.exe");
	uint64_t size = mem.GetBaseSize("SolarlandClient-Win64-Shipping.exe");

	uint64_t uworldAddr = mem.FindSignature("48 8B 1D ? ? ? ? 48 85 DB 74 ? 41 B0 ", base, base + size);

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
	std::cout << "GOBJECTS: 0x" << std::hex << Globals.offsets.GOBJECTS << std::dec << std::endl;

	return (util::IsValidVA(Globals.offsets.uworld) && util::IsValidVA(Globals.offsets.GNAMES));

}


void Game::Loop() {
	using clock = std::chrono::steady_clock;

	UpdateCore(true);

	UpdateActors(true);

	std::thread(Game::UpdateActorsLoop).detach();
	 

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	while (true) {

		if (Globals.refreshcheat)
		{
			Globals.refreshcheat = false;
			cache.Actors.clear();
			cache.Items.clear();
			mem.FullRefresh();
			Game::UpdateCore(true);
		 }

		auto tStart = clock::now();
		UpdateLocal(false);
		UpdateAttributes(false);
 
		UpdateItems(true, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);


		size_t n;
		{
			std::unique_lock lock(cache.cacheMutex);
			n = cache.Actors.size();
		}
		if (n > 0) {
			std::vector<uintptr_t> meshes(n);
			std::vector<TArray<FTransform>> boneArrays(n);
			std::vector<FTransform> compToWorlds(n);

			auto bone_scatter = mem.CreateScatterHandle();
			if (bone_scatter) {
				for (size_t i = 0; i < n; i++) {
					if (!util::IsValidVA(cache.Actors[i])) continue;
					if (cache.Actors[i] == cache.LocalPawn) continue;

					uintptr_t mesh = mem.Read<uintptr_t>(cache.Actors[i] + Globals.offsets.mesh);
					if (!util::IsValidVA(mesh)) continue;

					meshes[i] = mesh;
					mem.AddScatterReadRequest(bone_scatter, mesh + Globals.offsets.boneArray, &boneArrays[i], sizeof(TArray<FTransform>));
					mem.AddScatterReadRequest(bone_scatter, mesh + Globals.offsets.componentToWorld, &compToWorlds[i], sizeof(FTransform));
				}
				mem.ExecuteReadScatter(bone_scatter);
				mem.CloseScatterHandle(bone_scatter);
			}

			for (size_t i = 0; i < n; i++) {
				if (!util::IsValidVA(meshes[i])) continue;
				auto& arr = boneArrays[i];
				if (!arr.isValid() || arr.Num() <= 7 || arr.Num() > 200) continue;

				std::vector<FTransform> bones(arr.Num());
				if (!mem.Read(arr.getAddress(), bones.data(), arr.Num() * sizeof(FTransform))) continue;

				Vector2 head = doMatrix(bones[7], compToWorlds[i], cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
				Vector2 root = doMatrix(bones[0], compToWorlds[i], cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);

				Vector3 playerWorldPos = compToWorlds[i].translation;
				FVector cameraPos = cache.LocalCamera.Location;
				float deltaX = playerWorldPos.x - cameraPos.x;
				float deltaY = playerWorldPos.y - cameraPos.y;
				float deltaZ = playerWorldPos.z - cameraPos.z;
				float distanceCM = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
				float distanceMeters = distanceCM / 100.0f;

				if (i < Globals.renderPlayers.size()) {
					Globals.renderPlayers[i].headW2S = head;
					Globals.renderPlayers[i].bottomW2S = root;
					Globals.renderPlayers[i].distance = distanceMeters;
				}
			}
		}

		Vector2 best{ 0, 0 };
		float bestDist = FLT_MAX;
		uintptr_t bestMesh = 0;

		for (const auto& pr : Globals.renderPlayers) {
			if (pr.Actor == cache.LocalPawn) continue;
			if (pr.AliveDeadorKnocked == ECharacterHealthState::ECHS_Dead) continue;
			float d = GetCrossDistance(pr.headW2S.x, pr.headW2S.y,
				Globals.screenWidth / 2, Globals.screenHeight / 2);
			if (d < Globals.settings.fov && d < bestDist) {
				bestDist = d;
				best = pr.headW2S;
				bestMesh = pr.mesh;
			}
		}
		if (util::IsValidVA(bestMesh))
			Aimbot::aimbot(best);

		auto tEnd = clock::now();
		double ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tEnd - tStart).count();
		static double avgMs = 16.0;
		avgMs = avgMs * 0.9 + ms * 0.1;
		Globals.readFPS = static_cast<int>(1000.0 / avgMs + 0.5);

		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}
}

void Game::UpdateCore(bool debug) {

	Globals.renderPlayers.clear();
	Globals.renderItems.clear();

	auto core_handle = mem.CreateScatterHandle();

	if (debug) std::cout << "Updating core cache..." << std::endl;



	bool result = false;
	while (!result) {
		uintptr_t newUWorld = mem.Read<uintptr_t>(mem.baseAddress + Globals.offsets.uworld);

		if (!util::IsValidVA(newUWorld)) continue;

		uintptr_t newGameInstance, persistentlevel;
		mem.AddScatterReadRequest(core_handle, newUWorld + Globals.offsets.gameInstance, &newGameInstance, sizeof(uintptr_t));
		mem.AddScatterReadRequest(core_handle, newUWorld + Globals.offsets.persistentlevel, &persistentlevel, sizeof(uintptr_t));
		mem.ExecuteReadScatter(core_handle);
		mem.CloseScatterHandle(core_handle);

		if (!util::IsValidVA(newGameInstance) || !util::IsValidVA(persistentlevel)) {
			continue;
		}
		uintptr_t newLocalPlayers = mem.Read<uintptr_t>(mem.Read<uintptr_t>(newGameInstance + Globals.offsets.playersArray));
		uintptr_t newPlayerController = mem.Read<uintptr_t>(newLocalPlayers + Globals.offsets.playerController);
		uintptr_t newPawn = mem.Read<uintptr_t>(newPlayerController + Globals.offsets.localPawn);
		uintptr_t newCameraManager = mem.Read<uintptr_t>(newPlayerController + Globals.offsets.playerCameraManager);

		if (!util::IsValidVA(newLocalPlayers) || !util::IsValidVA(newPlayerController) ||
			!util::IsValidVA(newPawn) || !util::IsValidVA(newCameraManager)) {
			continue;
		}

		cache.UWorld = newUWorld;
		cache.GameInstance = newGameInstance;
		cache.LocalPawn = newPawn;
		cache.CameraManager = newCameraManager;
		cache.PersistentLevel = persistentlevel;

		result = true;

		if (debug) {
			std::cout << "Core pointers updated:" << std::endl;
			std::cout << "UWorld: 0x" << std::hex << cache.UWorld
				<< " \nGameInstance: 0x" << cache.GameInstance
				<< " \nPersistentLevel: 0x" << cache.PersistentLevel
				<< " \nLocalPawn: 0x" << cache.LocalPawn
				<< "\n CameraMgr: 0x" << cache.CameraManager
				<< std::dec << std::endl;
		}
	}
	Game::UpdateActors(true);
}

void Game::UpdateActorsLoop() {
	while (true) {
		Game::UpdateActors(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}
}

void Game::UpdateItemsLoop()
{
	while (true) {
		Game::UpdateItems(true, cache.LocalCamera, Globals.screenWidth, Globals.screenHeight);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}


void Game::UpdateActors(bool debug) {

	static std::vector<uintptr_t> lastActorList;

	auto check_scatter = mem.CreateScatterHandle();

	uintptr_t uWorldCheck, uLevelCheck;
	mem.AddScatterReadRequest(check_scatter, mem.baseAddress + Globals.offsets.uworld, &uWorldCheck, sizeof(uintptr_t));
	mem.ExecuteReadScatter(check_scatter);
	mem.CloseScatterHandle(check_scatter);

	if (!util::IsValidVA(uWorldCheck) || uWorldCheck != cache.UWorld) {
		Game::UpdateCore(true);
		return;
	}
	else {

		uLevelCheck = mem.Read<uintptr_t>(uWorldCheck + Globals.offsets.persistentlevel);
		if (!util::IsValidVA(uLevelCheck) || uLevelCheck != cache.PersistentLevel) {
			Game::UpdateCore(true);
			return;

		}
		//this means both uworld and persistentlevel are 
	}

	TArray<uintptr_t> actorArray = mem.Read<TArray<uintptr_t>>(cache.PersistentLevel + 0x98);
	if (!actorArray.isValid() || actorArray.Num() == 0) return;

	std::vector<uintptr_t> currentActors;
	currentActors.resize(actorArray.Num());
	if (!mem.Read(actorArray.getAddress(), currentActors.data(), currentActors.size() * sizeof(uintptr_t))) {
		return;
	}

	std::unordered_set<uintptr_t> currentSet(currentActors.begin(), currentActors.end());

	std::vector<uintptr_t> newActors;
	std::vector<ItemEntry> newItems;

	for (uintptr_t actor : currentActors) {
		if (!util::IsValidVA(actor)) continue;

		int nameID = mem.Read<int>(actor + 0x18);
		if (nameID <= 0) continue;

		std::string name = SafeGetName(nameID);
		if (name.empty()) continue;

		if (actor == cache.LocalPawn)
			continue;


		if (name.find("BP_Character") != std::string::npos ||
			name.find("SolarCharacter") != std::string::npos ||
			name.find("BP_Character_BattleRoyaleMap01_C") != std::string::npos ||
			name.find("BP_Character_TrainingMode_C") != std::string::npos) {
			newActors.push_back(actor);
		}

		if (Globals.itemsEnabled) {
			FSolarItemData itemData = mem.Read<FSolarItemData>(actor + 0x360);
			if (itemData.ItemType != EItemType::NONE) {
				ItemEntry e{};
				e.actor = actor;
				e.ItemID = itemData.ItemID;
				e.ThisID = itemData.ThisID;
				e.ItemType = itemData.ItemType;
				e.Count = itemData.Count;

				auto readStr = [&](const FString& f) -> std::string {
					if (!f.IsValid() || f.Count <= 0 || f.Count > 128 || !util::IsValidVA((uintptr_t)f.Data))
						return {};
					std::vector<wchar_t> buf(f.Count + 1);
					return mem.Read((uintptr_t)f.Data, buf.data(), f.Count * sizeof(wchar_t))
						? std::string(buf.begin(), buf.end())
						: "";
					};

				e.Name = readStr(itemData.Name);
				e.Icon = readStr(itemData.Icon);
				e.Info = readStr(itemData.Info);
				e.WeaponType = mem.Read<EWeaponType>(actor + 0x56c);
				newItems.push_back(std::move(e));
			}
		}
	}

	{
		std::unique_lock lock(cache.cacheMutex);
		cache.Actors = std::move(newActors);
		cache.Items = std::move(newItems);
		lastActorList = std::move(currentActors);
	}
	if (debug) {
		std::cout << "Players: " << cache.Actors.size() << ", Items: " << cache.Items.size() << std::endl;

	}
}

void Game::UpdateItems(bool debug, const Camera& cam, int screenW, int screenH)
{

	if (!Globals.itemsEnabled)return;
	//make sure camera has been read

	if (util::IsValidVA(cache.CameraManager) == 0) return;

	static std::unordered_set<uintptr_t> oldItemSet;
	static std::unordered_map<uintptr_t, ItemRenderer> cachedItems; 

	std::unordered_set<uintptr_t> newItemSet;
	for (const auto& item : cache.Items) {
		if (util::IsValidVA(item.actor) &&
			(item.ItemType == EItemType::WEAPON || item.ItemType == EItemType::SHIELD ||
				item.ItemType == EItemType::HELMET || item.ItemType == EItemType::BACKPACK ||
				item.ItemType == EItemType::ARMOR || item.ItemType == EItemType::WEAPON_PARTS)) {
			newItemSet.insert(item.actor);
		}
	}


	std::vector<uintptr_t> removed, added;
	std::set_difference(oldItemSet.begin(), oldItemSet.end(),
		newItemSet.begin(), newItemSet.end(),
		std::back_inserter(removed));
	std::set_difference(newItemSet.begin(), newItemSet.end(),
		oldItemSet.begin(), oldItemSet.end(),
		std::back_inserter(added));

	for (uintptr_t actor : removed) {
		cachedItems.erase(actor);
		if (debug) std::cout << "Removed item: 0x" << std::hex << actor << std::dec << "\n";
	}

	if (!added.empty()) {
		auto scatter = mem.CreateScatterHandle();
		std::vector<uintptr_t> rootComponents(added.size(), 0);

		for (size_t i = 0; i < added.size(); ++i) {
			mem.AddScatterReadRequest(scatter, added[i] + 0x1b0, &rootComponents[i], sizeof(uintptr_t));
		}
		mem.ExecuteReadScatter(scatter);

		
		std::vector<FTransform> compToWorlds(added.size());
		for (size_t i = 0; i < added.size(); ++i) {
			if (util::IsValidVA(rootComponents[i])) {
				mem.AddScatterReadRequest(scatter, rootComponents[i] + Globals.offsets.componentToWorld,
					&compToWorlds[i], sizeof(FTransform));
			}
		}
		mem.ExecuteReadScatter(scatter);
		mem.CloseScatterHandle(scatter);

		for (size_t i = 0; i < added.size(); ++i) {
			uintptr_t actor = added[i];

			
			const ItemEntry* itemEntry = nullptr;
			for (const auto& item : cache.Items) {
				if (item.actor == actor) {
					itemEntry = &item;
					break;
				}
			}
			if (!itemEntry) continue;

			ItemRenderer ir{};
			ir.Actor = actor;
			ir.Item = itemEntry->ItemType;
			ir.Weapon = itemEntry->WeaponType;
			ir.Name = itemEntry->Name;
			ir.Location = compToWorlds[i].translation; 

			cachedItems[actor] = ir;
			if (debug) std::cout << "Added item: " << ir.Name << " at 0x" << std::hex << actor << std::dec << "\n";
		}
	}

	std::vector<ItemRenderer> renderItems;
	renderItems.reserve(cachedItems.size());

	for (auto& [actor, item] : cachedItems) {
 
		FTransform dummyBone{};
		dummyBone.translation = Vector3(0, 0, 0);
		dummyBone.rot = { 0.0f, 0.0f, 0.0f, 1.0f };
		dummyBone.scale = { 1.0f, 1.0f, 1.0f };

		FTransform worldTransform{};
		worldTransform.translation = item.Location;
		worldTransform.rot = { 0.0f, 0.0f, 0.0f, 1.0f };
		worldTransform.scale = { 1.0f, 1.0f, 1.0f };

		Vector2 screenPos = doMatrix(dummyBone, worldTransform, cam, screenW, screenH);

		if (screenPos.x >= 0.0f && screenPos.x <= static_cast<float>(screenW) &&
			screenPos.y >= 0.0f && screenPos.y <= static_cast<float>(screenH)) {

			 
			float dx = item.Location.x - cam.Location.x;
			float dy = item.Location.y - cam.Location.y;
			float dz = item.Location.z - cam.Location.z;
			float distance = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f - 3.0f;

			item.W2S = screenPos;
			item.distance = distance;
			renderItems.push_back(item);
		}
	}

	oldItemSet = newItemSet; // Update for next frame

	{
		std::unique_lock lock(Globals.itemMutex);
		Globals.renderItems = std::move(renderItems);
	}

	if (debug && !added.empty()) {
		std::cout << "Items processed - Added: " << added.size() << ", Total cached: " << cachedItems.size() << "\n";
	}
}

void Game::UpdateLocal(bool debug)
{
	if (cache.LocalPawn == 0) return;

	auto Scatter_local = mem.CreateScatterHandle();
	mem.AddScatterReadRequest(Scatter_local, cache.CameraManager + Globals.offsets.playerCameraCache + Globals.offsets.playerCameraPOV, &cache.LocalCamera, sizeof(Camera));
	mem.ExecuteReadScatter(Scatter_local);
	mem.CloseScatterHandle(Scatter_local);

	if (debug) {
		std::cout << "\n Local camera updated:" << std::endl;
		std::cout << " Location: (" << cache.LocalCamera.Location.x << ", " << cache.LocalCamera.Location.y << ", " << cache.LocalCamera.Location.z << ")" << std::endl;
		std::cout << " Rotation: (" << cache.LocalCamera.Rotation.Pitch << ", " << cache.LocalCamera.Rotation.Yaw << ", " << cache.LocalCamera.Rotation.Roll << ")" << std::endl;
		std::cout << " FOV: " << cache.LocalCamera.FOV << std::endl;
	}
}



void Game::UpdateAttributes(bool debug)
{

	using clock = std::chrono::steady_clock;
	static auto lastUpdate = clock::now();


	auto now = clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 1000) {
		return;
	}
	lastUpdate = now;

	auto attribute_scatter = mem.CreateScatterHandle();
	if (!attribute_scatter) return;


	auto check_scatter2 = mem.CreateScatterHandle();

	uintptr_t uWorldCheck;
	mem.AddScatterReadRequest(check_scatter2, mem.baseAddress + Globals.offsets.uworld, &uWorldCheck, sizeof(uintptr_t));
	mem.ExecuteReadScatter(check_scatter2);
	mem.CloseScatterHandle(check_scatter2);

	if (!util::IsValidVA(uWorldCheck) || uWorldCheck != cache.UWorld) {
		Game::UpdateCore(true);
		return;
	}

	size_t n = cache.Actors.size();
	std::vector<uintptr_t> meshes(n);
	std::vector<uintptr_t> states(n);
	std::vector<uint64_t> abilities(n);

	if (n < 2)
	{
		Game::UpdateActors(true);
		return;          // abort only when we really have too few actors
	}

	for (size_t i = 0; i < n; i++) {
		uintptr_t pawn = cache.Actors[i];
		if (!util::IsValidVA(pawn)) continue;
		mem.AddScatterReadRequest(attribute_scatter, pawn + Globals.offsets.mesh, &meshes[i], sizeof(uintptr_t));
		mem.AddScatterReadRequest(attribute_scatter, pawn + 0x2f8, &states[i], sizeof(uintptr_t)); // 
		mem.AddScatterReadRequest(attribute_scatter, pawn + 0x650, &abilities[i], sizeof(uint64_t)); // AbilityComponent
	}
	mem.ExecuteReadScatter(attribute_scatter);
	mem.CloseScatterHandle(attribute_scatter);



	std::vector<int> teams(n, -1);
	std::vector<float> healths(n, 150.0f);
	std::vector<std::string> names(n);
	std::vector <ECharacterHealthState> AliveDeadorKnocked(n, ECharacterHealthState::ECHS_Alive);

	for (size_t i = 0; i < n; i++) {
		if (!util::IsValidVA(states[i])) continue;
		if (!util::IsValidVA(abilities[i])) continue;
		FString fName = mem.Read<FString>(states[i] + Globals.offsets.PlayerName);
		if (fName.IsValid() && fName.Count < 32) {
			std::vector<wchar_t> buf(fName.Count + 1, 0);
			if (mem.Read((uintptr_t)fName.Data, buf.data(), fName.Count * sizeof(wchar_t))) {
				std::wstring ws(buf.data());
				names[i] = std::string(ws.begin(), ws.end());
			}
		}

		ECharacterHealthState HealthState = mem.Read<ECharacterHealthState>(states[i] + Globals.offsets.CharacterHealthState);
		AliveDeadorKnocked[i] = HealthState;
		teams[i] = mem.ReadChain(states[i], { Globals.offsets.ASolarTeamInfo,Globals.offsets.teamState });
		//spawned attributes is a tarray , so health might not be at 0x0 well have to look through them all
		float hp = mem.ReadChain(abilities[i], { 0x150,0x0,0x54 });
		if (hp < 0 || hp > 150) hp = 150.0f;
		//
		healths[i] = hp > 0 ? hp : healths[i];
	}

	std::vector<PlayerRender> renders;
	renders.reserve(n);

	for (size_t i = 0; i < n; i++) {
		if (!util::IsValidVA(cache.Actors[i])) continue;
		PlayerRender pr{};
		pr.Actor = cache.Actors[i];
		pr.mesh = meshes[i];
		pr.Name = names[i];
		pr.TeamId = teams[i];
		pr.Health = healths[i];
		pr.AliveDeadorKnocked = AliveDeadorKnocked[i];
		renders.push_back(std::move(pr));
	}

	{
		std::unique_lock lock(Globals.playerMutex);
		Globals.renderPlayers = std::move(renders);
	}

	if (debug) {
		std::cout << "\n Player attribute update complete. Found " << Globals.renderPlayers.size() << " players:\n";
		for (const auto& pr : Globals.renderPlayers) {
			std::cout << " 0x" << std::hex << pr.Actor << std::dec
				<< " | Team: " << pr.TeamId
				<< " | Health: " << pr.Health
				<< " | State: " << (pr.AliveDeadorKnocked == ECharacterHealthState::ECHS_Alive ? "Alive" : pr.AliveDeadorKnocked == ECharacterHealthState::ECHS_Knocked ? "Knocked" : "Dead")

				<< " | Name: " << pr.Name << std::endl;
		}

	}
}

