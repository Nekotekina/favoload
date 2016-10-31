#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void redirect(LPCSTR dll, LPCSTR func, DWORD target)
{
	BYTE* proc = (BYTE*)::GetProcAddress(::GetModuleHandleA(dll), func);
	target -= DWORD(proc) + 5;
	DWORD old_protect;
	::VirtualProtect(proc, 8, PAGE_EXECUTE_READWRITE, &old_protect);
	proc[0] = 0xe9;
	::CopyMemory(proc + 1, &target, 4);
	::VirtualProtect(proc, 8, old_protect, NULL);
}

static int WINAPI lstrcmpiA_(LPCSTR s1, LPCSTR s2)
{
	return ::CompareStringA(0x411, NORM_IGNORECASE, s1, -1, s2, -1) - 2;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
	{
		redirect("KERNEL32.dll", "lstrcmpiA", (DWORD)&lstrcmpiA_);
		break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
