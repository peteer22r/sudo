
#include <iostream>
#include <sstream>
#include <Shlobj.h>

struct LocalFreeDeleter {
	void operator() (LPSTR ptr)
	{
		LocalFree(ptr);
	}
};

std::unique_ptr<CHAR, LocalFreeDeleter> GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();

	std::unique_ptr<CHAR, LocalFreeDeleter> messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	return messageBuffer;
}

std::string join_params(int argc, const char* argv[])
{
	std::stringstream params;

	for (int idx = 1; idx < argc; ++idx)
		params << argv[idx] << ' ';
	return params.str();
}

int WaitForReturnCode(HANDLE hProcess)
{
	DWORD rc = ERROR_SUCCESS;
	::WaitForSingleObject(hProcess, INFINITE);
	::GetExitCodeProcess(hProcess, &rc);
	::CloseHandle(hProcess);
	return rc;
}

int main(int argc, const char* argv[])
{
	size_t env_size = 0;
	char sudo_var[5] = {};
	getenv_s(&env_size, sudo_var, "sudo");
	if (IsUserAnAdmin() == FALSE && strcmp(sudo_var, "sudo") != 0)
	{
		auto params = join_params(argc, argv);

		HANDLE hIn = ::GetStdHandle(STD_INPUT_HANDLE);
		HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE hErr = ::GetStdHandle(STD_ERROR_HANDLE);

		std::stringstream ss;
		ss << GetCurrentProcessId() << ' ' << hIn << ' ' << hOut << ' ' << hErr << ' ' << params;
		auto par = ss.str();

		SHELLEXECUTEINFOA sei = { sizeof(sei), SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE };
		sei.lpVerb = strcmp(sudo_var, "test") != 0 ? "runas" : NULL;
		sei.lpFile = argv[0];
		sei.nShow = SW_HIDE;
		sei.lpParameters = par.c_str();
		_putenv("sudo=sudo");

		if (ShellExecuteExA(&sei) == FALSE)
		{
			std::cerr << GetLastErrorAsString().get();
			return EXIT_FAILURE;
		}
		SetConsoleCtrlHandler(NULL, TRUE);
		if (WAIT_TIMEOUT == WaitForSingleObject(sei.hProcess, 250))
		{
			FreeConsole();
		}
		return WaitForReturnCode(sei.hProcess);
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
		if (CreateProcessA(NULL, &params[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == FALSE)
		{
			params.insert(0, "cmd.exe /C ");
			if (GetLastError() != ERROR_FILE_NOT_FOUND || CreateProcessA(NULL, &params[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) == FALSE)
			{
				auto errorText = GetLastErrorAsString();
				BOOL rc = WriteFile(si.hStdError, errorText.get(), strlen(errorText.get()), NULL, NULL);
				return EXIT_FAILURE;
			}
		}
		FreeConsole();
		if (AttachConsole(pi.dwProcessId) != FALSE || GetLastError() != ERROR_INVALID_HANDLE)
		{
			FreeConsole();
			return WaitForReturnCode(pi.hProcess);
		}
		return EXIT_SUCCESS;
	}
}
