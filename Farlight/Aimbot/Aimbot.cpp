#include <fstream>
#include <string>
#include <wtypes.h>
#include <Utils/Globals.h>
#include <Kmbox/KmboxNet.h>
#include <Kmbox/kmbox_interface.hpp>
#include <DMALibrary/Memory/Memory.h>
#include <Aimbot/Aimbot.h>

bool Aimbot::init() {
	std::ifstream ipFile("kmboxnet/ip.txt");
	if (ipFile.peek() != std::ifstream::traits_type::eof()) {
		if (!ipFile.is_open()) {
			return false;
		}

		std::ifstream portFile("kmboxnet/port.txt");

		if (!portFile.is_open()) {
			return false;
		}

		std::ifstream uuidFile("kmboxnet/uuid.txt");

		if (!uuidFile.is_open()) {
			return false;
		}

		std::string uuid;
		std::string ip;
		std::string port;

		if (!std::getline(uuidFile, uuid)) return false;
		if (!std::getline(ipFile, ip)) return false;
		if (!std::getline(portFile, port)) return false;

		WORD portW = static_cast<WORD>(std::stoi(port));

		int resultNet = InitDevice(ip, portW, uuid);
		Globals.aimType = NET;
	}
	else {
		std::string port = find_port("USB-SERIAL CH340");
		if (port.empty()) {
			port = find_port("USB-Enhanced-SERIAL CH343");
		}
		if (!port.empty()) {

			if (!open_port(hSerial, port.c_str(), CBR_115200)) {
				return false;
			}
			else {
				Globals.aimType = BPRO;
			}
		}
		else {
			return false;
		}
	}
	return true;
}

void aimAt(Vector2 target) {
	switch (Globals.aimType) {
	case BPRO: {
		std::string command = "km.move(" + std::to_string(target.x) + "," + std::to_string(target.y) + ")\r\n";
		
		send_command(hSerial, command.c_str());
		break;
	}
	case NET:
		kmNet_mouse_move(target.x, target.y);
		break;
	case MEMORY:
		// TODO: Implement memory aimbot
		break;
	}

}

void Aimbot::aimbot(Vector2 target) {
	if (!mem.GetKeyboard()->IsKeyDown(0x02)) return;

	float x = target.x;
	float y = target.y;

	Vector2 ScreenCenter = { (float)Globals.screenWidth / 2 , (float)Globals.screenHeight / 2 };
	Vector2 Target;

	if (x != 0)
	{
		if (x > ScreenCenter.x)
		{
			Target.x = -(ScreenCenter.x - x);
			if (Target.x + ScreenCenter.x > ScreenCenter.x * 2) {
				Target.x = 0;
			}
		}

		if (x < ScreenCenter.x)
		{
			Target.x = x - ScreenCenter.x;
			if (Target.x + ScreenCenter.x < 0) {
				Target.x = 0;
			}
		}
	}
	if (y != 0)
	{
		if (y > ScreenCenter.y)
		{
			Target.y = -(ScreenCenter.y - y);
			if (Target.y + ScreenCenter.y > ScreenCenter.y * 2) {
				Target.y = 0;
			}
		}

		if (y < ScreenCenter.y)
		{
			Target.y = y - ScreenCenter.y;
			if (Target.y + ScreenCenter.y < 0) {
				Target.y = 0;
			}
		}
	}

	Vector2 Angles(Target.x / Globals.settings.smoothing, Target.y / Globals.settings.smoothing);
	aimAt(Angles);

}