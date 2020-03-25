#include "dllInj.h"
#include <ios>					// for create console
#include <cstdio>				// for create console
#include <string.h>
#include <psapi.h>


std::optional<std::vector<HMODULE>> GetProcessModules(const HANDLE h_proc, DWORD modules_cnt)
{
	// Get modules count
	DWORD bytes_needed;
	if (modules_cnt == -1)
	{
		EnumProcessModules(h_proc, nullptr, 0, &bytes_needed);
		modules_cnt = bytes_needed / sizeof(HMODULE);		
	}
	
	std::vector<HMODULE> modules(modules_cnt);

	if (EnumProcessModules(h_proc, modules.data(), modules_cnt * sizeof(HMODULE), &bytes_needed))
	{
		return modules;
	}
	else
	{
		report_errw("Failed to enum modules in \"GetProcessModules\"\n");
		return {};
	}
}


DWORD GetVirtualAddressBase(const HANDLE h_proc)
{
	auto wrpd_module = GetProcessModules(h_proc, 1);
	if (wrpd_module)
	{
		return (DWORD)wrpd_module.value()[0]; // cast HMODULE to DWORD
	}
	return 0;
}


HMODULE GetModuleByBaseName(const HANDLE h_proc, const char *base_name, const size_t len) /* returns the first module with the base_name */
{
	auto wrpd_modules = GetProcessModules(h_proc);
	
	if (wrpd_modules)
	{
		std::vector<HMODULE> &modules = wrpd_modules.value();
		
		for (const HMODULE module : modules)
		{
			const char mod_name[1024] = {};
			GetModuleBaseNameA(h_proc, module, (LPSTR)mod_name, len);
			if (strncmp(base_name, (const char*)mod_name, len) == 0)
			{
				return module;
			}
		}
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}


HMODULE GetExeModule(const HANDLE h_proc)
{
	auto wrpd_module = GetProcessModules(h_proc, 1);
	if (wrpd_module)
	{
		return wrpd_module.value()[0];
	}
	return (HMODULE)-1;
}


HMODULE GetExeModule()
{
	return GetExeModule(GetCurrentProcess());
}


std::string GetModulePath(const HMODULE h_mod)
{
	std::string path;
	path.resize(MAX_PATH);
	/*	
		todo: take into an account that GetModuleFileNameA also returns length
		and check whether MAX_PATH is enough		
	*/
	if (GetModuleFileNameA(h_mod, path.data(), path.size()))
	{
		return path;
	}
	else
	{
		path.resize(0);
		return path;
	}
}


std::string GetCurrentProcPath()
{
	return GetModulePath(GetExeModule());
}


bool write_at(const HANDLE h_proc, void *addr, const void *src, size_t len)
{
	return WriteProcessMemory(h_proc, addr, src, len, NULL);
}


bool PlaceCodeCave(const HANDLE h_proc, DWORD cave_addr, DWORD hole_RVA, DWORD nop_cnt = 0)
{
	DWORD vba;	// virual base address
	auto wrpd_module = GetProcessModules(h_proc, 1);
	if (wrpd_module)
	{
		vba = (DWORD)wrpd_module.value()[0]; // cast HMODULE to DWORD
	}
	else
	{
		return false;	
	}
	constexpr DWORD call_op_size = 1 + sizeof(DWORD);
	std::vector<BYTE> call_op_bytes(call_op_size + nop_cnt);
	call_op_bytes[0] = 0xE8;
	
	// { call_op  rel32
	//		0xE8, 0x00, 0x00, 0x00, 0x00
	// };
		
	DWORD call_rel32 = cave_addr - (hole_RVA + vba) - call_op_size;	

	*(DWORD*)(call_op_bytes.data() + 1) = call_rel32;
	if (nop_cnt)
	{
		std::fill(begin(call_op_bytes) + call_op_size, end(call_op_bytes), 0x90);	
	}
	
	if (write_at(h_proc, (void*)(hole_RVA + vba), call_op_bytes.data(), call_op_bytes.size()))
	{
		return true;
	}
	else
	{
		report_errw("Failed to write process memory at %Xh\n", hole_RVA);
		return false;
	}
}


bool CreateConsole()
{
	if (AllocConsole())
	{
		FILE *conin, *conout; 
		freopen_s(&conin, "conin$", "r", stdin);
		freopen_s(&conout, "conout$", "w", stdout);
		freopen_s(&conout, "conout$", "w", stderr);
	
		std::ios::sync_with_stdio();
		
		return true;
	}
	else
	{
		report_errw("Failed to AllocConsole\n");
		
		return false;
	}
}


bool WriteBytesArr(const HANDLE h_proc, DWORD write_addr_RVA, const BYTE *bytes, size_t len)
{
	DWORD va_addr = write_addr_RVA + GetVirtualAddressBase(h_proc);
	if (!write_at(h_proc, (void*)(va_addr), bytes, len))
	{
		report_errw("Failed to write bytes at %X\n", va_addr);
		return false;
	}
	return true;
}
