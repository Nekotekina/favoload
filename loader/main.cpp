#include <windows.h>

int main(int argc, wchar_t* argv[])
{
	const wchar_t* exe = argc >= 2 ? argv[1] : nullptr;
	const wchar_t* dll = argc >= 3 ? argv[2] : L"payload.dll";

	if (!exe)
	{
		static constexpr const wchar_t* exe_list[]
		{
			L"WizAnniversary.exe",
			L"WA_funta.exe",
			L"Memoria.exe",
			L"Hoshimemo_EH.exe",
			L"World.exe",
			L"Hikari.exe",
			L"WhiteEternity.exe",
			L"Shinku.exe",
		};

		for (const wchar_t* entry : exe_list)
		{
			if (::GetFileAttributesW(entry) != INVALID_FILE_ATTRIBUTES)
			{
				exe = entry;
				break;
			}
		}
	}

	wchar_t buf[256]{};

	if (!exe)
	{
		::MessageBoxW(0, L"No supported executable found.", L"<empty cmd line>", MB_ICONWARNING);

		wchar_t dir[256]{};
		::GetCurrentDirectoryW(sizeof(dir), dir);

		::OPENFILENAMEW ofn{};
		ofn.lStructSize     = sizeof(ofn);
		ofn.lpstrFilter     = L"*.exe\0*.exe\0";
		ofn.lpstrFile       = buf;
		ofn.nMaxFile        = sizeof(buf);
		ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrTitle      = L"Select executable file";
		ofn.lpstrInitialDir = dir;

		if (!::GetOpenFileNameW(&ofn))
		{
			return 0;
		}

		exe = buf;
	}

	::STARTUPINFOW start{};
	start.cb = sizeof(start);
	::PROCESS_INFORMATION out{};

	if (!::CreateProcessW(exe, argc >= 4 ? argv[3] : nullptr, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, nullptr, &start, &out))
	{
		::MessageBoxW(0, L"Failed to create a process.", exe, MB_ICONERROR);
		return 1;
	}

	auto dll_name = ::VirtualAllocEx(out.hProcess, NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!dll_name || !::WriteProcessMemory(out.hProcess, dll_name, dll, ::lstrlenW(dll) + sizeof(wchar_t), nullptr))
	{
		::MessageBoxW(0, L"Failed to write process memory.", exe, MB_ICONERROR);
		return 1;
	}

	auto thread = ::CreateRemoteThread(out.hProcess, nullptr, 0, reinterpret_cast<::PTHREAD_START_ROUTINE>(&LoadLibrary), dll_name, 0, nullptr);

	if (thread == INVALID_HANDLE_VALUE)
	{
		::MessageBoxW(0, L"Failed to create remote thread.", exe, MB_ICONERROR);
		return 1;
	}

	// Wait for remote thread
	::WaitForSingleObject(thread, INFINITE);

	// Cleanup
	::CloseHandle(thread);
	::VirtualFreeEx(out.hProcess, dll_name, 0, MEM_RELEASE);

	// Finally launch the process
	::ResumeThread(out.hThread);
	return 0;
}

void startup()
{
	auto argc = 0;
	auto argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	::ExitProcess(::main(argc, argv));
}

#pragma function(memset)
void*__cdecl memset(void* dst, int val, size_t size)
{
	auto ptr = static_cast<char*>(dst);
	while (size--) *ptr++ = static_cast<char>(val);
	return dst;
}
