#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void redirect(LPCSTR dll, LPCSTR func, LPVOID ptr)
{
	const auto data = reinterpret_cast<BYTE*>(::GetProcAddress(::GetModuleHandleA(dll), func));
	const auto target = reinterpret_cast<BYTE*>(ptr) - data - 5;

	DWORD old_protect;
	::VirtualProtect(data, 2 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect);

	if (target > 0x7fffffff || target < -0x80000000ll)
	{
		data[0] = 0x48; // MOV rax, imm64
		data[1] = 0xb8;
		::CopyMemory(data + 2, &ptr, sizeof(ptr));
		data[10] = 0xff; // JMP rax
		data[11] = 0xe0;
	}
	else
	{
		data[0] = 0xe9; // JMP rel32
		::CopyMemory(data + 1, &target, 4);
	}

	::VirtualProtect(data, 2 * sizeof(void*), old_protect, NULL);
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
		redirect("KERNEL32.dll", "lstrcmpiA", lstrcmpiA_);
		break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
