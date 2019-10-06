#include "Mem.h"
bool Mem::Open(DWORD flags) {
	m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, m_pid);
	return IsProcessHandleValid(m_hProcess);
}


uintptr_t Mem::GetProcessID(const wchar_t* ExeName) {
	PROCESSENTRY32W proc;
	proc.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	m_pid = -1;

	if (Process32FirstW(hSnap, &proc)) {
		do {
			if (wcscmp(ExeName, proc.szExeFile) == 0)
			{
				CloseHandle(hSnap);
				m_pid = proc.th32ProcessID;
				return true;
			}
		} while (Process32NextW(hSnap, &proc));
	}

	CloseHandle(hSnap);
	return NULL;
}

std::string Mem::readstring(uintptr_t address)
{
	char* buff = new char[128];
	ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), buff, 128, 0);
	std::string tmp(buff);
	return tmp;
}

uintptr_t Mem::GetModBase(const wchar_t* modName) {
	HANDLE hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_pid);
	MODULEENTRY32 me32;

	size_t size = strlen(me32.szModule) + 1;
	wchar_t* moduleName = new wchar_t[size];

	size_t outSize;
	mbstowcs_s(&outSize, moduleName, size, me32.szModule, size - 1);

	me32.dwSize = sizeof(MODULEENTRY32);
	Module32First(hsnapshot, &me32);
	do
	{
		if (_wcsicmp(moduleName, modName))
		{
			std::cout << "Base: 0x" << std::hex << (uintptr_t)me32.modBaseAddr << std::endl;
			std::cout << "Size: 0x" << std::hex << (uintptr_t)me32.modBaseSize << std::endl;
			m_base = (uintptr_t)me32.modBaseAddr;
			return (uintptr_t)me32.modBaseAddr;
		}
	} while (Module32Next(hsnapshot, &me32));
	CloseHandle(hsnapshot);
	return NULL;
}