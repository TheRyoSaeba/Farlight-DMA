#pragma warning (disable : 4995)

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


void DrawSidebar();

//DRAWING SECTION 
//TODO Move Drawbox from CFMANAGER
//draw box decide color  and thickness based on teamid


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
void DrawTraceline(Vector2 start, Vector2 end, ImColor color, float thickness = 1.0f) {
	ImGui::GetForegroundDrawList()->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, end.y + 20), color, thickness);
}
void DrawDistanceText(Vector2 position, float distance, ImColor color)
{
	// Subtract 4 meters from the reported distance, clamp to zero
	float adjustedDistance = distance - 4.0f;
	if (adjustedDistance < 0.0f) adjustedDistance = 0.0f;

	int meters = static_cast<int>(std::lround(adjustedDistance));
	std::string txt = "[" + std::to_string(meters) + "M]";

	ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
	constexpr float verticalOffset = 1.0f;
	ImVec2 tp(position.x - ts.x / 2.0f, position.y + verticalOffset);
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 16.0f, ImVec2(tp.x + 1, tp.y + 1), ImColor(0, 0, 0, 150), txt.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 16.0f, tp, color, txt.c_str());
}
void DrawHealthBar(Vector2 center, float health, float width = 6.0f, float height = 60.0f)
{
	float pct = std::clamp(health / 150.0f, 0.0f, 1.0f);

	ImColor fillColor;
	if (pct > 0.66f) {
		fillColor = ImColor(0, 220, 0, 220);
	}
	else if (pct > 0.33f) {
		fillColor = ImColor(255, 165, 0, 220); 
	}
	else {
		fillColor = ImColor(220, 20, 20, 220);
	}
	ImVec2 tl(center.x - width / 2.0f, center.y - height / 2.0f);
	ImVec2 br(center.x + width / 2.0f, center.y + height / 2.0f);
	ImGui::GetForegroundDrawList()->AddRectFilled(tl, br, ImColor(0, 0, 0, 140));

	float filledTopY = tl.y + height * (1.0f - pct);
	ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(tl.x, filledTopY), ImVec2(br.x, br.y), fillColor);

	ImGui::GetForegroundDrawList()->AddRect(tl, br, ImColor(255, 255, 255, 200));
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(tl.x + 1.0f, tl.y + 1.0f),
		ImVec2(br.x - 1.0f, br.y - 1.0f),
		ImColor(0, 0, 0, 60));
}
void DrawNames(Vector2 position, const std::string& name, ImColor color)
{
	// Calculate text size using the explicit font and size
	ImVec2 ts = Globals.headerFont->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, name.c_str());
	ImVec2 tp(position.x + 4, position.y - ts.y / 2 - 20);
	// Use the explicit font and size for drawing
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, ImVec2(tp.x + 1, tp.y + 1), ImColor(0, 0, 0, 150), name.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, tp, color, name.c_str());
}
void DrawItemName(Vector2 position, const std::string& name, ImVec4 color)
{
	ImU32 col = ImGui::GetColorU32(color);
	ImU32 shadow = ImGui::GetColorU32(ImVec4(0, 0, 0, 0.6f));
	ImVec2 ts = ImGui::CalcTextSize(name.c_str());
	ImVec2 tp(position.x - ts.x / 2, position.y - 30);
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, ImVec2(tp.x + 1, tp.y + 1), shadow, name.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, tp, col, name.c_str());
}

void DrawItemBox(Vector2 position, float width, float height, ImVec4 color, float thickness = 1.0f) {
	ImU32 col = ImGui::GetColorU32(color);
	ImVec2 tl(position.x - width / 2, position.y - height / 2);
	ImVec2 br(position.x + width / 2, position.y + height / 2);
	ImGui::GetForegroundDrawList()->AddRect(tl, br, col, 0.0f, 0, thickness);
}
void DrawItemDistanceText(Vector2 position, float distance, ImColor color)
{
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(1) << distance << "m";
	std::string txt = ss.str();
	ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
	ImVec2 tp(position.x - ts.x / 2, position.y + 8);
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, ImVec2(tp.x + 1, tp.y + 1), ImColor(0, 0, 0, 150), txt.c_str());
	ImGui::GetForegroundDrawList()->AddText(Globals.logoFont, 18.0f, tp, color, txt.c_str());
}


// --- RENDERING SECTION

void Render::Loop() {
		auto* draw = ImGui::GetForegroundDrawList();

		if (Globals.settings.fov > 0) {
			draw->AddCircle(
				ImVec2(Globals.screenWidth / 2, Globals.screenHeight / 2),
				Globals.settings.fov,
				ImColor(Globals.ColorFOV), 
				50,
				Globals.LineThickness
			);
		}
		for (const PlayerRender& player : Globals.renderPlayers) {
			if (player.AliveDeadorKnocked == ECharacterHealthState::ECHS_Dead)
				continue;
			 

			float boxHeight = fabs(player.headW2S.y - player.bottomW2S.y);
			float boxWidth = boxHeight * 0.5f;

                

			if (Globals.ESPEnabled) {
				DrawBox(Vector2(player.bottomW2S.x, (player.headW2S.y + player.bottomW2S.y) / 2),
					boxWidth, boxHeight,
					Globals.ColorPlayerBox,
					Globals.LineThickness,
					Globals.BoxStyle);
			}

			if (Globals.DrawNames)
				DrawNames(Vector2(player.headW2S.x, player.headW2S.y - 12),
					player.Name, Globals.ColorName);

			 

			if (Globals.DrawBones)
			//	DrawSkeleton(player.Skeleton, Globals.ColorBones, Globals.LineThickness);

				if (Globals.DrawHP) {
					float boxCenterY = (player.headW2S.y + player.bottomW2S.y) / 2.0f;
					float boxLeft = player.bottomW2S.x - (boxWidth / 2.0f);
					const float barWidth = 6.0f;
					const float gap = 6.0f; // gap between box and bar
					Vector2 barCenter;
					barCenter.x = boxLeft - gap - (barWidth / 2.0f);
					barCenter.y = boxCenterY;
					DrawHealthBar(barCenter, player.Health, barWidth, boxHeight);
				}

			if (Globals.DrawHeadCircle)
				DrawHeadCircle(player.headW2S, boxWidth / 2, Globals.ColorHead, Globals.LineThickness);

			if (Globals.DrawTraceline) {
				Vector2 start{ static_cast<float>(Globals.screenWidth) * 0.5f, static_cast<float>(Globals.screenHeight) };
				ImColor traceColor = ImColor(Globals.ColorLines);
				DrawTraceline(start, player.bottomW2S, traceColor, Globals.LineThickness);
			}

			if (Globals.maxDistance)
				DrawDistanceText(Vector2(player.bottomW2S.x, player.bottomW2S.y),
					player.distance, Globals.ColorDistance);
		}

		//drawsidebar is in menu.cpp so lets forward declare it
		 
	DrawSidebar();
	for (const ItemRenderer& item : Globals.renderItems) {
		if (Globals.itemsEnabled)
		{
			if (item.distance > Globals.MaxItemDistance)
				continue;
			if (Globals.DrawItemName)
				DrawItemName(item.W2S, item.Name, Globals.ColorItems);

			if (Globals.DrawItemBox)
				DrawItemBox(item.W2S, 50.0f, 20.0f, Globals.ColorItems);

			DrawItemDistanceText(item.W2S, item.distance, ImColor(255, 255, 255));

		}
			 
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
	sd.BufferDesc.RefreshRate.Numerator = 165;
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
		nullptr,                 
		MAKEINTRESOURCE(resourceId),  
		RT_RCDATA);             

	if (!hRes) {
		std::cout << "FindResource failed for ID " << resourceId
			<< ", GetLastError = " << GetLastError() << "\n";
		return {};
	}

	 
	HGLOBAL hGlob = LoadResource(nullptr, hRes);
	if (!hGlob) return {};

	DWORD size = SizeofResource(nullptr, hRes);
	const void* p = LockResource(hGlob);
	if (!p || size == 0) return {};

	 
	const unsigned char* bytes = static_cast<const unsigned char*>(p);
	return std::vector<unsigned char>(bytes, bytes + size);
}


ImFont* LoadFontFromResource(int resourceId, float fontSize, ImGuiIO& io) {
	std::vector<unsigned char> fontData = LoadResourceData(resourceId);
	if (fontData.empty()) return nullptr;

	 
	void* fontMemory = IM_ALLOC(fontData.size());
	memcpy(fontMemory, fontData.data(), fontData.size());

	ImFontConfig config;
	config.FontDataOwnedByAtlas = true;  

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

 
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory((const unsigned char*)file_data, (int)file_size, &image_width, &image_height, NULL, 4);
	IM_FREE(file_data);

	if (image_data == NULL)
		return false;

 
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

	 
	int image_width = 0;
	int image_height = 0;
	unsigned char* decoded_data = stbi_load_from_memory(imageData.data(), imageData.size(), &image_width, &image_height, NULL, 4);

	if (decoded_data == NULL) return false;

	 
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



//no overload
void SetOverlayMode2(HWND hwnd, bool showMenu, int overlayMode)
{
	LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);

	// Always keep TOOLWINDOW to hide from taskbar
	style |= WS_EX_TOOLWINDOW;

	if (overlayMode == 0) {
		// Transparent Mode


		MARGINS margins = { 0, 0, Globals.screenWidth, Globals.screenHeight };
		DwmExtendFrameIntoClientArea(hwnd, &margins);

		if (showMenu) {
			style &= ~WS_EX_LAYERED;
			style &= ~WS_EX_TRANSPARENT;  // Capture mouse for menu interaction
			style &= ~WS_EX_NOACTIVATE;   // Allow keyboard focus

			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE);
		}
		else {
			style |= WS_EX_LAYERED;
			style |= WS_EX_TRANSPARENT;   // Allow clicks to pass through to game
			style |= WS_EX_NOACTIVATE;    // Prevent focus stealing

			// Keep TOPMOST so ESP remains visible over game
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
	else {
		// Fuser Mode
		style &= ~WS_EX_LAYERED;  // No transparency for black background

		MARGINS margins = { 0, 0, 0, 0 };
		DwmExtendFrameIntoClientArea(hwnd, &margins);

		if (showMenu) {
			// When menu is visible:
			style &= ~WS_EX_TRANSPARENT;  // Capture mouse
			style &= ~WS_EX_NOACTIVATE;   // Allow keyboard focus

			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE);
		}
		else {
			// When menu is hidden:
			style |= WS_EX_TRANSPARENT;   // Pass mouse clicks
			style |= WS_EX_NOACTIVATE;    // Prevent focus stealing

			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}

	SetWindowLong(hwnd, GWL_EXSTYLE, style);
}
void SetOverlayMode(HWND hwnd, bool isTransparent, bool menuVisible) {
	auto style = GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW;

	if (isTransparent) {
		// Transparent Mode - DWM boost + original behavior
		MARGINS margins = { -1, -1, -1, -1 };
		DwmExtendFrameIntoClientArea(hwnd, &margins);

		style |= WS_EX_TOPMOST | WS_EX_LAYERED;
		style = menuVisible ? style & ~WS_EX_TRANSPARENT : style | WS_EX_TRANSPARENT;
		SetWindowLong(hwnd, GWL_EXSTYLE, style);
		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);
	}
	else {
		// Fuser Mode - no DWM, different z-order behavior
		MARGINS margins = { 0, 0, 0, 0 };
		DwmExtendFrameIntoClientArea(hwnd, &margins);

		if (menuVisible) {
			style &= ~(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		else {
			style &= ~WS_EX_LAYERED;
			style |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
			SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		SetWindowLong(hwnd, GWL_EXSTYLE, style);
	}
}




void SwitchToMonitor(HWND hwnd, int monitorIndex) {
	DISPLAY_DEVICE dd = { sizeof(dd) };
	if (EnumDisplayDevices(NULL, monitorIndex, &dd, 0)) {
		DEVMODE dm = { sizeof(dm) };
		if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
			SetWindowPos(hwnd, HWND_TOPMOST,
				dm.dmPosition.x, dm.dmPosition.y,
				dm.dmPelsWidth, dm.dmPelsHeight,
				SWP_NOACTIVATE);
		}
	}
}


void DrawSimpleSettings() {
	// Monitor selection
	static int currentMonitor = 0;
	int monitorCount = GetSystemMetrics(SM_CMONITORS);
	if (ImGui::Combo("Monitor", &currentMonitor,
		[](void*, int idx, const char** out) {
			static char buf[32];
			sprintf(buf, "Monitor %d", idx + 1);
			*out = buf;
			return true;
		}, nullptr, monitorCount)) {
		SwitchToMonitor(Globals.overlayHWND, currentMonitor);
	}

	// Overlay mode selection
	static int overlayMode = 0; // 0 = transparent, 1 = black
	const char* modes[] = { "Transparent ESP", "Black Fuser" };
	if (ImGui::Combo("Mode", &overlayMode, modes, 2)) {
		Globals.overlayMode = overlayMode;
		SetOverlayMode2(Globals.overlayHWND, overlayMode == 0, Globals.sidebarOpen);
	}
}


void Render::Init() {
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
		GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Overlay", nullptr };
	RegisterClassExW(&wc);

	Globals.screenWidth = GetSystemMetrics(SM_CXSCREEN);
	Globals.screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		wc.lpszClassName, L"Overlay", WS_POPUP,
		0, 0, Globals.screenWidth, Globals.screenHeight,
		nullptr, nullptr, wc.hInstance, nullptr);

	Globals.overlayHWND = hwnd;

	 
	SetOverlayMode2(hwnd, true, true);

	 
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return;
	}

 
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

 
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	 
	 
	Globals.logoFont = LoadFontFromResource(IDR_FONT1, 32.0f, io); 
	Globals.headerFont = LoadFontFromResource(IDR_FONT2, 22.0f, io); 
	Globals.regularFont = LoadFontFromResource(IDR_FONT3, 19.0f, io);  

 
	if (!Globals.logoFont) Globals.logoFont = io.Fonts->AddFontDefault();
	if (!Globals.headerFont) Globals.headerFont = io.Fonts->AddFontDefault();
	if (!Globals.regularFont) Globals.regularFont = io.Fonts->AddFontDefault();

	 
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

 
	ImGui::StyleColorsDark();
	 

	 
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	bool show_demo_window = true;
	bool show_another_window = false;
	 
 
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

		ImVec4 clear_color;
		if (Globals.overlayMode == 0) {
			// Transparent mode - clear with transparent
			clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		}
		else {
			// Fuser mode - clear with black
			clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		 
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = g_pSwapChain->Present(0, 0);   // Present with vsync
		//HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
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