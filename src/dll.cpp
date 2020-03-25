#include "dll.h"
#include "fixer.h"
#include <dllInjLib/dllInj.h>
#include <windows.h>

#define CAVE void __declspec(naked)

DWORD base_ima;


CAVE cave1()
{
	__asm
	{
		pushad
		pushfd
	}
	__asm
	{
		sub esp, 08h
		fstp qword ptr[esp]
		call fix
		add esp, 08h
		fld st(0)
		fstp tbyte ptr[ebp - 020h]
		
		mov eax, 0023FC10h
		add eax, base_ima
		fstp tbyte ptr[eax]
		wait
	}
	
	// epilog 
	__asm
	{
		popfd
		popad
		ret
	}
}


void insert_caves(const HANDLE h_proc)
{

	base_ima = (DWORD)GetExeModule(h_proc);
	PlaceCodeCave(h_proc, (DWORD)cave1, 0x0021C64B, 0x1C); // 0x1A
}


EXPORT(void, init)()
{
	HANDLE h_proc = GetCurrentProcess();
	if (CreateConsole() && init_fixer("adj.ini"))
	{
		insert_caves(h_proc);		
	}
	else
	{
		printf("Failed to init\n");
	}
}
