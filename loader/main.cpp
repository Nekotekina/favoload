#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

int main()
{
	int       argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

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

	if (!exe)
	{
		::MessageBoxW(0, L"No supported executable found.", L"<empty cmd line>", MB_ICONERROR);
		::ExitProcess(1);
	}

	::STARTUPINFOW start;
	::RtlSecureZeroMemory(&start, sizeof(start));
	start.cb = sizeof(start);
	::PROCESS_INFORMATION out;

	if (!::CreateProcessW(exe, argc >= 4 ? argv[3] : nullptr, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, nullptr, &start, &out))
	{
		::MessageBoxW(0, L"Failed to create a process.", exe, MB_ICONERROR);
		::ExitProcess(1);
	}

	LPVOID dll_name = ::VirtualAllocEx(out.hProcess, NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!dll_name || !::WriteProcessMemory(out.hProcess, dll_name, dll, ::lstrlenW(dll) + sizeof(wchar_t), nullptr))
	{
		::MessageBoxW(0, L"Failed to write process memory.", exe, MB_ICONERROR);
		::ExitProcess(1);
	}

	auto dll_entry = reinterpret_cast<::PTHREAD_START_ROUTINE>(&LoadLibrary);

	HANDLE rmt = ::CreateRemoteThread(out.hProcess, nullptr, 0, dll_entry, dll_name, 0, nullptr);

	if (rmt == INVALID_HANDLE_VALUE)
	{
		::MessageBoxW(0, L"Failed to create remote thread.", exe, MB_ICONERROR);
		::ExitProcess(1);
	}

	// Wait for remote thread
	::WaitForSingleObject(rmt, INFINITE);

	// Cleanup
	::CloseHandle(rmt);
	::VirtualFreeEx(out.hProcess, dll_name, 0, MEM_RELEASE);

	// Finally launch the process
	::ResumeThread(out.hThread);
	::ExitProcess(0);
}
