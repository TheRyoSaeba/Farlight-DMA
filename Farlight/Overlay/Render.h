#pragma once
 
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "Utils/Globals.h"
#include "Cache/ItemESP.h"
#include <dwmapi.h>
#include <functional>
#include <iostream>

static bool g_dwmCompositionDisabled = false;
static bool g_lastMenuState = false;
static int g_lastOverlayMode = -1;

class Render {
public:
	void Loop();
	void Init();
};

void SetOverlayMode(HWND hwnd, int mode, bool menuVisible);


   
 
