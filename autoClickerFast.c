#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdbool.h>

#pragma comment(lib, "winmm.lib")

#define HOTKEY_TOGGLE   1
#define HOTKEY_MODE     2
#define TARGET_CPS      480

typedef struct {
    bool clicking;
    bool maxMode;
} ClickData;

DWORD WINAPI ClickThread(LPVOID lpParam) {
    ClickData *data = (ClickData*)lpParam;

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

    LARGE_INTEGER freq, lastPrint, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastPrint);

    unsigned long long count = 0;

    while (data->clicking) {
        SendInput(1, &input, sizeof(INPUT));
        count++;

        if (data->maxMode) {
            Sleep(0);
        } else {
            Sleep(1000 / TARGET_CPS);
        }

        QueryPerformanceCounter(&now);
        if ((now.QuadPart - lastPrint.QuadPart) >= freq.QuadPart) {
            double elapsed = (double)(now.QuadPart - lastPrint.QuadPart) / freq.QuadPart;
            double cps = count / elapsed;
            printf("CPS: %.1f (%s mode)\n", cps, data->maxMode ? "MAX" : "CAPPED");
            fflush(stdout);
            count = 0;
            lastPrint = now;
        }
    }
    return 0;
}

int main() {
    ClickData data = {0};
    HANDLE hThread = NULL;

    timeBeginPeriod(1);

    if (!RegisterHotKey(NULL, HOTKEY_TOGGLE, 0, VK_F6)) {
        MessageBox(NULL, "Failed to register F6 hotkey", "Error", MB_ICONERROR);
        return 1;
    }
    if (!RegisterHotKey(NULL, HOTKEY_MODE, 0, VK_F7)) {
        MessageBox(NULL, "Failed to register F7 hotkey", "Error", MB_ICONERROR);
        return 1;
    }

    printf("Press F6 to toggle autoclicker on/off\n");
    printf("Press F7 to switch between MAX and CAPPED mode\n");

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_TOGGLE) {
                data.clicking = !data.clicking;
                if (data.clicking) {
                    MessageBeep(MB_ICONASTERISK);
                    printf("Started clicking (%s mode)...\n", data.maxMode ? "MAX" : "CAPPED");
                    hThread = CreateThread(NULL, 0, ClickThread, &data, 0, NULL);
                } else {
                    MessageBeep(MB_ICONHAND);
                    printf("Stopped clicking.\n");
                    if (hThread) {
                        WaitForSingleObject(hThread, INFINITE);
                        CloseHandle(hThread);
                        hThread = NULL;
                    }
                }
            } else if (msg.wParam == HOTKEY_MODE) {
                data.maxMode = !data.maxMode;
                printf("Switched to %s mode.\n", data.maxMode ? "MAX" : "CAPPED");
            }
        }
    }

    timeEndPeriod(1);
    UnregisterHotKey(NULL, HOTKEY_TOGGLE);
    UnregisterHotKey(NULL, HOTKEY_MODE);
    return 0;
}
