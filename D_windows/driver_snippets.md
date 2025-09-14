KMDF Driver snippets and build notes:

1) Create a new "KMDF Driver" project in Visual Studio with WDK installed.
2) Add gpudrv_ioctl.h to the project (shared header).
3) In DriverEntry/EvtDeviceAdd, create a device and default IO queue.
4) Implement EvtIoDeviceControl and dispatch IOCTL_GPUDRV_SUBMIT / IOCTL_GPUDRV_COMPLETE.
   Use WdfRequestRetrieveInputBuffer() to obtain the buffer and copy it into driver-owned memory.
5) Use WdfRequestCompleteWithInformation(...) to complete I/O and return bytes if needed.
6) For debugging, enable test signing on Windows and use WinDbg or local kernel debugging.
7) For a full implementation see Microsoft docs for "KMDF Ioctl Example".

This file intentionally contains guidance rather than a full project file. Use the code fragments in the main assistant output as basis for EvtIoDeviceControl handling.