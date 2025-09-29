#include "ItemESP.h"
#include <Utils/Globals.h>
#include <DMALibrary/Memory/Memory.h>
#include <algorithm>
#include <iostream>

std::unordered_set<uintptr_t> ItemESP::oldItemSet;
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
    static std::unordered_set<EItemCategory> lastCategories;
    static std::unordered_set<EWeaponType> lastWeaponTypes;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    while (running) {
        bool filtersChanged = (lastCategories != Globals.enabledItemCategories) ||
            (lastWeaponTypes != Globals.enabledWeaponTypes);

        if (filtersChanged) {
            lastCategories = Globals.enabledItemCategories;
            lastWeaponTypes = Globals.enabledWeaponTypes;
            RefreshList();
        }

        RefreshList(); 
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}
void ItemESP::RefreshList() {
    extern struct _GameCache cache;
    if (!Globals.itemsEnabled) {
        std::unique_lock lk(Globals.itemMutex);
        Globals.renderItems.clear();
        cachedItems.clear();
        oldItemSet.clear();
        return;
    }

    if (!util::IsValidVA(cache.CameraManager.load()) ||
        !util::IsValidVA(cache.LocalPawn.load())) {
        return; 
    }


    std::unordered_set<uintptr_t> currentActors;
    {
        std::unique_lock lk(cache.cacheMutex);
        currentActors = cache.cachedItemActors;
    }

    if (currentActors.empty()) return;

    std::vector<uintptr_t> actorList(currentActors.begin(), currentActors.end());

    auto scatter = mem.CreateScatterHandle();
    std::vector<FSolarItemData> itemDataBatch(actorList.size());
    std::vector<EWeaponType> weaponTypes(actorList.size());

    for (size_t i = 0; i < actorList.size(); ++i) {
        mem.AddScatterReadRequest(scatter, actorList[i] + 0x360, &itemDataBatch[i], sizeof(FSolarItemData));
        mem.AddScatterReadRequest(scatter, actorList[i] + 0x56c, &weaponTypes[i], sizeof(EWeaponType));
    }
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);

    std::unordered_set<uintptr_t> newSet;
    for (size_t i = 0; i < actorList.size(); ++i) {
        uintptr_t actor = actorList[i];
        const auto& itemData = itemDataBatch[i];

        if (itemData.ItemType == EItemType::NONE) continue;

       
        EItemCategory cat = GetItemCategory(itemData.ItemType);
        if (Globals.enabledItemCategories.find(cat) == Globals.enabledItemCategories.end()) continue;

        if (cat == EItemCategory::WEAPONS) {
            if (Globals.enabledWeaponTypes.find(weaponTypes[i]) == Globals.enabledWeaponTypes.end()) continue;
        }

        newSet.insert(actor);
    }

    for (uintptr_t a : oldItemSet) {
        if (newSet.find(a) == newSet.end()) cachedItems.erase(a);
    }

    
    std::vector<uintptr_t> newItems;
    for (uintptr_t a : newSet) {
        if (oldItemSet.find(a) == oldItemSet.end()) {
            newItems.push_back(a);
        }
    }

    if (!newItems.empty()) {
       
        std::vector<FSolarItemData> newItemData(newItems.size());
        std::vector<EWeaponType> newWeaponTypes(newItems.size());

        auto stringScatter = mem.CreateScatterHandle();
        for (size_t i = 0; i < newItems.size(); ++i) {
            mem.AddScatterReadRequest(stringScatter, newItems[i] + 0x360, &newItemData[i], sizeof(FSolarItemData));
            mem.AddScatterReadRequest(stringScatter, newItems[i] + 0x56c, &newWeaponTypes[i], sizeof(EWeaponType));
        }
        mem.ExecuteReadScatter(stringScatter);
        mem.CloseScatterHandle(stringScatter);


        auto readFString = [&](const FString& f) -> std::string {
            if (!f.IsValid() || f.Count <= 0 || f.Count > 128 || !util::IsValidVA((uintptr_t)f.Data))
                return {};
            std::vector<wchar_t> buf(f.Count + 1);
            if (!mem.Read((uintptr_t)f.Data, buf.data(), f.Count * sizeof(wchar_t)))
                return {};
            buf[buf.size() - 1] = L'\0';
            return std::string(buf.begin(), buf.end() - 1);
            };

        for (size_t i = 0; i < newItems.size(); ++i) {
            ItemRenderer ir{};
            ir.Actor = newItems[i];
            ir.Item = newItemData[i].ItemType;
            ir.Weapon = newWeaponTypes[i];
            ir.Name = readFString(newItemData[i].Name);
            cachedItems[newItems[i]] = std::move(ir);
        }
    }

    oldItemSet = std::move(newSet);
}


void ItemESP::Render(const Camera& cam, int screenW, int screenH)
{
    extern struct _GameCache cache;
    if (!Globals.itemsEnabled) return;
    if (!util::IsValidVA(cache.CameraManager.load()) ||
        !util::IsValidVA(cache.LocalPawn.load())) return;

   
    std::vector<ItemRenderer> toRender;
    {
        std::unique_lock lk(Globals.itemMutex);
        toRender.reserve(cachedItems.size());
        for (auto& [actor, data] : cachedItems) toRender.push_back(data);
    }
    if (toRender.empty()) return;

   
    auto scatter = mem.CreateScatterHandle();
    std::vector<uintptr_t> rootComp(toRender.size(), 0);
    for (size_t i = 0; i < toRender.size(); ++i)
        mem.AddScatterReadRequest(scatter, toRender[i].Actor + 0x1b0, &rootComp[i], sizeof(uintptr_t));
    mem.ExecuteReadScatter(scatter);

    std::vector<FTransform> world(toRender.size());
    for (size_t i = 0; i < toRender.size(); ++i) {
        if (util::IsValidVA(rootComp[i]))
            mem.AddScatterReadRequest(scatter,
                rootComp[i] + Globals.offsets.componentToWorld,
                &world[i], sizeof(FTransform));
    }
    mem.ExecuteReadScatter(scatter);
    mem.CloseScatterHandle(scatter);

    
    std::vector<ItemRenderer> renderItems;
    renderItems.reserve(toRender.size());

    for (size_t i = 0; i < toRender.size(); ++i) {
        if (!util::IsValidVA(rootComp[i])) continue;
        const auto& it = toRender[i];
        ItemRenderer r = it;
        r.Location = world[i].translation;

       
        FTransform dummy{};
        dummy.translation = Vector3(0, 0, 0);
        dummy.rot = { 0,0,0,1 };
        dummy.scale = { 1,1,1 };
        

        r.W2S = doMatrix(dummy, world[i], cam, screenW, screenH);

      
        float dx = r.Location.x - cam.Location.x;
        float dy = r.Location.y - cam.Location.y;
        float dz = r.Location.z - cam.Location.z;
        r.distance = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0f - 3.0f;

        renderItems.push_back(std::move(r));
    }

  
    {
        std::unique_lock lk(Globals.itemMutex);
        Globals.renderItems = std::move(renderItems);
    }
    if (debug)
        std::cout << "ItemESP rendered " << Globals.renderItems.size()
        << " items this frame\n";
}