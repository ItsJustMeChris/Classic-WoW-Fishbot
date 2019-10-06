#include "Mem.h"
#include <thread>

Mem m;

float Deg2Rad = 3.14159265359 / 180.0;

struct Vector3
{
	float x;
	float y;
	float z;
};

struct Vector2
{
	float x;
	float y;
};

struct CGameObject
{
	byte type;
	std::string name;
	Vector3 pos;
};

struct Matrix
{
	float _00; //a
	float _01; //b
	float _02; //c
	float _10; //d
	float _11; //e
	float _12; //f
	float _20; //g
	float _21; //h
	float _22; //i
};

struct CCamera {
	char pad[0x10];
	Vector3 camPos;//0x10-0x18
	Matrix mat;//0x1C-0x3C
	float fov;//0x40
};

Matrix Inverse(Matrix in) {
	return Matrix{ in._11 * in._22 - in._12 * in._21,
					in._02 * in._21 - in._01 * in._22,
					in._01 * in._12 - in._02 * in._11,
					in._12 * in._20 - in._10 * in._22,
					in._00 * in._22 - in._02 * in._20,
					in._02 * in._10 - in._00 * in._12,
					in._10 * in._21 - in._11 * in._20,
					in._01 * in._20 - in._00 * in._21,
					in._00 * in._11 - in._01 * in._10,
	};
}

Vector2 W2S(Vector3 pos)
{
	auto pCameraBase = m.read<uintptr_t>(m.m_base + 0x2535398);
	auto pCamera = m.read<uintptr_t>(pCameraBase + 0x3330);
	auto cam = m.read<CCamera>(pCamera);
	RECT rc = { 0,0,1920,1080 };
	Vector3 difference{ pos.x - cam.camPos.x, pos.y - cam.camPos.y, pos.z - cam.camPos.z };

	float product =
		difference.x * cam.mat._00 +
		difference.y * cam.mat._01 +
		difference.z * cam.mat._02;

	if (product < 0)
		return Vector2{ 0,0 };

	Matrix inverse = Inverse(cam.mat);
	Vector3 fView{
		inverse._00 * difference.x + inverse._10 * difference.y + inverse._20 * difference.z ,
		inverse._01 * difference.x + inverse._11 * difference.y + inverse._21 * difference.z,
		inverse._02 * difference.x + inverse._12 * difference.y + inverse._22 * difference.z
	};
	Vector3 camera{ -fView.y, -fView.z, fView.x };
	Vector2 gameScreen{ (rc.right - rc.left) / 2.0f , (rc.bottom - rc.top) / 2.0f };
	Vector2 aspect{ gameScreen.x / tan(((cam.fov * 55.0f) / 2.0f) * Deg2Rad) ,gameScreen.y / tan(((cam.fov * 35.0f) / 2.0f) * Deg2Rad) };
	Vector2 screenPos{ gameScreen.x + camera.x * aspect.x / camera.z,gameScreen.y + camera.y * aspect.y / camera.z };

	if (screenPos.x < 0 || screenPos.y < 0 || screenPos.x > rc.right || screenPos.y > rc.bottom)
		return Vector2{ 0,0 };

	return screenPos;
}

int right_click(Vector2 screen)
{
	SetCursorPos(screen.x, screen.y);
	INPUT   Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	::SendInput(1, &Input, sizeof(INPUT));
	::ZeroMemory(&Input, sizeof(INPUT));
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	::SendInput(1, &Input, sizeof(INPUT));
	return 1;
}

int send_key(int i) {
	INPUT ip;
	ip.ki.wVk = i;
	ip.ki.dwFlags = 0; 
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wVk = i;
	ip.ki.dwFlags = 0;
	SendInput(1, &ip, sizeof(INPUT));
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wVk = i;
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
	return 1;
}

void fish() {
	bool state = false;
	while (1)
	{
		if (GetKeyState(0x77) & 0x8000)
		{
			break;
		}
		if (GetKeyState(0x76) & 0x8000)
		{
			state = !state;
			printf("Bot State: %d\n", state);
			Sleep(100);
		}
		if (state)
		{
			Sleep(1);
			uintptr_t NEXT_OBJECT = 0x70;
			auto pObjMgr = m.read<uintptr_t>(m.m_base + 0x231CED8);
			auto pNextObj = m.read<uintptr_t>(pObjMgr + 0x18);
			do
			{
				Sleep(1);
				CGameObject obj;
				obj.type = m.read<byte>(pNextObj + 0x20);
				if (obj.type == 8)
				{
					obj.pos.x = m.read<float>(pNextObj + 0x1B0);
					obj.pos.y = m.read<float>(pNextObj + 0x1B4);
					obj.pos.z = m.read<float>(pNextObj + 0x1B8);
					auto pUnitNameList = m.read<uintptr_t>(pNextObj + 0x478);
					auto pUnitName = m.read<uintptr_t>(pUnitNameList + 0xE0);
					obj.name = m.readstring(pUnitName);
					if (obj.name == "Fishing Bobber") {
						BYTE bobber = m.read<BYTE>(pNextObj + 0x14C);
						if (bobber == 1) {
							auto screenPos = W2S(obj.pos);
							printf("Fish Found, Click@ [%f,%f]\n", screenPos.x, screenPos.y);
							right_click(screenPos);
							send_key(0x31);
							Sleep(1500);
						}
					}
				}
				pNextObj = m.read<uintptr_t>(pNextObj + NEXT_OBJECT);
			} while ((pNextObj & 1) == 0 && pNextObj != 0);
		}
	}
}

int main() {
	m.GetProcessID(L"Wow.exe");
	std::cout << "Executable found, Pid: " << m.m_pid << std::endl;
	if (!m.Open(PROCESS_ALL_ACCESS))
	{
		std::cout << "Failed to open process." << std::endl;
		return 0;
	}
	std::cout << "Opened process handle" << std::endl;
	std::cout << "Searching for base address" << std::endl;
	std::cout << "Base: 0x" << std::hex << m.GetModBase(L"Wow.exe") << std::endl;
	std::cout << "====================" << std::endl;
	std::cout << "Starting Fish Thread" << std::endl;
	std::cout << "Ensure Fishing Skill is on Key [1]" << std::endl;
	std::cout << "Hotkeys:" << std::endl;
	std::cout << "F7: Toggle pause" << std::endl;
	std::cout << "F8: Close" << std::endl;
	std::thread fishThread(fish);
	fishThread.join();
	std::cout << "Thread Closed" << std::endl;
	return 1;
}