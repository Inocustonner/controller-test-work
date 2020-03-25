/* Lib with methods for dll injections */
#pragma once
/* #include <string_view> */
#include <string>
#include <vector>
#include <array>
#include <optional>
#define NOMINMAX
#include <windows.h>
#include <type_traits>
// #include <cstdio>
#define report_errw(msg, ...) fprintf(stderr, "[dllInj] " msg "GetLastError : %d", __VA_ARGS__, GetLastError())
#define report_err(msg, ...) fprintf(stderr, "[dllInj] " msg, __VA_ARGS__)
#define report(msg, ...) printf("[dllInj] " msg, __VA_ARGS__)

std::optional<std::vector<HMODULE>> GetProcessModules(const HANDLE h_proc, DWORD modules_cnt = -1);
DWORD GetVirtualAddressBase(const HANDLE h_proc);
HMODULE GetModuleByBaseName(const HANDLE h_proc, const char *base_name, const size_t len); /* returns the first module with the base_name */
HMODULE GetExeModule(const HANDLE h_proc);
HMODULE GetExeModule();			// for the current process

std::string GetModulePath(HMODULE h_mod);
std::string GetCurrentProcPath();

bool PlaceCodeCave(const HANDLE h_proc, DWORD cave_addr, DWORD hole_RVA, DWORD nop_cnt);
bool CreateConsole();

/* template code */
bool WriteBytesArr(const HANDLE h_proc, DWORD write_addr_RVA, const BYTE *bytes, size_t len);

template<typename... B>
bool WriteBytes(const HANDLE h_proc, DWORD write_addr, B... bytes)
{
	std::array<int, sizeof...(bytes)> arr_bytes_int{ bytes... };
	std::array<BYTE, sizeof...(bytes)> arr_bytes = {};
	for (int i = 0; i < sizeof...(bytes); ++i)
		arr_bytes[i] = (BYTE)arr_bytes_int[i];
	
	return WriteBytesArr(h_proc, write_addr, arr_bytes.data(), arr_bytes.size());
}
