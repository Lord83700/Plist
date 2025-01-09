#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <time.h>


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

    printf("%-50s %-10s %-10s %-10s %-10s %-20s %-20s\n", headers[0], headers[1], headers[2], headers[3], headers[4], headers[5], headers[6]);
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");

    if (Process32First(hSnapshot, &pe32)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);

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

            printf("%-50s %-10d %-10d %-10d %-10lu %-20s  %-20s\n",
                pe32.szExeFile,
                processId,
                Pri,
                Thd,
                MemVirt,  //  dwSize placeholder pour la private memory
                cpuTimeBuffer,
                elapsedTimeBuffer);
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
}

// Approximation de l'état du thread
const char* GetThreadStateApprox(HANDLE hThread) {
    FILETIME ftCreation, ftExit, ftKernel, ftUser;

    if (GetThreadTimes(hThread, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        // If no cpu time, assume it's idle (oui c'est pas parfait mais bon)
        if (ftKernel.dwLowDateTime == 0 && ftKernel.dwHighDateTime == 0 &&
            ftUser.dwLowDateTime == 0 && ftUser.dwHighDateTime == 0) {
            return "Idle/Waiting";
        } else {
            return "Running/Active";
        }
    }

    return "Unknown";
}

void PrintThreadDetails(DWORD processID) {
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 te32;
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    SYSTEMTIME stUser, stKernel;
    HANDLE hThread;

    te32.dwSize = sizeof(THREADENTRY32);

    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        printf(" thread snapshot fail.\n");
        return;
    }

    printf("Threads for process %d:\n", processID);
    printf("Tid    Pri  State           User Time    Kernel Time   Elapsed Time\n");

    if (Thread32First(hThreadSnap, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processID) {
                hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);

                const char* state = "Unknown";

                if (hThread) {
                    state = GetThreadStateApprox(hThread);

                    if (GetThreadTimes(hThread, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                        // Convert time SYSTEMTIME 
                        FileTimeToSystemTime(&ftUser, &stUser);
                        FileTimeToSystemTime(&ftKernel, &stKernel);

                        // Calcul temp 
                        ULARGE_INTEGER liCreation, liNow;
                        GetSystemTimeAsFileTime((FILETIME*)&liNow);
                        liCreation.LowPart = ftCreation.dwLowDateTime;
                        liCreation.HighPart = ftCreation.dwHighDateTime;
                        ULONGLONG elapsedTime = (liNow.QuadPart - liCreation.QuadPart) / 10000; // mili secs

                       
                        printf("%5d  %3d  %-15s %02d:%02d:%02d.%03d   %02d:%02d:%02d.%03d   %llu ms\n",
                            te32.th32ThreadID,                   // Thread ID
                            te32.tpBasePri,                     // Thread priority
                            state,                              // Thread state
                            stUser.wHour, stUser.wMinute, stUser.wSecond, stUser.wMilliseconds, // User time
                            stKernel.wHour, stKernel.wMinute, stKernel.wSecond, stKernel.wMilliseconds, // Kernel time
                            elapsedTime);                       // Elapsed time
                    } else {
                        printf("%5d  %3d  %-15s  times inaccessible\n",
                               te32.th32ThreadID, te32.tpBasePri, state);
                    }

                    CloseHandle(hThread);
                } else {
                    printf("%5d  %3d  thread inaccessible\n",
                           te32.th32ThreadID, te32.tpBasePri);
                }
            }
        } while (Thread32Next(hThreadSnap, &te32));
    } else {
        printf("No threads found.\n");
    }

    CloseHandle(hThreadSnap);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // List processes
        printf("Liste des processes:\n");
        Plist();
    } else if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        // "-d" + ProcessID:  thread details for process
        DWORD processID = atoi(argv[2]);
        if (processID <= 0) {
            printf(" ProcessID invalide: %s\n", argv[2]);
            return 1;
        }

        printf("detail du thread pour le process ID %d:\n", processID);
        PrintThreadDetails(processID);
    } else {
        // Invalid = man
        printf("Usage:\n");
        printf("  pslist.exe              - Liste les process\n");
        printf("  pslist.exe -d <ProcessID> - Détail du thread pour le process\n");
        return 1;
    }

    return 0;
}

