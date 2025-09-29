#pragma once
#define NOMINMAX
#include <limits>
#include "Cache/Game.h"
#include "Cache/ItemESP.h"
#include <string>
#include <Utils/singleton.h>
#include <shared_mutex>
#include <vector>
#include <Utils/Utils.h>
#include <ImGUI/imgui.h>
#include "../DMALibrary/Memory/Memory.h"
#include <iostream>
inline namespace FarlightDMA {
    class Globals : public Singleton<Globals> {
    public:
        int screenWidth = 2560;
        int screenHeight = 1440;

        // Throttle overlay to reduce GPU/CPU usage when overlay is active.
        // Set to 0 to disable throttling.
        int overlayFPSLimit = 60;

        
        float maxDistance = 300.0f;
        float MaxItemDistance = 300.0f;

       
        bool ESPEnabled = true;
		bool DrawHeadCircle = true;
		bool DrawBones = true;
		bool DrawHP = false;
		bool DrawNames = true;
		bool DrawTraceline = false;
        bool itemsEnabled = true;
        bool DrawItemName = true;
        bool DrawItemBox = true;
        bool HeadEnabled = true;
        bool DistanceEnabled = true;
        bool refreshcheat = false;

     
       //aimbot 
        bool EnableAimbot = true;
        
        int BoxStyle = 1;   
        float LineThickness = 1.5f;   

       
        std::unordered_set<EItemCategory> enabledItemCategories;
        std::unordered_set<EWeaponType> enabledWeaponTypes;

       //MENU
        int OvrlyMode = 0 ;
         HWND overlayHWND = nullptr;
         bool sidebarOpen = true;
        //
        ImVec4 ColorPlayerBox = ImVec4(0.1f, 1.0f, 0.1f, 1.0f); // green
        ImVec4 ColorBones = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
        ImVec4 ColorLines = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
        ImVec4 ColorName = ImVec4(1.0f, 1.0f, 0.6f, 1.0f); // yellowish
        ImVec4 ColorDistance = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // grey
        ImVec4 ColorHead = ImVec4(0.6f, 0.6f, 1.0f, 1.0f); // bluish
        ImVec4 ColorHP = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // red
        ImVec4 ColorItems = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
        ImVec4 ColorWeapons = ImVec4(1.0f, 0.5f, 0.2f, 1.0f); // orange
        ImVec4 ColorFOV = ImVec4(1.0f, 1.0f, 1.0f, 0.8f); // white (slightly transparent)

        // --- Menu Style ---
        ImVec4 menuAccentColor = ImVec4(0.8f, 0.3f, 0.0f, 1.0f);
        ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
        ImVec4 headerColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

        // --- Process / Memory ---
        std::string processName = "SolarlandClient-Win64-Shipping.exe";
        HANDLE scatterHandle;
        int OpenMenuKey = VK_INSERT;

        // --- Render Data ---
        std::vector<PlayerRender> renderPlayers;
        std::vector<ItemRenderer> renderItems;
        std::mutex playerMutex;
        std::mutex itemMutex;

        // --- Misc ---
         int overlayMode = 1;
          int target_monitor = -1;
         int monitor_enum_state = 0;
        int readFPS = 0;
        AimType aimType;
        ETargetPriority TargetPriority = ETargetPriority::Head;

        // --- Fonts & Assets ---
        ImFont* logoFont = nullptr;
        ImFont* headerFont = nullptr;
        ImFont* regularFont = nullptr;

        ImTextureID logoTexture = 0;
        int logoWidth = 1280;
        int logoHeight = 608;
		struct Offsets {
			uintptr_t uworld = 0x9F319B0;
			uintptr_t persistentlevel = 0x30;
            uintptr_t AActors = 0x98;
			uintptr_t gameInstance = 0x1f0;
			uintptr_t playersArray = 0x38; //localplayers
			uintptr_t playerController = 0x30;
			uintptr_t playerCameraManager = 0x388;
			uintptr_t playerCameraCache = 0x2bc0;
			uintptr_t playerCameraPOV = 0x10;
			uintptr_t localPawn = 0x370;
			uintptr_t gameState = 0x198;
			uintptr_t playerArray = 0x310;
			uintptr_t pawnPrivate = 0x340;
            uintptr_t PlayerName = 0x488;
			uintptr_t marvelCharacter = 0x0;
			uintptr_t defaultFov = 0x0;
			uintptr_t ASolarTeamInfo = 0xf50;
			uintptr_t teamState = 0x2f0;
			uintptr_t CharacterHealthState = 0x1f21;
            uintptr_t Actor_State = 0x2f8;
			uintptr_t AbilitySystemComponent = 0x650;
            uintptr_t FSolarItemData = 0x360; //ASolarItemActor
			uintptr_t mesh = 0x338;  /*// Inheritance: APawn > AActor > UObjectnamespace ACharacter {*/
			uintptr_t lastSubmitTime = 0x0;
			uintptr_t lastRenderTime = 0x0;
			uintptr_t componentToWorld = 0x270;
			uintptr_t boneArray = 0x6B0;   //-> UskinnedMeshComponent
			uintptr_t GNAMES = 0x9DBA1C0;
			uintptr_t GOBJECTS;
		} offsets;

		struct Settings {
			float smoothing = 2.25;
			float fov = 90.0f;
            bool IgnoreKnocked = true;
            bool TriggerbotEnabled = false;
            int TriggerDelayMS;
			float AimbotMaxDistance = 300.0f;
		} settings;

		struct Aimbot {
			float closestDistance = FLT_MAX;
			Vector2 closestTarget = Vector2(0.0f, 0.0f);
			uintptr_t targetMesh = 0;
		} aimbot;
	};
#define Globals FarlightDMA::Globals::Get()
}

inline std::string GetNameFromFName(int id) {
    auto chunk = (uint32_t)((int)(id) >> 16);
    auto name = (uint16_t)id;
    auto poolChunk = mem.Read<uint64_t>(mem.baseAddress + Globals.offsets.GNAMES + ((chunk + 2) * 8));
    auto entryOffset = poolChunk + (uint32_t)(2 * name);
    auto nameEntry = mem.Read<int16_t>(entryOffset);
    auto nameLength = nameEntry >> 6;
    char buff[1028];
    if ((uint32_t)nameLength && nameLength > 0) {
        mem.Read(entryOffset + 2, buff, nameLength);
        buff[nameLength] = '\0';
        return std::string(buff);
    }
    else return "";
}
 
inline void DumpGame() {
    while (true) {
        system("cls"); // Clear screen for better menu experience

        std::cout << "=============================================" << std::endl;
        std::cout << "        Farlight DMA By Makimura" << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << std::endl;
        std::cout << "  [1] Dump Game Memory" << std::endl;
        std::cout << "  [2] Continue to Cheat" << std::endl;
        std::cout << "  [3] Exit Program" << std::endl;
        std::cout << std::endl;
        std::cout << "  Select an option (1-3): ";

        int choice;
        std::cin >> choice;

        // Clear input buffer
        std::cin.clear();
        std::cin.ignore(10000, '\n');

        switch (choice) {
        case 1: {
            system("cls");
            std::cout << "=============================================" << std::endl;
            std::cout << "           Memory Dump Utility" << std::endl;
            std::cout << "=============================================" << std::endl;
            std::cout << std::endl;
            std::cout << "Enter dump file name (without extension): ";
            std::string dumpName;
            std::getline(std::cin, dumpName);

            if (dumpName.empty()) {
                std::cout << "Invalid file name. Operation cancelled." << std::endl;
                system("pause");
                break;
            }

             
            if (dumpName.length() < 4 ||
                (dumpName.substr(dumpName.length() - 4) != ".exe" &&
                    dumpName.substr(dumpName.length() - 4) != ".EXE")) {
                dumpName += ".exe";
            }

            std::cout << std::endl;
            std::cout << "Dumping memory to '" << dumpName << ".dmp'..." << std::endl;

            auto processBase = mem.GetBaseDaddy(Globals.processName);
            if (!processBase) {
                std::cout << "Error: Failed to get process base address!" << std::endl;
                system("pause");
                break;
            }

            if (mem.DumpMemory(processBase, dumpName)) {
                std::cout << "Success: Memory dumped to '" << dumpName << ".dmp'" << std::endl;
            }
            else {
                std::cout << "Error: Failed to dump memory!" << std::endl;
            }

            std::cout << std::endl;
            system("pause");
            break;
        }

        case 2: {
            system("cls");
            std::cout << "Continuing..." << std::endl;
            
            return; 
        }

        case 3: {
            std::cout << "Exiting program..." << std::endl;
            exit(0);
        }

        default: {
            std::cout << "Invalid choice! Please select 1, 2, or 3." << std::endl;
            system("pause");
            break;
        }
        }
    }
}
