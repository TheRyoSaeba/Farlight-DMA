#include <iostream>
#include "../DMALibrary/Memory/Memory.h"
#include "Utils/Globals.h"
#include <Cache/Game.h>
#include <thread>
#include <Overlay/Render.h>
#include <Aimbot/Aimbot.h>


void refreshMemory()
{
	while (true)
	{
		mem.MemoryPartialRefresh();
		mem.TLBRPartialefresh();

		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}
}

 

int main()
{

	 
 
init:
	if (!mem.Init(Globals.processName),true,false)
	{
		std::cout << "Failed to initialize DMA" << std::endl;
		system("pause");
		return 1;
	}

	//DumpGame();
	std::cout << "DMA initialized" << std::endl;




	if (!mem.GetKeyboard()->InitKeyboard())
	{
		std::cout << "Failed to initialize keyboard hotkeys through kernel." << std::endl;
		system("pause");
		return 1;
	}

	 
	 

	if (!Aimbot::init())
	{
		std::cout << "Failed to initialize kmbox" << std::endl;
		 
	}
	


	if (!Get_Uworld())
	{
		std::cout << "Failed to get UWorld" << std::endl;
		system("pause");
		return 1;

	}

	mem.FullRefresh();

	std::thread(refreshMemory).detach();


	std::cout << "\n Enjoy ! Refresh Cheat in Settings for any issues" << std::endl;

	std::thread(Game::Loop).detach();


	Render render = Render();

	render.Init();

	render.Loop();

	return 0;

}
