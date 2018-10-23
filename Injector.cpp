#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <chrono>
#include <filesystem>

long long CurrentTimeMs();
bool injectDll (DWORD processId, std::string dllPath);

typedef HINSTANCE(*fpLoadLibrary)(char*);

const std::string Lstr[] = {"[*----]","[-*---]","[--*--]","[---*-]","[----*]"};

int main(int argc, char ** argv) {
	using std::cout;
	using std::endl;
	using std::cin;
	namespace fs = std::filesystem;

	std::string processName = "";
	std::string dllName = "";

	cout << "[64Bit]\n" << endl;

	for (int i = 1; i < argc; i++) {
		if (!(strcmp(argv[i], "-h")) || !(strcmp(argv[i], "--help"))) {
			cout << "--- DllInjector Help ---\n\n  -f or --file       -> dll filename\n  -e or --executable -> process name\n\n------------------------\n\n";
			return 0;
		}
		else if (!(strcmp(argv[i], "-f")) || !(strcmp(argv[i], "--file"))) {
			i++;
			dllName = argv[i];
		}
		else if (!(strcmp(argv[i], "-e")) || !(strcmp(argv[i], "--executable"))) {
			i++;
			processName = argv[i];
		}
	}

	if (!strcmp(processName.c_str(), "")) {
		cout << "Process name : ";
		cin >> processName;
	}

	if (!strcmp(dllName.c_str(), "")) {
		cout << "DLL File Must Be Where Executable Is Located.\nDll File : ";
		cin >> dllName;
	}

	fs::path path = fs::current_path();
	std::string dllPath = path.string() + "\\" + dllName;

	if (!fs::exists(dllPath)) {
		cout << "No dll file found\n";
		return 0;
	}

	DWORD processId = NULL;
	PROCESSENTRY32 processEntry32 = { sizeof(PROCESSENTRY32) };
	HANDLE hGetProc;

	int i = 0;
	int direction = 1;
	long long ms = CurrentTimeMs();
	long long ms2 = CurrentTimeMs();

	while (!processId) {
		cout << '\r' << "Searching for Process \"" << processName << "\" " << Lstr[i];
		i += direction;
		if (i >= sizeof(Lstr) / sizeof(Lstr[i]) - 1 || i <= 0) {
			direction = -direction;
		}
		hGetProc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (Process32First(hGetProc, &processEntry32)) {
			do {
				if (!strcmp(processEntry32.szExeFile, processName.c_str())) {
					processId = processEntry32.th32ProcessID;
					break;
				}
			} while (Process32Next(hGetProc, &processEntry32));
		}
		Sleep(500);
	}

	cout << "\nFound Process \"" << processName << "\"!\n";
	while (!injectDll(processId, dllPath)) {
		i += direction;
		if (i >= sizeof(Lstr) / sizeof(Lstr[i]) - 1 || i <= 0) {
			direction = -direction;
		}
		cout << "Injecting DLL... \"" << dllPath << "\"\n" << Lstr[i] << " [Error " << GetLastError() << "]";
		Sleep(500);
	}
	cout << "DLL injected succesfully!\n\nClosing injector.\n\n";
	CloseHandle(hGetProc);
	Sleep(4000);
}

bool injectDll(DWORD processID,std::string dllPath) {
	HANDLE hProc;
	LPVOID parameterAddress;
	HINSTANCE hDll = LoadLibrary("kernel32");

	fpLoadLibrary LoadLibraryAddr = (fpLoadLibrary)GetProcAddress(hDll, "LoadLibraryA");
	
	hProc = OpenProcess(PROCESS_ALL_ACCESS, false, processID);

	parameterAddress = VirtualAllocEx(hProc, 0, strlen(dllPath.c_str()) + 1, MEM_COMMIT, PAGE_READWRITE);
	bool memoryWritten = WriteProcessMemory(hProc, parameterAddress, dllPath.c_str(), strlen(dllPath.c_str()) + 1, NULL);

	if (!CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryAddr, parameterAddress, 0, 0)) {
		memoryWritten = false;
	}

	CloseHandle(hProc);

	return memoryWritten;
}

long long CurrentTimeMs() {
	using namespace std::chrono;
	return duration_cast<milliseconds> (system_clock::now().time_since_epoch()).count();
}