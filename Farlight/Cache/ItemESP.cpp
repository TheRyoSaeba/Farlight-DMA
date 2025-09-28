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
void ItemESP::RefreshLoop()
{
    extern struct _GameCache cache;
    while (running) {
        RefreshList();  
        std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_MS));
    }
}
void ItemESP::RefreshList()
{
    extern struct _GameCache cache;
    if (!Globals.itemsEnabled) {
        std::unique_lock lk(Globals.itemMutex);
        Globals.renderItems.clear();
        cachedItems.clear();
        oldItemSet.clear();
        return;
    }
    if (!util::IsValidVA(cache.CameraManager.load()) ||
        !util::IsValidVA(cache.LocalPawn.load())) return;

    std::vector<ItemEntry> curItems;
    {
        std::unique_lock lk(cache.cacheMutex);
        curItems = cache.Items;
    }

    std::unordered_set<uintptr_t> newSet;
    for (const auto& i : curItems) {
        if (!util::IsValidVA(i.actor)) continue;
        EItemCategory cat = GetItemCategory(i.ItemType);
        if (Globals.enabledItemCategories.find(cat) == Globals.enabledItemCategories.end()) continue;
        if (cat == EItemCategory::WEAPONS &&
            Globals.enabledWeaponTypes.find(i.WeaponType) == Globals.enabledWeaponTypes.end())
            continue;
        newSet.insert(i.actor);
    }

    for (uintptr_t a : oldItemSet) {
        if (newSet.find(a) == newSet.end()) cachedItems.erase(a);
    }
    for (uintptr_t a : newSet) {
        if (oldItemSet.find(a) != oldItemSet.end()) continue; 
        const ItemEntry* src = nullptr;
        for (const auto& it : curItems) if (it.actor == a) { src = &it; break; }
        if (!src) continue;
        ItemRenderer ir{};
        ir.Actor = a;
        ir.Item = src->ItemType;
        ir.Weapon = src->WeaponType;
        FSolarItemData itemData = mem.Read<FSolarItemData>(a + 0x360);
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
        cachedItems.emplace(a, std::move(ir));
    }
    oldItemSet = std::move(newSet);
}


void ItemESP::Render(const Camera& cam, int screenW, int screenH)
{
    extern struct _GameCache cache;
    if (!Globals.itemsEnabled) return;
    if (!util::IsValidVA(cache.CameraManager.load()) ||
        !util::IsValidVA(cache.LocalPawn.load())) return;

    // copy the current list of cached items (static data only)
    std::vector<ItemRenderer> toRender;
    {
        std::unique_lock lk(Globals.itemMutex);
        toRender.reserve(cachedItems.size());
        for (auto& [actor, data] : cachedItems) toRender.push_back(data);
    }
    if (toRender.empty()) return;

    // ----- scatter read root‑component & world transform for *all* items -----
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

    // ----- build final render list (screen‑space, distance) -----
    std::vector<ItemRenderer> renderItems;
    renderItems.reserve(toRender.size());

    for (size_t i = 0; i < toRender.size(); ++i) {
        if (!util::IsValidVA(rootComp[i])) continue;
        const auto& it = toRender[i];
        ItemRenderer r = it;
        r.Location = world[i].translation;

        // world → screen
        FTransform dummy{};
        dummy.translation = Vector3(0, 0, 0);
        dummy.rot = { 0,0,0,1 };
        dummy.scale = { 1,1,1 };
        

        r.W2S = doMatrix(dummy, world[i], cam, screenW, screenH);

        // distance (same formula used for players)
        float dx = r.Location.x - cam.Location.x;
        float dy = r.Location.y - cam.Location.y;
        float dz = r.Location.z - cam.Location.z;
        r.distance = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0f - 3.0f;

        renderItems.push_back(std::move(r));
    }

    // ----- atomic update of the global rendering vector -----
    {
        std::unique_lock lk(Globals.itemMutex);
        Globals.renderItems = std::move(renderItems);
    }
    if (debug)
        std::cout << "ItemESP rendered " << Globals.renderItems.size()
        << " items this frame\n";
}