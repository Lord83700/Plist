#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <tlhelp32.h>

DWORD GetBasePriority(DWORD processID)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (NULL != hProcess){
        DWORD Pri = GetPriorityClass(hProcess);
        DWORD basePriority;

        switch (Pri) {
        case IDLE_PRIORITY_CLASS:
            basePriority = 4;
            break;
        case BELOW_NORMAL_PRIORITY_CLASS:
            basePriority = 6;
            break;
        case NORMAL_PRIORITY_CLASS:
            basePriority = 8;
            break;
        case ABOVE_NORMAL_PRIORITY_CLASS:
            basePriority = 10;
            break;
        case HIGH_PRIORITY_CLASS:
            basePriority = 13;
            break;
        case REALTIME_PRIORITY_CLASS:
            basePriority = 24;
            break;
        default:
            basePriority = -1; // Inconnu
            break;
        }

        CloseHandle(hProcess);

        return basePriority;
    }
}

DWORD countThreads(DWORD processId){
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    DWORD countThread = 0;

    if (Thread32First(hThreadSnapshot, &te32)){
        do {
            if (te32.th32OwnerProcessID == processId) {
                countThread++;
            }
        } while (Thread32Next(hThreadSnapshot, &te32));
    }

    CloseHandle(hThreadSnapshot);

    return countThread;
}

void PrintProcessNameAndID()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            DWORD processId = pe32.th32ProcessID;
            DWORD Pri = GetBasePriority(processId);
            DWORD Thd = countThreads(processId);
            _tprintf(TEXT("%s       %d %d %d\n"), pe32.szExeFile, processId,Pri,Thd);

        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
}

int main( void )
{
    _tprintf(TEXT("Name          Pid Pri Thd  Hnd  Priv    CPU Time     Elapsed Time\n"));
    PrintProcessNameAndID();

    return 0;
}