#include "ItemESP.h"
#include <Utils/Globals.h>
#include <DMALibrary/Memory/Memory.h>
#include <algorithm>
#include <iostream>

std::unordered_map<uintptr_t, ItemRenderer> ItemESP::cachedItems;
std::thread ItemESP::refreshThread;
bool ItemESP::running = false;
bool ItemESP::debug = false;


void ItemESP::Start(bool dbg)
{
	if (running) return;
	debug = dbg;
	running = true;
	refreshThread = std::thread(RefreshLoop);
	if (debug) std::cout << "ItemESP refresh thread started\n";
}

void ItemESP::Stop()
{
	running = false;
	if (refreshThread.joinable()) refreshThread.join();
}

void ItemESP::RefreshLoop() {
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	while (running) {
		RefreshList();
		std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_MS));
	}
}

void ItemESP::RefreshList() {
	extern struct _GameCache cache;

	if (!Globals.itemsEnabled) {
		std::unique_lock lk(Globals.itemMutex);
		cachedItems.clear();
		return;
	}

	// Get ALL current item actors from game cache
	std::unordered_set<uintptr_t> currentActors;
	{
		std::unique_lock lk(cache.cacheMutex);
		currentActors = cache.cachedItemActors;
	}

	if (currentActors.empty()) {
		std::unique_lock lk(Globals.itemMutex);
		cachedItems.clear();
		return;
	}

	std::vector<uintptr_t> actorList(currentActors.begin(), currentActors.end());

	// Separate new actors from existing cached actors
	std::vector<uintptr_t> newActors;
	std::vector<uintptr_t> existingActors;
	{
		std::unique_lock lk(Globals.itemMutex);
		for (uintptr_t actor : actorList) {
			if (cachedItems.find(actor) == cachedItems.end()) {
				newActors.push_back(actor);
			} else {
				existingActors.push_back(actor);
			}
		}
	}

	std::unordered_map<uintptr_t, ItemRenderer> newCache;

	// For existing items: only update transforms (fast path)
	if (!existingActors.empty()) {
		auto scatter = mem.CreateScatterHandle();
		std::vector<uintptr_t> rootComps(existingActors.size());

		for (size_t i = 0; i < existingActors.size(); ++i) {
			mem.AddScatterReadRequest(scatter, existingActors[i] + Globals.offsets.RootComponent, &rootComps[i], sizeof(uintptr_t));
		}
		mem.ExecuteReadScatter(scatter);

		std::vector<FTransform> transforms(existingActors.size());
		for (size_t i = 0; i < existingActors.size(); ++i) {
			if (util::IsValidVA(rootComps[i])) {
				mem.AddScatterReadRequest(scatter, rootComps[i] + Globals.offsets.componentToWorld,
					&transforms[i], sizeof(FTransform));
			}
		}
		mem.ExecuteReadScatter(scatter);
		mem.CloseScatterHandle(scatter);

		// Update cached items with new positions
		std::unique_lock lk(Globals.itemMutex);
		for (size_t i = 0; i < existingActors.size(); ++i) {
			auto it = cachedItems.find(existingActors[i]);
			if (it != cachedItems.end()) {
				it->second.Location = transforms[i].translation;
				newCache[existingActors[i]] = it->second;
			}
		}
	}

	// For new items: do full read (slow path, but only for new items)
	if (newActors.empty()) {
		std::unique_lock lk(Globals.itemMutex);
		cachedItems = newCache;
		if (debug)
			std::cout << "ItemESP refreshed " << cachedItems.size() << " items (0 new)\n";
		return;
	}

	auto scatter = mem.CreateScatterHandle();
	std::vector<FSolarItemData> itemData(newActors.size());
	std::vector<EWeaponType> weaponTypes(newActors.size());
	std::vector<uintptr_t> rootComps(newActors.size());

	for (size_t i = 0; i < newActors.size(); ++i) {
		mem.AddScatterReadRequest(scatter, newActors[i] + Globals.offsets.FSolarItemData, &itemData[i], sizeof(FSolarItemData));
		mem.AddScatterReadRequest(scatter, newActors[i] + Globals.offsets.WeaponTypes, &weaponTypes[i], sizeof(EWeaponType));
		mem.AddScatterReadRequest(scatter, newActors[i] + Globals.offsets.RootComponent, &rootComps[i], sizeof(uintptr_t));
	}
	mem.ExecuteReadScatter(scatter);

	std::vector<FTransform> transforms(newActors.size());
	for (size_t i = 0; i < newActors.size(); ++i) {
		if (util::IsValidVA(rootComps[i])) {
			mem.AddScatterReadRequest(scatter, rootComps[i] + Globals.offsets.componentToWorld,
				&transforms[i], sizeof(FTransform));
		}
	}
	mem.ExecuteReadScatter(scatter);
	mem.CloseScatterHandle(scatter);

	// Collect valid NEW items to cache
	std::vector<size_t> validIndices;
	for (size_t i = 0; i < newActors.size(); ++i) {
		if (itemData[i].ItemType == EItemType::NONE) continue;
		validIndices.push_back(i);
	}

	// Batch read names for valid items
	if (!validIndices.empty()) {
		auto nameScatter = mem.CreateScatterHandle();
		std::vector<std::vector<wchar_t>> stringBuffers(validIndices.size());

		for (size_t j = 0; j < validIndices.size(); ++j) {
			size_t i = validIndices[j];
			const FString& fName = itemData[i].Name;

			if (fName.IsValid() && fName.Count > 0 && fName.Count < 128 && util::IsValidVA((uintptr_t)fName.Data)) {
				stringBuffers[j].resize(fName.Count + 1, 0);
				mem.AddScatterReadRequest(nameScatter, (uintptr_t)fName.Data,
					stringBuffers[j].data(), fName.Count * sizeof(wchar_t));
			}
		}

		mem.ExecuteReadScatter(nameScatter);
		mem.CloseScatterHandle(nameScatter);

		// Add new items to cache
		for (size_t j = 0; j < validIndices.size(); ++j) {
			size_t i = validIndices[j];

			ItemRenderer ir;
			ir.Actor = newActors[i];
			ir.Item = itemData[i].ItemType;
			ir.Weapon = weaponTypes[i];
			ir.Location = transforms[i].translation;

			if (!stringBuffers[j].empty()) {
				std::wstring ws(stringBuffers[j].data());
				ir.Name = std::string(ws.begin(), ws.end());
			}

			newCache[newActors[i]] = std::move(ir);
		}
	}

	// Replace cache atomically
	{
		std::unique_lock lk(Globals.itemMutex);
		cachedItems = std::move(newCache);
	}

	if (debug)
		std::cout << "ItemESP refreshed " << cachedItems.size() << " items ("
		          << newActors.size() << " new, " << existingActors.size() << " updated)\n";
}


void ItemESP::Render(const Camera& cam, int screenW, int screenH) {
	extern struct _GameCache cache;
	if (!Globals.itemsEnabled) return;
	if (!util::IsValidVA(cache.CameraManager.load()) || !util::IsValidVA(cache.LocalPawn.load())) return;

	std::vector<ItemRenderer> renderItems;
	{
		std::unique_lock lk(Globals.itemMutex);
		renderItems.reserve(cachedItems.size());


		FTransform dummy{};
		dummy.translation = Vector3(0, 0, 0);
		dummy.rot = { 0, 0, 0, 1 };
		dummy.scale = { 1, 1, 1 };

		for (const auto& [actor, cached] : cachedItems) {
			// Filter by category
			EItemCategory cat = GetItemCategory(cached.Item);
			if (Globals.enabledItemCategories.find(cat) == Globals.enabledItemCategories.end()) continue;

			// Filter weapons by type
			if (cat == EItemCategory::WEAPONS) {
				if (Globals.enabledWeaponTypes.find(cached.Weapon) == Globals.enabledWeaponTypes.end()) continue;
			}

			float dx = cached.Location.x - cam.Location.x;
			float dy = cached.Location.y - cam.Location.y;
			float dz = cached.Location.z - cam.Location.z;
			float distanceSq = dx * dx + dy * dy + dz * dz;
			float maxDistCm = Globals.MaxItemDistance * 100.0f;

			if (distanceSq > maxDistCm * maxDistCm) continue;

			ItemRenderer r = cached;

			FTransform worldTransform{};

			worldTransform.translation = cached.Location;
			worldTransform.rot = { 0, 0, 0, 1 };
			worldTransform.scale = { 1, 1, 1 };

			r.W2S = doMatrix(dummy, worldTransform, cam, screenW, screenH);
			r.distance = std::sqrt(distanceSq) / 100.0f - 3.0f;

			renderItems.push_back(std::move(r));
		}
	}

	{
		std::unique_lock lk(Globals.itemMutex);
		Globals.renderItems = std::move(renderItems);
	}
	if (debug)
		std::cout << "ItemESP rendered " << Globals.renderItems.size()
		<< " items this frame\n";
}