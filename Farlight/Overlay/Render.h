#pragma once
#include "Utils/Globals.h"
class Render {
public:
	void Loop();
	void Init();
};

static bool sidebarOpen = true;
static int currentPage = 0;  
static int selectedTab = 0;  

static void HoverTooltip(const char* txt)
{
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", txt);
}

inline void DrawVisualsTab()
{
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

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.3f, 0.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.4f, 0.0f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
    if (ImGui::CollapsingHeader("Player ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("ESP"); ImGui::NextColumn();
        
        ImGui::Checkbox("##ESP", &Globals.ESPEnabled);
        ImGui::NextColumn();

        ImGui::Text("Box style"); ImGui::NextColumn();
     
        ImGui::Combo("##boxstyle", &Globals.BoxStyle, "Corner\0Full\0");
        ImGui::NextColumn();

        ImGui::Text("Bones"); ImGui::NextColumn();
        static bool bones = true;
        ImGui::Checkbox("##bones", &bones);
        ImGui::NextColumn();

        ImGui::Text("Show name"); ImGui::NextColumn();
        static bool showName = true;
        ImGui::Checkbox("##showname", &showName);
        ImGui::NextColumn();

        ImGui::Text("Health bar"); ImGui::NextColumn();
        static bool healthBar = true;
        ImGui::Checkbox("##healthbar", &healthBar);
        ImGui::NextColumn();

        bool HeadEnabled = true;
        bool DistanceEnabled = true;

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.7f, 0.2f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.9f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
    if (ImGui::CollapsingHeader("Item ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Enable Items"); ImGui::NextColumn();
        ImGui::Checkbox("##enableitems", &Globals.itemsEnabled);
        ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();
}

inline void DrawSettingsTab()
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

    // --------------------------------------------------------------
    // 1️⃣ Configs section (Pink Accent)
    // --------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.2f, 0.6f, 0.7f));         // Pink tint for header
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.3f, 0.7f, 0.8f));  // Brighter pink on hover
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.4f, 0.8f, 1.0f));   // Solid pink when active
    if (ImGui::CollapsingHeader("Configs", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Save Config"); ImGui::NextColumn();
        static bool saveConfig = false;
        ImGui::Checkbox("##saveconfig", &saveConfig);
        ImGui::NextColumn();

        ImGui::Text("Load Config"); ImGui::NextColumn();
        static bool loadConfig = false;
        ImGui::Checkbox("##loadconfig", &loadConfig);
        ImGui::NextColumn();

        ImGui::Text("Config Name"); ImGui::NextColumn();
        static char configName[32] = "default";
        ImGui::InputText("##configname", configName, sizeof(configName));
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::Separator();
    ImGui::Spacing();

    // --------------------------------------------------------------
    // 2️⃣ Connection section (Purple Accent)
    // --------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 0.8f, 0.7f));         // Purple tint for header
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.3f, 0.9f, 0.8f));  // Brighter purple on hover
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.7f, 0.4f, 1.0f, 1.0f));   // Solid purple when active
    if (ImGui::CollapsingHeader("Connection", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Auto Connect"); ImGui::NextColumn();
        static bool autoConnect = true;
        ImGui::Checkbox("##autoconnect", &autoConnect);
        ImGui::NextColumn();

        ImGui::Text("Connection Timeout"); ImGui::NextColumn();
        static int timeout = 5000;
        ImGui::InputInt("##timeout", &timeout);
        ImGui::NextColumn();

        ImGui::Text("Server IP"); ImGui::NextColumn();
        static char serverIP[32] = "127.0.0.1";
        ImGui::InputText("##serverip", serverIP, sizeof(serverIP));
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.3f, 0.0f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.4f, 0.0f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
    if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnOffset(1, 150);

        ImGui::Text("Show FPS"); ImGui::NextColumn();
        static bool showFPS = true;
        ImGui::Checkbox("##showfps", &showFPS);
        ImGui::NextColumn();
        ImGui::Spacing(); // Add spacing

        ImGui::Text("Menu Keybind"); ImGui::NextColumn();
        static int currentKeyIndex = 0;
        const char* keyItems[] = { "INSERT", "DELETE", "HOME", "END", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };
        const int keyValues[] = { VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12 };

        if (ImGui::BeginCombo("##menukey", keyItems[currentKeyIndex]))
        {
            for (int i = 0; i < IM_ARRAYSIZE(keyItems); i++)
            {
                bool isSelected = (currentKeyIndex == i);
                if (ImGui::Selectable(keyItems[i], isSelected))
                {
                    currentKeyIndex = i;
                    Globals.OpenMenuKey = keyValues[i];
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::NextColumn();
        ImGui::Spacing(); // Add spacing

        // Refresh Cheat button
        ImGui::Text("Refresh Cheat"); ImGui::NextColumn();
        if (ImGui::Button("Refresh##refreshcheat", ImVec2(-1, 0))) {
            Globals.refreshcheat = !Globals.refreshcheat;
        }
        ImGui::NextColumn();
        ImGui::Spacing(); // Add spacing

        // Theme editor combo dropdown
        ImGui::Text("Theme Editor"); ImGui::NextColumn();
        static int selectedColor = 0;
        const char* colorItems[] = { "Menu Accent", "Text Color", "Background", "Header Color" };
        if (ImGui::BeginCombo("##themecombo", colorItems[selectedColor]))
        {
            for (int i = 0; i < IM_ARRAYSIZE(colorItems); i++)
            {
                bool isSelected = (selectedColor == i);
                if (ImGui::Selectable(colorItems[i], isSelected))
                {
                    selectedColor = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::NextColumn();
        ImGui::Spacing(); // Add spacing

        // Single color wheel that changes based on selection
        ImGui::Text("Edit Color"); ImGui::NextColumn();
        switch (selectedColor)
        {
        case 0: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.menuAccentColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 1: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.textColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 2: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.bgColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        case 3: ImGui::ColorEdit4("##colorwheel", (float*)&Globals.headerColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel); break;
        }
        // Removed ImGui::NextColumn() here to keep "Reset All Colors" in the same column
        ImGui::Spacing(); // Add spacing between color wheel and reset button
        if (ImGui::Button("Reset All Colors", ImVec2(-1, 0)))
        {
            Globals.menuAccentColor = ImVec4(244.0f, 5.0f, 9.0f, 0.8f);
            Globals.textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            Globals.bgColor = ImVec4(43.0f, 33.0f, 33.0f, 0.9f);
            Globals.headerColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        }
        ImGui::NextColumn(); // End of options, move to next row (column 0) for cleanup

        ImGui::Columns(1);
        ImGui::Spacing();
    }
    ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();
}