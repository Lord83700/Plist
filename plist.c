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
    return 0;
}

void GetThreadTimeAndState(HANDLE hThread) {
    FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;

    if (GetThreadTimes(hThread, &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime)){
        if (lpExitTime.dwLowDateTime == 0 && lpExitTime.dwHighDateTime == 0){
            _tprintf(TEXT("       State : Le Thread est actif\n"));
        }
        else {
            _tprintf(TEXT("       State : Le Thread est inactif\n"));
        }
    }
}   

void PrintThreadInfo(DWORD processID) {
    HANDLE hsnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hsnapshot == INVALID_HANDLE_VALUE){
        _tprintf(TEXT("Impossible de créer une Snapshot\n"));
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if(Thread32First(hsnapshot, &te32)){
        do{
            if(te32.th32OwnerProcessID==processID){
                DWORD threadID = te32.th32ThreadID;

                HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,FALSE,threadID);
                if (hThread == NULL){
                    DWORD errorCode = GetLastError();
                    _tprintf(TEXT("    Impossible d'ouvrir les Thread de %lu.\n        Error Code : %lu\n"), processID, errorCode);
                    CloseHandle (hsnapshot);
                    return;
                }

                DWORD lastError = 0;
                CONTEXT context = {0};
                context.ContextFlags = CONTEXT_FULL;

                if(hThread !=NULL){

                    LPVOID entryPoint = NULL; // A compléter
                    lastError = GetLastError();

                    _tprintf(TEXT("    Thread ID:  %lu\n"), threadID);
                    _tprintf(TEXT("       Entry Point:\n"));
                    GetThreadTimeAndState(hThread);
                    _tprintf(TEXT("       Last Error: %lu\n"), lastError);


                    CloseHandle(hThread);
                }
            }
        }while(Thread32Next(hsnapshot, &te32));

    }

    CloseHandle (hsnapshot);
}

void formatTime(FILETIME ft, char *buffer, size_t bufferSize) {
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

   
    unsigned long long totalMilliseconds = uli.QuadPart / 10000;
    unsigned int hours = totalMilliseconds / (1000 * 60 * 60);
    unsigned int minutes = (totalMilliseconds / (1000 * 60)) % 60;
    unsigned int seconds = (totalMilliseconds / 1000) % 60;
    unsigned int milliseconds = totalMilliseconds % 1000;

    snprintf(buffer, bufferSize, "%02u:%02u:%02u.%03u", hours, minutes, seconds, milliseconds);
}


void Plist()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    const char *headers[] = {"Name", "PID", "Pri", "Thd", "Priv", "CPU TIME", "Elapsed Time"};

    printf("%-50s %-10s %-10s %-10s %-10s %-10s %-10s\n", headers[0], headers[1], headers[2], headers[3], headers[4], headers[5], headers[6]);
    printf("---------------------------------------------------------------------------------------------------------------------------\n");

    if (Process32First(hSnapshot, &pe32)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);

            if (hProcess){
                DWORD processId = pe32.th32ProcessID;
                DWORD Pri = pe32.pcPriClassBase;
                DWORD Thd = pe32.cntThreads;
                SIZE_T MemVirt = GetMemoryVirt(processId);

                FILETIME creationTime, exitTime, kernelTime, userTime;
                char cpuTimeBuffer[20] = "N/A";
                char elapsedTimeBuffer[20] = "N/A";

                if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                    
                    ULARGE_INTEGER user, kernel;
                    user.LowPart = userTime.dwLowDateTime;
                    user.HighPart = userTime.dwHighDateTime;
                    kernel.LowPart = kernelTime.dwLowDateTime;
                    kernel.HighPart = kernelTime.dwHighDateTime;

                    FILETIME totalCpuTime;
                    totalCpuTime.dwLowDateTime = user.LowPart + kernel.LowPart;
                    totalCpuTime.dwHighDateTime = user.HighPart + kernel.HighPart;
                    formatTime(totalCpuTime, cpuTimeBuffer, sizeof(cpuTimeBuffer));

                    
                    SYSTEMTIME nowSystemTime;
                    FILETIME nowFileTime;
                    GetSystemTime(&nowSystemTime);
                    SystemTimeToFileTime(&nowSystemTime, &nowFileTime);

                    ULARGE_INTEGER now, creation;
                    now.LowPart = nowFileTime.dwLowDateTime;
                    now.HighPart = nowFileTime.dwHighDateTime;
                    creation.LowPart = creationTime.dwLowDateTime;
                    creation.HighPart = creationTime.dwHighDateTime;

                    if (now.QuadPart > creation.QuadPart) {
                        ULARGE_INTEGER elapsed;
                        elapsed.QuadPart = now.QuadPart - creation.QuadPart;
                        FILETIME elapsedTime;
                        elapsedTime.dwLowDateTime = elapsed.LowPart;
                        elapsedTime.dwHighDateTime = elapsed.HighPart;
                        formatTime(elapsedTime, elapsedTimeBuffer, sizeof(elapsedTimeBuffer));
                    }
                }

                printf("%-50s %-10d %-10d %-10d %-10lu %-10s  %-10s\n",
                    pe32.szExeFile,
                    processId,
                    Pri,
                    Thd,
                    MemVirt,  //  dwSize placeholder pour la private memory
                    cpuTimeBuffer,
                    elapsedTimeBuffer);
                //PrintThreadInfo(processId);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
}

int main( void )
{
    
    Plist();

    return 0;
}
