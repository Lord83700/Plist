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

void PrintThreadInfo(DWORD processID) {
    HANDLE hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hsnapshot == INVALID_HANDLE_VALUE){
        _tprintf(TEXT("Unable to create thread snapshot.\n"));
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if(Thread32First(hsnapshot, &te32)){
        do{
            if(te32.th32OwnerProcessID==processID){
                DWORD threadID = te32.th32ThreadID;

                HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,FALSE,threadID);

                DWORD lastError = 0;
                CONTEXT context = {0};
                context.ContextFlags = CONTEXT_FULL;

                if(hThread !=NULL ){

                    LPVOID entryPoint = NULL;
                    lastError = GetLastError();

                    _tprintf(TEXT("   Thread ID:  %lu\n"), threadID);
                    _tprintf(TEXT("   Entry Point: .%lu\n"), entryPoint);
                    _tprintf(TEXT("   State%lu\n"));
                    _tprintf(TEXT("   Last Error: %lu\n"), lastError);


                    CloseHandle(hThread);
                }else{
                    _tprintf(TEXT("   Thread ne s'ouvre pas %lu.\n"), threadID);
                }
            }
        }while(Thread32Next(hsnapshot, &te32));

    }

    CloseHandle (hsnapshot);




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
            PrintThreadInfo(processId);

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
