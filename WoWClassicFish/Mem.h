#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <string>

class Mem {
public:
	uintptr_t m_pid;
	uintptr_t mod;
	HANDLE m_hProcess;
	uintptr_t m_base;

	inline bool IsProcessHandleValid(HANDLE h) { return h > 0 && h != INVALID_HANDLE_VALUE; }

	bool Open(DWORD flags);
	uintptr_t GetModBase(const wchar_t* modName);
	uintptr_t GetProcessID(const wchar_t* ExeName);
	std::string readstring(uintptr_t address);
	template<class C>
	C read(DWORD_PTR(Address));
};

template<class C>
inline C Mem::read(DWORD_PTR(Address))
{
	C c;
	ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(Address), &c, sizeof(c), nullptr);
	return c;
}
