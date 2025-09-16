#include <windows.h>
#include <stdio.h>
#include "gpudrv_ioctl.h"

int main() {
    HANDLE hDevice;
    DWORD bytesReturned;
    BOOL success;

    // Open the GPU driver device (from our Windows WDF kernel driver)
    hDevice = CreateFileA(
        "\\\\.\\gpudrv",               // Device name (matches WDF symbolic link)
        GENERIC_READ | GENERIC_WRITE,  // Access rights
        0,                             // No sharing
        NULL,                          // Default security
        OPEN_EXISTING,                 // Must already exist
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Error: Could not open device (error code %lu)\n", GetLastError());
        return 1;
    }

    // -----------------------------
    // 1. Submit a workload
    // -----------------------------
    struct gpudrv_submit submit;
    submit.size = 1024;
    submit.id   = 42;

    success = DeviceIoControl(
        hDevice,
        IOCTL_GPUDRV_SUBMIT,   // From gpudrv_ioctl.h
        &submit, sizeof(submit),
        NULL, 0,
        &bytesReturned,
        NULL
    );

    if (!success) {
        printf("IOCTL_SUBMIT failed (error %lu)\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }

    printf("Submitted workload with id=%lu, size=%lu\n", submit.id, submit.size);

    // -----------------------------
    // 2. Query status
    // -----------------------------
    struct gpudrv_status status;
    status.id = submit.id;

    success = DeviceIoControl(
        hDevice,
        IOCTL_GPUDRV_STATUS,   // From gpudrv_ioctl.h
        &status, sizeof(status),
        &status, sizeof(status),
        &bytesReturned,
        NULL
    );

    if (!success) {
        printf("IOCTL_STATUS failed (error %lu)\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }

    printf("Status for workload id=%lu: completed=%d\n", status.id, status.completed);

    CloseHandle(hDevice);
    return 0;
}