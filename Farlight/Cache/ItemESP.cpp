#include "ItemESP.h"
#include <Utils/Globals.h>
#include <DMALibrary/Memory/Memory.h>
#include <algorithm>
#include <iostream>

std::unordered_set<uintptr_t> ItemESP::oldItemSet;
std::unordered_map<uintptr_t, ItemRenderer> ItemESP::cachedItems;
std::thread ItemESP::updateThread;
bool ItemESP::running = false;
bool ItemESP::debug = false;

void ItemESP::Start(bool dbg) {
    if (!running) {
        debug = dbg;
        running = true;
        updateThread = std::thread(UpdateLoop);
        if (debug) std::cout << "ItemESP thread started\n";
    }
}

void ItemESP::Stop() {
    running = false;
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void ItemESP::UpdateLoop() {
    extern struct _GameCache cache;

    while (running) {
        if (!Globals.itemsEnabled) {
            // Clear items when disabled - but don't clear cache.Items
            {
                std::unique_lock lock(Globals.itemMutex);
                Globals.renderItems.clear();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            continue;
        }

        if (util::IsValidVA(cache.CameraManager.load())) {
            std::vector<ItemEntry> items;
            Camera localCam;

            // Single atomic read to prevent race conditions
            {
                std::unique_lock lock(cache.cacheMutex);
                items = cache.Items;
            }
            localCam = cache.LocalCamera; // This doesn't need mutex in your original code

            Update(items, localCam, Globals.screenWidth, Globals.screenHeight);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Reduced frequency to reduce flicker
    }
}

void ItemESP::Update(const std::vector<ItemEntry>& items, const Camera& cam, int screenW, int screenH) {
    std::unordered_set<uintptr_t> newItemSet;

    // Use your original filtering logic - don't break what works
    for (const auto& item : items) {
        if (!util::IsValidVA(item.actor))
            continue;

        EItemCategory category = GetItemCategory(item.ItemType);
        if (Globals.enabledItemCategories.find(category) == Globals.enabledItemCategories.end())
            continue;
        if (category == EItemCategory::WEAPONS) {
            if (Globals.enabledWeaponTypes.find(item.WeaponType) == Globals.enabledWeaponTypes.end())
                continue;
        }

        newItemSet.insert(item.actor);
    }

    // Calculate differences
    std::vector<uintptr_t> removed, added;
    std::set_difference(oldItemSet.begin(), oldItemSet.end(),
        newItemSet.begin(), newItemSet.end(),
        std::back_inserter(removed));
    std::set_difference(newItemSet.begin(), newItemSet.end(),
        oldItemSet.begin(), oldItemSet.end(),
        std::back_inserter(added));

    // Remove deleted actors from cache
    for (uintptr_t actor : removed) {
        cachedItems.erase(actor);
        if (debug) std::cout << "Removed item: 0x" << std::hex << actor << std::dec << "\n";
    }

    // Process new items
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
            for (const auto& item : items) {
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
            ir.Location = compToWorlds[i].translation;
            float dx = ir.Location.x - cam.Location.x;
            float dy = ir.Location.y - cam.Location.y;
            float dz = ir.Location.z - cam.Location.z;
            float distance = sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f - 3.0f;
            ir.distance = distance;

            FSolarItemData itemData = mem.Read<FSolarItemData>(actor + 0x360);
            auto readFString = [&](const FString& f) -> std::string {
                if (!f.IsValid() || f.Count <= 0 || f.Count > 128 || !util::IsValidVA((uintptr_t)f.Data))
                    return {};
                std::vector<wchar_t> buf(f.Count + 1);
                if (!mem.Read((uintptr_t)f.Data, buf.data(), f.Count * sizeof(wchar_t)))
                    return {};
                buf[buf.size() - 1] = L'\0';
                return std::string(buf.begin(), buf.end() - 1);
                };
            ir.Name = readFString(itemData.Name);

            cachedItems[actor] = std::move(ir);
        }

        if (debug) {
            std::cout << "Items processed - Added: " << added.size() << ", Total cached: " << cachedItems.size() << "\n";
        }
    }

    oldItemSet = std::move(newItemSet);

    // Process cached items for rendering - ONLY CHANGE: Single atomic update
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

            ItemRenderer renderItem = item;
            renderItem.W2S = screenPos;
            renderItem.distance = distance;
            renderItems.push_back(std::move(renderItem));
        }
    }

    // MAIN FIX: Single atomic update instead of modifying during iteration
    {
        std::unique_lock lock(Globals.itemMutex);
        Globals.renderItems = std::move(renderItems);
    }
}