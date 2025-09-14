// gpudrv_win_client.c
#include <windows.h>
#include <stdio.h>
#include "gpudrv_ioctl.h"

int main() {
    HANDLE h = CreateFileW(L"\\\\.\\gpudrv", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed %lu\n", GetLastError());
        return 1;
    }

    // Example: send small buffer (headerless for simplicity). In your Windows user app, mirror Linux protocol if desired.
    char buf[16] = {0};
    DWORD out = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_GPUDRV_SUBMIT, buf, sizeof(buf), NULL, 0, &out, NULL);
    if (!ok) { printf("DeviceIoControl failed %lu\n", GetLastError()); }
    CloseHandle(h);
    return 0;
}