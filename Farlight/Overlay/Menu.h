#pragma once

#include  "Render.h"

void SetOverlayMode(HWND hwnd, bool isTransparent, bool menuVisible);
void SetOverlayMode2(HWND hwnd, int overlayMode, bool showMenu);
void SwitchToMonitor(HWND hwnd, int monitorIndex);
void AddVerticalSpacing(float spacing);
void HoverTooltip(const char* txt);
void DrawNamedColorPicker(const char* name, ImVec4& color);
void DrawItemCategoryFilter();
void DrawWeaponItemFilter();
void DrawSimpleSettings();
void DrawMainMenu();
void DrawSidebar();
void DrawVisualsTab();
void DrawAimbotTab();
void DrawMiscTab();
void DrawColorsTab();
void DrawSettingsTab();

 
static int currentPage = 0;
static int selectedTab = 0;
 

class ScopedStyleColor {
public:
    ScopedStyleColor(ImGuiCol idx, const ImVec4& col) : idx_(idx) {
        ImGui::PushStyleColor(idx_, col);
    }
    ~ScopedStyleColor() {
        ImGui::PopStyleColor();
    }

private:
    ImGuiCol idx_;
};

struct StyleColorScope {
    int count = 0;
    StyleColorScope(std::initializer_list<std::pair<ImGuiCol, ImVec4>> cols) {
        for (auto& c : cols) { ImGui::PushStyleColor(c.first, c.second); ++count; }
    }
    ~StyleColorScope() { if (count) ImGui::PopStyleColor(count); }
    StyleColorScope(const StyleColorScope&) = delete;
    StyleColorScope& operator=(const StyleColorScope&) = delete;
};


