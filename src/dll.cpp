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
		// push bool is_stable
		add eax, 0Eh
		xor ecx, ecx
		mov cl, byte ptr[eax]	// mov ecx, [char isnt_stable]
		mov eax, 031h
		sub eax, ecx
		push eax

		sub esp, 08h

		mov eax, 0023FC10h
		add eax, base_ima
		fld tbyte ptr[eax]
		fstp qword ptr[esp]
		call fix

		add esp, 08h
		pop eax
	
		// save result
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

		cmp word ptr[eax + 0Eh], 30h

		ret
	}
}


void insert_caves(const HANDLE h_proc)
{

	base_ima = (DWORD)GetExeModule(h_proc);
	PlaceCodeCave(h_proc, (DWORD)cave1, 0x0021C68E); // cmp word ptr [eax+0Eh], 30h
}


EXPORT(void, init)()
{
	HANDLE h_proc = GetCurrentProcess();
	if (init_fixer("ini.ini"))
	{
		insert_caves(h_proc);		
	}
	else
	{
		printf("Failed to init\n");
	}
}
