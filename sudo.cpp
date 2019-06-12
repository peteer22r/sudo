
#include <iostream>
#include <sstream>
#include <Shlobj.h>

std::string join_params(int argc, const char* argv[])
{
	std::stringstream params;

	for (int idx = 1; idx < argc; ++idx)
		params << argv[idx] << ' ';
	return params.str();
}

int main(int argc, const char* argv[])
{
	if (IsUserAnAdmin() == FALSE)
	{
		auto params = join_params(argc, argv);

		HANDLE hIn = ::GetStdHandle(STD_INPUT_HANDLE);
		HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE hErr = ::GetStdHandle(STD_ERROR_HANDLE);

		std::stringstream ss;
		ss << GetCurrentProcessId() << ' ' << hIn << ' ' << hOut << ' ' << hErr << ' ' << params;
		auto par = ss.str();

		SHELLEXECUTEINFOA sei = {};
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
		sei.cbSize = sizeof(sei);
		sei.lpVerb = "runas";
		sei.lpFile = argv[0];
		sei.nShow = SW_HIDE;
		sei.lpParameters = par.c_str();
		ShellExecuteExA(&sei);
		SetConsoleCtrlHandler(NULL, TRUE);
		if (WAIT_TIMEOUT == WaitForSingleObject(sei.hProcess, 250))
		{
			FreeConsole();
			WaitForSingleObject(sei.hProcess, INFINITE);
		}
		DWORD dwRc = 0;
		GetExitCodeProcess(sei.hProcess, &dwRc);
		CloseHandle(sei.hProcess);
		return dwRc;
	}
	else
	{
		auto params = join_params(argc - 4, argv + 4);
		FreeConsole();
		AttachConsole(ATTACH_PARENT_PROCESS);
		PROCESS_INFORMATION pi = {};
		STARTUPINFOA si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		HANDLE hParent = OpenProcess(PROCESS_ALL_ACCESS, TRUE, strtoul(argv[1], nullptr, 10));
		DuplicateHandle(hParent, (HANDLE)strtoull(argv[2], nullptr, 16), GetCurrentProcess(), &si.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
		DuplicateHandle(hParent, (HANDLE)strtoull(argv[3], nullptr, 16), GetCurrentProcess(), &si.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
		DuplicateHandle(hParent, (HANDLE)strtoull(argv[4], nullptr, 16), GetCurrentProcess(), &si.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
		CreateProcessA(NULL, &params[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		FreeConsole();
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD dwRc = 0;
		GetExitCodeProcess(pi.hProcess, &dwRc);
		CloseHandle(pi.hProcess);
		return dwRc;
	}
}
