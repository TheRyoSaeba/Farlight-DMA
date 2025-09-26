#define STB_IMAGE_IMPLEMENTATION
#define NOMINMAX
#include <Overlay/Render.h>
#include <Utils/Globals.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"
#include <d3d11.h>

#include "ImGUI/D3DX11tex.h"
#include "ImGUI/stb_image.h"
#include <algorithm>
#include <resource.h>
#include <DMALibrary/Memory/Memory.h>

//DRAWING SECTION 
//TODO Move Drawbox from CFMANAGER
//draw box decide color  and thickness based on teamid

//let's use a boxstyle variable to decide between corner and full box

void DrawBox(Vector2 position, float width, float height, ImColor color, float thickness = 1.0f, int boxStyle = 0) {
	ImVec2 tl(position.x - width / 2, position.y - height / 2);
	ImVec2 br(position.x + width / 2, position.y + height / 2);
	if (boxStyle == 0) { //corner box
		float lineW = width / 4;
		float lineH = height / 4;
		//top left
		ImGui::GetForegroundDrawList()->AddLine(tl, ImVec2(tl.x + lineW, tl.y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(tl, ImVec2(tl.x, tl.y + lineH), color, thickness);
		//top right
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(br.x - lineW, tl.y), ImVec2(br.x, tl.y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(br.x, tl.y), ImVec2(br.x, tl.y + lineH), color, thickness);
		//bottom left
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(tl.x, br.y - lineH), ImVec2(tl.x, br.y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(tl.x, br.y), ImVec2(tl.x + lineW, br.y), color, thickness);
		//bottom right
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(br.x - lineW, br.y), ImVec2(br.x, br.y), color, thickness);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(br.x, br.y - lineH), ImVec2(br.x, br.y), color, thickness);
	}
	else { //full box
		ImGui::GetForegroundDrawList()->AddRect(tl, br, color, 0.0f, 0, thickness);
	}
}

 

void DrawHeadCircle(Vector2 position, float radius, ImColor color, float thickness = 3.0f) {
	ImVec2 center(position.x, position.y - radius / 2);
	//make the head circle much smaller
	ImGui::GetForegroundDrawList()->AddCircle(center, radius / 2, color, 20, thickness);

}


void DrawTraceline(Vector2 start, Vector2 end, ImColor color, float thickness = 1.0f)
{
	ImVec2 p0(start.x, start.y);
	ImVec2 p1(end.x, end.y);
	ImGui::GetForegroundDrawList()->AddLine(p0, p1, color, thickness);
	ImGui::GetForegroundDrawList()->AddCircleFilled(p1, 3.0f, color);
}

void DrawDistanceText(Vector2 position, float distance, ImColor color)
{
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(1) << distance << "m";
	std::string txt = ss.str();
	ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
	ImVec2 tp(position.x - ts.x / 2, position.y + 8);
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, ImVec2(tp.x + 1, tp.y + 1), ImColor(0, 0, 0, 150), txt.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, tp, color, txt.c_str());
}

void DrawHealthBar(Vector2 position, float health, float width = 6.0f, float height = 60.0f)
{
	float pct = std::clamp(health / 100.0f, 0.0f, 1.0f);
	ImVec2 base(position.x - width - 4, position.y - height / 2);
	ImGui::GetForegroundDrawList()->AddRectFilled(base, ImVec2(base.x + width, base.y + height), ImColor(0, 0, 0, 150));
	ImGui::GetForegroundDrawList()->AddRectFilled(base, ImVec2(base.x + width, base.y + height * pct), ImColor(0, 255, 0, 200));
	ImGui::GetForegroundDrawList()->AddRect(base, ImVec2(base.x + width, base.y + height), ImColor(255, 255, 255, 200));
}

//reduce name size 

void DrawNames(Vector2 position, const std::string& name, ImColor color)
{
	// Calculate text size using the explicit font and size
	ImVec2 ts = Globals.headerFont->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, name.c_str());
	ImVec2 tp(position.x + 4, position.y - ts.y / 2 - 20);
	// Use the explicit font and size for drawing
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, ImVec2(tp.x + 1, tp.y + 1), ImColor(0, 0, 0, 150), name.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, tp, color, name.c_str());
}

void DrawItemESP(Vector2 position, const std::string& name, float distance, ImColor color)
{
	float w = 50.0f, h = 50.0f;
	ImVec2 tl(position.x - w / 2, position.y - h / 2);

	// Fixed font size for consistent text rendering
	float fontSize = 16.0f;
	ImVec2 ns = ImGui::CalcTextSize(name.c_str());
	ImVec2 np(position.x - ns.x / 2, position.y - h / 2 - ns.y - 4); // Fixed offset above item

	ImGui::GetForegroundDrawList()->AddText(nullptr, fontSize, np, color, name.c_str());

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(1) << distance << "m";
	std::string dt = ss.str();

	ImVec2 ds = ImGui::CalcTextSize(dt.c_str());
	ImVec2 dp(position.x - ds.x / 2, position.y + h / 2 + 4); // Fixed offset below item

	ImGui::GetForegroundDrawList()->AddText(nullptr, fontSize, dp, color, dt.c_str());
}


//MENU SECTION 

 
inline void DrawAimbotTab() {
	if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnOffset(1, 150);

		// ── Enable aimbot
		ImGui::Text("Enabled"); ImGui::NextColumn();
		static bool aimbotEnabled = false;
		ImGui::Checkbox("##aimbot", &aimbotEnabled);
		HoverTooltip("Toggle the aimbot");
		ImGui::NextColumn();

		// ── FOV
		ImGui::Text("FOV"); ImGui::NextColumn();
		static float aimbotFov = 35.0f;
		ImGui::SliderFloat("##fov", &aimbotFov, 5.0f, 180.0f, "%.0f");
		HoverTooltip("Maximum angle from screen centre");
		ImGui::NextColumn();

		// ── Smooth
		ImGui::Text("Smooth"); ImGui::NextColumn();
		static float aimbotSmooth = 5.0f;
		ImGui::SliderFloat("##smooth", &aimbotSmooth, 1.0f, 20.0f, "%.1f");
		HoverTooltip("Higher = slower, smoother aim");
		ImGui::NextColumn();

		// ── Keybind
		ImGui::Text("Keybind"); ImGui::NextColumn();
		static int aimbotKey = VK_RBUTTON; // default right mouse button
	 
		HoverTooltip("Press to activate aimbot");
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Spacing();
	}
}


inline void DrawMiscTab() {
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

 
void DrawMainMenu() {
	const char* tabs[] = { "Visuals", "Aimbot", "Misc", "Colors", "Settings" };
	int tabCount = sizeof(tabs) / sizeof(tabs[0]);

	float buttonHeight = 40.0f;
	float totalButtonHeight = tabCount * buttonHeight;
	float availableForButtons = ImGui::GetContentRegionAvail().y - 100; // Leave space for metrics
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
			currentPage = 1; // Navigate to sub-page
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
		sidebarOpen = !sidebarOpen;
	}
	wasDownLastFrame = isDown;

	if (!sidebarOpen) return;

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

	// Logo and header (always shown)
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

	// Title and subtitle
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

	// Main content area
	float metricsHeight = 100.0f;
	float availableHeight = ImGui::GetContentRegionAvail().y;
	float mainContentHeight = availableHeight - metricsHeight - 60; // 60 for back button spacing

	ImGui::BeginChild("Content", ImVec2(0, mainContentHeight), false, ImGuiWindowFlags_NoScrollbar);

	if (Globals.regularFont) ImGui::PushFont(Globals.regularFont);

	// Page navigation logic
	if (currentPage == 0) {
		// Main menu page
		DrawMainMenu();
	}
	else {
		// Sub-pages
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

	// Back button (only show when not on main menu)
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
			currentPage = 0; // Return to main menu
		}

		if (Globals.headerFont) ImGui::PopFont();
		ImGui::PopStyleColor(3);
	}

	// Metrics display
	ImGui::Separator();
	ImGui::Spacing();

	ImVec2 metricsStart = ImGui::GetCursorScreenPos();
	ImVec2 metricsSize = ImVec2(264, 70);

	ImGui::GetWindowDrawList()->AddRectFilled(
		metricsStart,
		ImVec2(metricsStart.x + metricsSize.x, metricsStart.y + metricsSize.y),
		ImColor(Globals.headerColor),  // Use header color instead of hardcoded
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


// --- RENDERING SECTION

void Render::Loop() {
	for (const PlayerRender& player : Globals.renderPlayers) {


		if (player.AliveDeadorKnocked == ECharacterHealthState::ECHS_Dead)
			continue; //skip dead players

 

		float boxHeight = abs(player.headW2S.y - player.bottomW2S.y);
		float boxWidth = boxHeight * 0.5f;

		ImColor colour;
		if (player.Health < 30.0f) {
			colour = ImColor(255, 100, 100); // Light red for low health
		}
		else {
			colour = ImColor(100, 255, 100); // Light green for healthy
		}

		// Draw box

		if(Globals.ESPEnabled)
			DrawBox(Vector2(player.bottomW2S.x, (player.headW2S.y + player.bottomW2S.y) / 2), boxWidth, boxHeight, colour, 2.f, Globals.BoxStyle);

		// Draw health bar
	//	Vector2 healthBarPos(player.bottomW2S.x, player.bottomW2S.y - 12);
		//DrawHealthBar(healthBarPos, player.Health);

		// draw names 
		DrawNames(Vector2(player.headW2S.x, player.headW2S.y - 12), player.Name, colour);

		// Draw head circle
		DrawHeadCircle(player.headW2S, boxWidth / 2, colour, 2.f);

		//draw distance text
		DrawDistanceText(Vector2(player.bottomW2S.x, player.bottomW2S.y - 12), player.distance, ImColor(255, 255, 255));

		// Draw traceline
		//DrawTraceline(Vector2(Globals.screenWidth / 2, Globals.screenHeight), Vector2(player.bottomW2S.x, player.bottomW2S.y), colour, 1.f);

		ImGui::GetForegroundDrawList()->AddCircle(ImVec2(Globals.screenWidth / 2, Globals.screenHeight / 2), Globals.settings.fov, ImColor(255, 255, 255), 50, 1.f);
		 
	}

	DrawSidebar();
	//for now color will be white
	for (const ItemRenderer& item : Globals.renderItems) {
		if(Globals.itemsEnabled)
		DrawItemESP(item.W2S, item.Name, item.distance, ImColor(255, 255, 255));
		 
	}
	 
}

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

std::vector<unsigned char> LoadResourceData(int resourceId)
{
	// 1️⃣  Find the resource – numeric type (RT_RCDATA) and *no* language filter
	//     (generic FindResource automatically selects the right language).
	HRSRC hRes = FindResource(
		nullptr,                 // current module (the EXE)
		MAKEINTRESOURCE(resourceId), // the numeric ID defined in resource.h
		RT_RCDATA);              // numeric type = 10 (RCDATA)

	if (!hRes) {
		std::cout << "FindResource failed for ID " << resourceId
			<< ", GetLastError = " << GetLastError() << "\n";
		return {};
	}

	// 2️⃣  Load the resource into memory.
	HGLOBAL hGlob = LoadResource(nullptr, hRes);
	if (!hGlob) return {};

	DWORD size = SizeofResource(nullptr, hRes);
	const void* p = LockResource(hGlob);
	if (!p || size == 0) return {};

	// 3️⃣  Copy the raw bytes into a std::vector (our own copy).
	const unsigned char* bytes = static_cast<const unsigned char*>(p);
	return std::vector<unsigned char>(bytes, bytes + size);
}


ImFont* LoadFontFromResource(int resourceId, float fontSize, ImGuiIO& io) {
	std::vector<unsigned char> fontData = LoadResourceData(resourceId);
	if (fontData.empty()) return nullptr;

	// ImGui takes ownership of the data, so we need to allocate it
	void* fontMemory = IM_ALLOC(fontData.size());
	memcpy(fontMemory, fontData.data(), fontData.size());

	ImFontConfig config;
	config.FontDataOwnedByAtlas = true; // ImGui will free the memory

	return io.Fonts->AddFontFromMemoryTTF(fontMemory, fontData.size(), fontSize, &config);
}


bool LoadTextureFromFile(const char* file_name, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	FILE* f = fopen(file_name, "rb");
	if (f == NULL)
		return false;

	fseek(f, 0, SEEK_END);
	size_t file_size = (size_t)ftell(f);
	if (file_size == -1) {
		fclose(f);
		return false;
	}

	fseek(f, 0, SEEK_SET);
	void* file_data = IM_ALLOC(file_size);
	fread(file_data, 1, file_size, f);
	fclose(f);

	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory((const unsigned char*)file_data, (int)file_size, &image_width, &image_height, NULL, 4);
	IM_FREE(file_data);

	if (image_data == NULL)
		return false;

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;

	HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
	if (FAILED(hr)) {
		stbi_image_free(image_data);
		return false;
	}

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return SUCCEEDED(hr);
}
bool LoadTextureFromResource(int resourceId, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
	std::vector<unsigned char> imageData = LoadResourceData(resourceId);
	if (imageData.empty()) return false;

	// Load from memory using stbi
	int image_width = 0;
	int image_height = 0;
	unsigned char* decoded_data = stbi_load_from_memory(imageData.data(), imageData.size(), &image_width, &image_height, NULL, 4);

	if (decoded_data == NULL) return false;

	// Create texture (same as your existing code)
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = decoded_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;

	HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
	if (FAILED(hr)) {
		stbi_image_free(decoded_data);
		return false;
	}

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(decoded_data);

	return SUCCEEDED(hr);
}
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void Render::Init() {
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Overlay", nullptr };
	::RegisterClassExW(&wc);

	Globals.screenWidth = GetSystemMetrics(SM_CXSCREEN);
	Globals.screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Overlay",
		WS_POPUP | WS_EX_TOOLWINDOW, 0, 0, Globals.screenWidth, Globals.screenHeight,
		nullptr, nullptr, wc.hInstance, nullptr);
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~WS_CAPTION; // Remove title bar
	style &= ~WS_THICKFRAME; // Remove borders
	SetWindowLong(hwnd, GWL_STYLE, style);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Load custom fonts with enhanced sizes for better visibility
	Globals.logoFont = LoadFontFromResource(IDR_FONT1, 32.0f, io); // Increased from 28.0f
	Globals.headerFont = LoadFontFromResource(IDR_FONT2, 22.0f, io); // Increased from 16.0f
	Globals.regularFont = LoadFontFromResource(IDR_FONT3, 19.0f, io); // Increased from 14.0f

	// Fallback to default if loading fails
	if (!Globals.logoFont) Globals.logoFont = io.Fonts->AddFontDefault();
	if (!Globals.headerFont) Globals.headerFont = io.Fonts->AddFontDefault();
	if (!Globals.regularFont) Globals.regularFont = io.Fonts->AddFontDefault();

	// Load logo texture (place image in "images" folder)
	ID3D11ShaderResourceView* my_texture = NULL;
	int my_image_width = 0;
	int my_image_height = 0;
	bool ret = LoadTextureFromResource(IDR_LOGO_PNG, &my_texture, &my_image_width, &my_image_height);


	if (ret) {
		Globals.logoTexture = (ImTextureID)my_texture;
		Globals.logoWidth = my_image_width;
		Globals.logoHeight = my_image_height;

	}
	else {
		std::cout << "Failed to load logo texture!" << std::endl;
		Globals.logoTexture = 0;  // Use 0 instead of nullptr
		Globals.logoWidth = 0;
		Globals.logoHeight = 0;
	}

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 0.f);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Po and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		Render::Loop();

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		//HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
		HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}