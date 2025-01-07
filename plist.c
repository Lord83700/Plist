#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <tlhelp32.h>

SIZE_T GetMemoryVirt(DWORD processID){
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    PROCESS_MEMORY_COUNTERS pmc;
    
    if (NULL != hProcess){
        if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))){
            CloseHandle(hProcess);
            return pmc.PagefileUsage;
        }
        else {
            return 0;
        }
        
    }
}

void PrintProcessNameAndID()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            DWORD processId = pe32.th32ProcessID;
            DWORD Pri = pe32.pcPriClassBase;
            DWORD Thd = pe32.cntThreads;
            SIZE_T MemVirt = GetMemoryVirt(processId);
            _tprintf(TEXT("Processus : %s\n    PID : %d\n    MemVirt : %zu octets\n    Priority : %d\n    Thread: %d\n"), pe32.szExeFile, processId, MemVirt, Pri, Thd);

        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
}

int main( void )
{
    //_tprintf(TEXT("Name                        Pid Pri Thd  Hnd  Priv    CPU Time     Elapsed Time\n"));
    PrintProcessNameAndID();

    return 0;
}