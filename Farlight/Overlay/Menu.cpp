#include "Menu.h"


void DrawMainMenu() {
	const char* tabs[] = { "Visuals", "Aimbot", "Misc", "Colors", "Settings" };
	int tabCount = sizeof(tabs) / sizeof(tabs[0]);

	float buttonHeight = 40.0f;
	float totalButtonHeight = tabCount * buttonHeight;
	float availableForButtons = ImGui::GetContentRegionAvail().y - 100;
	float spacingBetweenButtons = (availableForButtons - totalButtonHeight) / (tabCount + 1);
	spacingBetweenButtons = std::max(spacingBetweenButtons, 8.0f);

	ImGui::Dummy(ImVec2(0, spacingBetweenButtons * 0.6f));

	if (Globals.headerFont) ImGui::PushFont(Globals.headerFont);

	for (int i = 0; i < tabCount; ++i) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

		if (ImGui::Button(tabs[i], ImVec2(270, buttonHeight))) {
			selectedTab = i;
			currentPage = 1;
		}

		ImGui::PopStyleColor(3);
		ImGui::Dummy(ImVec2(0, spacingBetweenButtons * 0.7f));
	}

	if (Globals.headerFont) ImGui::PopFont();
}
void DrawSidebar() {
	static bool wasDownLastFrame = false;

	bool isDown = (GetAsyncKeyState(Globals.OpenMenuKey) & 0x8000 || mem.GetKeyboard()->IsKeyDown(Globals.OpenMenuKey));

	if (isDown && !wasDownLastFrame) {
		Globals.sidebarOpen = !Globals.sidebarOpen;

	}
	wasDownLastFrame = isDown;

	if (!Globals.sidebarOpen) return;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 darkBg = Globals.bgColor;
	ImVec4 sectionBg = Globals.headerColor;
	ImVec4 accentRed = Globals.menuAccentColor;

	ImGui::SetNextWindowPos(ImVec2(Globals.screenWidth - 300, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, Globals.screenHeight), ImGuiCond_Always);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, darkBg);
	ImGui::PushStyleColor(ImGuiCol_Border, accentRed);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 18));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 7.0f);

	ImGui::Begin("##Sidebar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar);


	if (Globals.logoTexture != 0) {
		float contentWidth = ImGui::GetContentRegionAvail().x;
		float texW = static_cast<float>(Globals.logoWidth);
		float texH = static_cast<float>(Globals.logoHeight);
		if (texW <= 0 || texH <= 0) { texW = 1080.0f; texH = 608.0f; }

		const float maxHeight = 150.0f;
		const float maxWidth = contentWidth;
		float scale = std::min(maxWidth / texW, maxHeight / texH);
		float drawW = texW * scale;
		float drawH = texH * scale;

		ImGui::Dummy(ImVec2(0, 8));
		float offsetX = (contentWidth - drawW) * 0.5f;
		ImGui::SetCursorPosX(offsetX);
		ImGui::Image(Globals.logoTexture, ImVec2(drawW, drawH));
		ImGui::Dummy(ImVec2(0, 8));
	}

	ImGui::Separator();
	ImGui::Spacing();


	if (Globals.logoFont) {
		ImGui::PushFont(Globals.logoFont);
		const char* title = "Makimura.DeV";
		float txtW = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((300.0f - txtW) * 0.5f);
		ImGui::TextColored(accentRed, "%s", title);
		ImGui::PopFont();
	}
	else {
		ImGui::SetCursorPosX((300.0f - ImGui::CalcTextSize("Makimura.dev[Fallback]").x) * 0.5f);
		ImGui::TextColored(accentRed, "Makimura.Dev[Fallback]");
	}

	ImGui::Spacing();
	if (Globals.headerFont) {
		ImGui::PushFont(Globals.headerFont);
		const char* sub = "Farlight Manager";
		float subW = ImGui::CalcTextSize(sub).x;
		ImGui::SetCursorPosX((300.0f - subW) * 0.5f);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", sub);
		ImGui::PopFont();
	}

	ImGui::Spacing();
	//REMOVED COMMENTS..

	float metricsHeight = 100.0f;
	float availableHeight = ImGui::GetContentRegionAvail().y;
	float mainContentHeight = availableHeight - metricsHeight - 60;

	ImGui::BeginChild("Content", ImVec2(0, mainContentHeight), false, ImGuiWindowFlags_NoScrollbar);

	if (Globals.regularFont) ImGui::PushFont(Globals.regularFont);


	if (currentPage == 0) {

		DrawMainMenu();
	}
	else {

		switch (selectedTab) {
		case 0: DrawVisualsTab(); break;
		case 1: DrawAimbotTab();  break;
		case 2: DrawMiscTab();    break;
		case 3: DrawColorsTab();  break;
		case 4: DrawSettingsTab(); break;
		}
	}

	if (Globals.regularFont) ImGui::PopFont();
	ImGui::EndChild();


	if (currentPage > 0) {
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Button, Globals.headerColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
			Globals.headerColor.x * 1.2f,
			Globals.headerColor.y * 1.2f,
			Globals.headerColor.z * 1.2f,
			Globals.headerColor.w
		));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
			Globals.headerColor.x * 0.8f,
			Globals.headerColor.y * 0.8f,
			Globals.headerColor.z * 0.8f,
			Globals.headerColor.w
		));

		if (Globals.headerFont) ImGui::PushFont(Globals.headerFont);

		float buttonWidth = 270;
		float buttonHeight = 40.0f;
		ImGui::SetCursorPosX((300.0f - buttonWidth) * 0.5f);

		if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight))) {
			currentPage = 0;
		}

		if (Globals.headerFont) ImGui::PopFont();
		ImGui::PopStyleColor(3);
	}


	ImGui::Separator();
	ImGui::Spacing();

	ImVec2 metricsStart = ImGui::GetCursorScreenPos();
	ImVec2 metricsSize = ImVec2(264, 70);

	ImGui::GetWindowDrawList()->AddRectFilled(
		metricsStart,
		ImVec2(metricsStart.x + metricsSize.x, metricsStart.y + metricsSize.y),
		ImColor(Globals.headerColor),
		4.0f
	);

	ImGui::GetWindowDrawList()->AddRect(
		metricsStart,
		ImVec2(metricsStart.x + metricsSize.x, metricsStart.y + metricsSize.y),
		ImColor(accentRed),
		4.0f,
		0,
		2.0f
	);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

	if (Globals.regularFont) ImGui::PushFont(Globals.regularFont);

	std::ostringstream renderFpsStream;
	renderFpsStream << std::fixed << std::setprecision(0) << "OVERLAY: " << ImGui::GetIO().Framerate << " FPS";
	std::string renderFpsText = renderFpsStream.str();

	ImGui::SetCursorPosX((300.0f - ImGui::CalcTextSize(renderFpsText.c_str()).x) * 0.5f);
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", renderFpsText.c_str());

	std::string gameFpsText = "DMA: " + std::to_string(Globals.readFPS) + " FPS";
	ImGui::SetCursorPosX((300.0f - ImGui::CalcTextSize(gameFpsText.c_str()).x) * 0.5f);
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 178.0f, 1.0f), "%s", gameFpsText.c_str());

	if (Globals.regularFont) ImGui::PopFont();

	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
}


 void AddVerticalSpacing(float spacing) {
    ImGui::Dummy(ImVec2(0.0f, spacing));
    ImGui::Spacing();
}

 

static void HoverTooltip(const char* txt)
{
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", txt);
}
static void DrawNamedColorPicker(const char* name, ImVec4& color)
{
    ImGui::Text("%s", name); // show the variable name
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueWheel
        | ImGuiColorEditFlags_AlphaBar
        | ImGuiColorEditFlags_NoSidePreview
        | ImGuiColorEditFlags_NoInputs
        | ImGuiColorEditFlags_NoOptions;
    std::string id = std::string("##picker_") + name; // hidden ID to avoid conflicts
    ImGui::ColorEdit4(id.c_str(), (float*)&color, flags);
    ImGui::Spacing();
}

 void DrawItemCategoryFilter() {
	
     static const std::unordered_map<EItemCategory, const char*> categoryNames = {
        {EItemCategory::SHIELDS, "Shields"},
        {EItemCategory::ARMORS, "Armors"},
        {EItemCategory::AMMO, "Ammo"},
        {EItemCategory::ENERGY, "Energy"},
        {EItemCategory::BACKPACKS, "Backpacks"},
        {EItemCategory::WEAPONS, "Weapons"},
        {EItemCategory::ENHANCERS, "Enhancers"},
        {EItemCategory::REVIVE, "Revive"},
        {EItemCategory::LOOTBOXES, "Lootboxes"},
        {EItemCategory::QUEST, "Quest Items"},
        {EItemCategory::MISC, "Miscellaneous"}
	 };
     std::vector<EItemCategory> categories = {
        EItemCategory::SHIELDS,
        EItemCategory::ARMORS,
        EItemCategory::AMMO,
        EItemCategory::ENERGY,
        EItemCategory::BACKPACKS,
        EItemCategory::WEAPONS,
        EItemCategory::ENHANCERS,
        EItemCategory::REVIVE,
        EItemCategory::LOOTBOXES,
        EItemCategory::QUEST,
        EItemCategory::MISC
	 };
    std::string preview = "Select Types";
    if (!Globals.enabledItemCategories.empty()) {
        preview.clear();
        for (auto it = Globals.enabledItemCategories.begin(); it != Globals.enabledItemCategories.end(); ++it) {
            if (categoryNames.count(*it)) {
                preview += categoryNames.at(*it);
                preview += ", ";
            }
        }
        if (!preview.empty()) { preview.resize(preview.size() - 2); }
    }


    StyleColorScope comboStyle({
        { ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.8f, 0.7f) },
        { ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 1.0f, 0.8f) },
        { ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 1.0f, 1.0f) }
        });


    if (ImGui::BeginCombo("##ItemCategoryFilter", preview.c_str(), ImGuiComboFlags_HeightLarge)) {
        for (auto category : categories) {
            const char* name = categoryNames.count(category) ? categoryNames.at(category) : "Unknown";
            bool selected = Globals.enabledItemCategories.count(category) > 0;


            if (ImGui::Selectable(name, selected)) {
                if (selected) Globals.enabledItemCategories.erase(category);
                else Globals.enabledItemCategories.insert(category);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}


 void DrawWeaponItemFilter()
{
     static const std::unordered_map<EWeaponType, const char*> weaponNames = {
     {EWeaponType::AssualtRifle, "Assault Rifle"},
     {EWeaponType::Submachinegun, "Submachine Gun"},
     {EWeaponType::Shotgun, "Shotgun"},
     {EWeaponType::Sniper, "Sniper Rifle"},
     {EWeaponType::ShooterRifle, "DMR"},
     {EWeaponType::HandGun, "Handgun"},
     {EWeaponType::LightMachineGun, "Light Machine Gun"}
     };

    std::vector<EWeaponType> weapons = {
    EWeaponType::AssualtRifle,
    EWeaponType::Submachinegun,
    EWeaponType::Shotgun,
    EWeaponType::Sniper,
    EWeaponType::ShooterRifle,
    EWeaponType::HandGun,
    EWeaponType::LightMachineGun
    };

    const int columns = 4;
    ImGui::Columns(columns, nullptr, false);
    ImGui::SetColumnOffset(0, 0);

    int idx = 0;
    for (auto w : weapons)
    {
        bool selected = Globals.enabledWeaponTypes.count(w) > 0;
        const char* label = weaponNames.count(w) ? weaponNames.at(w) : "Unknown";


        ImVec4 base = selected ? ImVec4(0.6f, 0.2f, 0.9f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        ImVec4 hov = ImVec4(std::min(base.x + 0.08f, 1.0f), std::min(base.y + 0.08f, 1.0f), std::min(base.z + 0.08f, 1.0f), 1.0f);
        ImVec4 act = ImVec4(std::min(base.x + 0.16f, 1.0f), std::min(base.y + 0.16f, 1.0f), std::min(base.z + 0.16f, 1.0f), 1.0f);


        {
            StyleColorScope btnStyle({
                { ImGuiCol_Button, base },
                { ImGuiCol_ButtonHovered, hov },
                { ImGuiCol_ButtonActive, act }
                });


            std::string btnLabel = std::string(label) + "##weapon" + std::to_string(static_cast<int>(w));
            if (ImGui::Button(btnLabel.c_str(), ImVec2(-FLT_MIN, 24))) {
                if (selected) Globals.enabledWeaponTypes.erase(w);
                else Globals.enabledWeaponTypes.insert(w);
            }
        }


        ImGui::NextColumn();
        ++idx;
    }

    ImGui::Columns(1);
}


 void DrawVisualsTab() {
    if (Globals.headerFont) {
        ImGui::PushFont(Globals.headerFont);
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "VISUALS");
        ImGui::PopFont();
    }
    else {
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "VISUALS");
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushItemWidth(-1);

    auto SectionHeader = [&](const char* label, ImVec4 base, std::function<void()> body) {
        ScopedStyleColor c1(ImGuiCol_Header, base);
        ScopedStyleColor c2(ImGuiCol_HeaderHovered, ImVec4(base.x + 0.2f, base.y + 0.2f, base.z + 0.2f, 0.9f));
        ScopedStyleColor c3(ImGuiCol_HeaderActive, ImVec4(base.x + 0.3f, base.y + 0.3f, base.z + 0.3f, 1.0f));
        if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
            AddVerticalSpacing(8.0f);
            body();
            AddVerticalSpacing(10.0f);
        }
        };

    // -------- Player ESP --------
    SectionHeader("Player ESP", { 0.8f,0.3f,0.0f,0.7f }, [&]() {
        ImGui::Columns(2, nullptr, false); ImGui::SetColumnOffset(1, 150);

        const char* labels[] = { "ESP", "Bones", "Show Head", "Show Name", "Health Bar", "Snaplines" };
        bool* states[] = { &Globals.ESPEnabled, &Globals.DrawBones, &Globals.DrawHeadCircle, &Globals.DrawNames,&Globals.DrawHP, &Globals.DrawTraceline };

        for (int i = 0; i < 6; i++) {
            ImGui::Text(labels[i]); ImGui::NextColumn();
            ImGui::Checkbox(("##" + std::string(labels[i])).c_str(), states[i]); ImGui::NextColumn();
        }

        ImGui::Text("Box Style"); ImGui::NextColumn();
        ImGui::Combo("##boxstyle", &Globals.BoxStyle, "Corner\0Full\0"); ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Text("Max Player Distance:");
        ImGui::SliderFloat("##DrawDistance", &Globals.maxDistance, 10.f, 1000.f, "%.0f m");
        });

    // -------- Item ESP --------
    SectionHeader("Item ESP", { 0.2f,0.7f,0.2f,0.7f }, [&]() {
        ImGui::Columns(2, nullptr, false); ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Enable Items"); ImGui::NextColumn(); ImGui::Checkbox("##enableitems", &Globals.itemsEnabled); ImGui::NextColumn();
        ImGui::Text("Draw Name"); ImGui::NextColumn(); ImGui::Checkbox("##drawitemname", &Globals.DrawItemName); ImGui::NextColumn();
        ImGui::Text("Draw Box"); ImGui::NextColumn(); ImGui::Checkbox("##drawitembox", &Globals.DrawItemBox); ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Text("Max Item Distance:");
        ImGui::SliderFloat("##ItemDrawDistance", &Globals.MaxItemDistance, 10.f, 1000.f, "%.0f m");

        DrawItemCategoryFilter();
        });

    // -------- Weapon ESP --------
    SectionHeader("Weapon ESP", { 0.5f,0.1f,0.8f,0.8f }, [&]() {
        DrawWeaponItemFilter();
        });

    // -------- ESP Colors --------
    SectionHeader("ESP Colors", { 0.1f, 0.6f, 0.9f, 0.7f }, [&]() {

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Select ESP Element"); ImGui::NextColumn();

        static int selectedESPColor = 0;
        const char* espItems[] = {
            "Player Box", "Bones", "Name", "Distance",
            "Head Circle", "Items", "Weapons", "FOV Circle"
        };

        if (ImGui::BeginCombo("##espcombo", espItems[selectedESPColor])) {
            for (int i = 0; i < IM_ARRAYSIZE(espItems); i++) {
                bool isSelected = (selectedESPColor == i);
                if (ImGui::Selectable(espItems[i], isSelected))
                    selectedESPColor = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::NextColumn();

        ImGui::Text("Edit Color"); ImGui::NextColumn();

        switch (selectedESPColor) {
        case 0: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorPlayerBox, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 1: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorBones, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 2: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorName, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 3: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorDistance, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 4: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorHead, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 5: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorItems, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 6: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorWeapons, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 7: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.ColorFOV, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        }

        ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Spacing();

        ImGui::Text("Line Thickness");
        ImGui::SliderFloat("##LineThickness", &Globals.LineThickness, 0.5f, 5.0f, "%.1f");

        ImGui::PopItemWidth();
        });
}
 void DrawSettingsTab()
{
    if (Globals.headerFont) {
        ImGui::PushFont(Globals.headerFont);
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "SETTINGS");
        ImGui::PopFont();
    }
    else {
        ImGui::TextColored(ImVec4(1, 1, 1, 1), "SETTINGS");
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushItemWidth(-1);

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.3f, 0.7f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.4f, 0.8f, 1.0f));
    // ---------------------- CONFIGS ----------------------
    if (ImGui::CollapsingHeader("Configs", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(3, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Save Config"); ImGui::NextColumn();
        static bool saveConfig = false;
        ImGui::Checkbox("##saveconfig", &saveConfig);
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Text("Load Config"); ImGui::NextColumn();
        static bool loadConfig = false;
        ImGui::Checkbox("##loadconfig", &loadConfig);
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Text("Config Name"); ImGui::NextColumn();
        static char configName[32] = "default";
        ImGui::InputText("##configname", configName, sizeof(configName));
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::Separator();
    ImGui::Spacing();

    // ---------------------- MENU ----------------------
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 0.8f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.3f, 0.9f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.7f, 0.4f, 1.0f, 1.0f));

    if (ImGui::CollapsingHeader("Menu", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(3, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Monitor"); ImGui::NextColumn();
        static int currentMonitor = 0;
        int monitorCount = GetSystemMetrics(SM_CMONITORS);
        if (ImGui::Combo("##monitor", &currentMonitor,
            [](void*, int idx, const char** out_text) {
                static char buffer[64];
                snprintf(buffer, sizeof(buffer), "Monitor %d", idx + 1);
                *out_text = buffer;
                return true;
            }, nullptr, monitorCount)) {
            SwitchToMonitor(Globals.overlayHWND, currentMonitor);
        }
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Text("Overlay Mode"); ImGui::NextColumn();
        const char* overlayModes[] = { "Transparent ESP", "Black Fuser" };
        if (ImGui::Combo("##overlaymode", &Globals.overlayMode, overlayModes, 2)) {
            SetOverlayMode(Globals.overlayHWND, Globals.overlayMode == 0, Globals.sidebarOpen);
        }
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
    }

    // ---------------------- OPTIONS ----------------------
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.3f, 0.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.4f, 0.0f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));

    if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(3, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Show FPS"); ImGui::NextColumn();
        static bool showFPS = true;
        ImGui::Checkbox("##showfps", &showFPS);
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Text("Menu Keybind"); ImGui::NextColumn();
        static int currentKeyIndex = 0;
        const char* keyItems[] = { "INSERT", "DELETE", "HOME", "END", "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12" };
        const int keyValues[] = { VK_INSERT,VK_DELETE,VK_HOME,VK_END,VK_F1,VK_F2,VK_F3,VK_F4,VK_F5,VK_F6,VK_F7,VK_F8,VK_F9,VK_F10,VK_F11,VK_F12 };
        if (ImGui::BeginCombo("##menukey", keyItems[currentKeyIndex])) {
            for (int i = 0; i < IM_ARRAYSIZE(keyItems); i++) {
                bool isSelected = (currentKeyIndex == i);
                if (ImGui::Selectable(keyItems[i], isSelected)) {
                    currentKeyIndex = i;
                    Globals.OpenMenuKey = keyValues[i];
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::NextColumn(); ImGui::NextColumn();

        ImGui::Text("Refresh Cheat"); ImGui::NextColumn();
        if (ImGui::Button("Refresh##refreshcheat", ImVec2(-1, 0))) {
            Globals.refreshcheat = !Globals.refreshcheat;
        }
        ImGui::NextColumn(); ImGui::NextColumn();
        ImGui::Text("Theme Editor"); ImGui::NextColumn();
        static int selectedColor = 0;
        const char* colorItems[] = { "Menu Accent", "Text Color", "Background", "Header Color" };
        if (ImGui::BeginCombo("##themecombo", colorItems[selectedColor])) {
            for (int i = 0; i < IM_ARRAYSIZE(colorItems); i++) {
                bool isSelected = (selectedColor == i);
                if (ImGui::Selectable(colorItems[i], isSelected)) selectedColor = i;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::NextColumn();

        // Place color picker directly below combo, full width
        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Text("Edit Color");
        switch (selectedColor)
        {
        case 0: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.menuAccentColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 1: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.textColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 2: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.bgColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 3: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.headerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        }
        ImGui::Spacing();
        if (ImGui::Button("Reset Colors", ImVec2(-1, 0)))
        {
            Globals.menuAccentColor = ImVec4(0.8f, 0.2f, 0.6f, 1.0f);
            Globals.textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            Globals.bgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
            Globals.headerColor = ImVec4(0.5f, 0.2f, 0.8f, 1.0f);
        }
       
        ImGui::NextColumn();  

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

}



 void DrawMiscTab() {
    ImGui::PushFont(Globals.headerFont); // Use header font for bigger text
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "MISCELLANEOUS");
    ImGui::PopFont();
    ImGui::Separator();

}

void DrawColorsTab() {
    ImGui::PushFont(Globals.headerFont); // Use header font for bigger text
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "COLOR SETTINGS");
    ImGui::PopFont();
    ImGui::Separator();


}

 void DrawAimbotTab()
{
    auto SectionHeader = [&](const char* label, ImVec4 base, const std::function<void()>& body) {
        ScopedStyleColor c1(ImGuiCol_Header, base);
        ScopedStyleColor c2(ImGuiCol_HeaderHovered, ImVec4(base.x + 0.12f, base.y + 0.12f, base.z + 0.12f, 0.95f));
        ScopedStyleColor c3(ImGuiCol_HeaderActive, ImVec4(base.x + 0.2f, base.y + 0.2f, base.z + 0.2f, 1.0f));
        if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
            AddVerticalSpacing(6.0f);
            body();
            AddVerticalSpacing(8.0f);
        }
        };

    SectionHeader("Aimbot", ImVec4(0.85f, 0.35f, 0.05f, 0.8f), [&] {
        ImGui::Columns(2, nullptr, false); ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Enable Aimbot:"); ImGui::NextColumn(); ImGui::Checkbox("##AimbotEnable", &Globals.EnableAimbot); ImGui::NextColumn();
        ImGui::Text("Smoothing"); ImGui::NextColumn(); ImGui::SliderFloat("##aimbot_smooth", &Globals.settings.smoothing, 0.1f, 20.0f, "%.1f"); HoverTooltip("Higher = slower, smoother aim"); ImGui::NextColumn();
        ImGui::Text("Ignore Knocked"); ImGui::NextColumn(); ImGui::Checkbox("##IgnoreKnocked", &Globals.settings.IgnoreKnocked); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Text("Max Aim Distance:");
        ImGui::SliderFloat("##MaxAimdist", &Globals.settings.AimbotMaxDistance, 10.f, 1000.f, "%.0f m");
        ImGui::Columns(1);
        ImGui::Spacing();
        ImGui::Text("FOV"); ImGui::NextColumn(); ImGui::SliderFloat("##aimbot_fov", &Globals.settings.fov, 90.0f, 360.0f, "%.1f°"); HoverTooltip("Field of View for aimbot"); ImGui::NextColumn();

        });

    SectionHeader("Triggerbot", ImVec4(0.0f, 0.85f, 0.9f, 0.85f), [&] {
        ImGui::Columns(2, nullptr, false); ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Enabled"); ImGui::NextColumn();
        ImGui::Checkbox("##trigger_enabled", &Globals.settings.TriggerbotEnabled); HoverTooltip("Toggle triggerbot"); ImGui::NextColumn();

        ImGui::Text("Delay (ms)"); ImGui::NextColumn();
        ImGui::SliderInt("##trigger_delay", &Globals.settings.TriggerDelayMS, 0, 500, "%d");

        HoverTooltip("Delay after aim before firing"); ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
        });

    SectionHeader("Target Selection", ImVec4(0.55f, 0.15f, 0.8f, 0.85f), [&] {
        ImGui::Columns(2, nullptr, false); ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Priority"); ImGui::NextColumn();
        const char* priorities[] = { "Head", "Body" };
        int pIdx = static_cast<int>(Globals.TargetPriority);
        if (ImGui::Combo("##target_priority", &pIdx, priorities, IM_ARRAYSIZE(priorities)))
            Globals.TargetPriority = static_cast<ETargetPriority>(pIdx);
        ImGui::Columns(1);
        ImGui::Spacing();
        });
}


