# Cross-Platform GPU Compute Driver Testing & Benchmark Framework

**Author**: Abhinav (GitHub: Abhinavcodez)  
**Date**: 2025  
**Status**: Prototype / Proof-of-Concept  

---

## 🚀 Overview

This repository implements a **cross-platform framework** combining:

- A **Linux kernel module** (`/dev/gpudrv`) that queues buffers for GPU processing  
- A **user-space daemon + client** to submit tasks and retrieve results  
- A **shared OpenCL compute engine** (`libgpufw`) for vector-add workloads (extendable)  
- A **Perl automation harness** to run experiments across sizes, log results, and parse CSV  
- Windows-side skeleton (KMDF driver + Windows client) showcasing how to mirror the same IOCTL interface on Windows  

The goal is to **benchmark and validate GPU offload via drivers** in a way that spans Linux & Windows, showing you have skills in kernel programming, GPU compute, cross-platform systems, and automation.

---

## 📁 Repository Structure

```

.
├── A_libgpufw/               # OpenCL compute engine and test app
│   ├── kernels/              # `.cl` kernel source files
│   └── src/                   # C source and headers for libgpufw & test
│
├── B_gpudrv/                 # Linux driver + user components
│   ├── kern/                  # Kernel module code (gpudrv kernel module)
│   └── user/                  # Daemon, client user programs to interface with kernel
│
├── C_perl_harness/           # Perl scripts to run benchmarks and parse results
│
└── D_windows/                # Windows skeleton driver snippets & Windows client

````

---

## ✅ Features & Current Capabilities

- **Linux kernel module (`B_gpudrv/kern`)**  
  - Character device `/dev/gpudrv`  
  - `write()` for submission (header + payload)  
  - `read()` to either return queued tasks (daemon) or results (client)  
  - Simple FIFO queue management (submit & result queues)  

- **User-side daemon + client (`B_gpudrv/user`)**  
  - Client: constructs a buffer (two float arrays) and submits it  
  - Daemon: listens for submissions, performs compute via `libgpufw`, writes results  
  - Uses robust read/write loops to handle partial reads/writes  

- **OpenCL compute engine (`A_libgpufw`)**  
  - Initializes OpenCL platform, device, command queue  
  - Reads kernel source (e.g. `vecadd.cl`)  
  - Executes vector-add kernel and reports kernel execution time  

- **Perl automation harness (`C_perl_harness`)**  
  - `run_bench.pl`: loops over sizes, runs client, logs elapsed time + kernel time  
  - `parse_results.pl`: simple CSV parser to inspect results  

- **Windows skeleton (`D_windows`)**  
  - Windows `gpudrv_ioctl.h` with IOCTL macros using `CTL_CODE`  
  - Example Windows client that opens `\\.\gpudrv` and sends `IOCTL_GPUDRV_SUBMIT` / `IOCTL_GPUDRV_STATUS`  
  - Notes and snippets to integrate with a KMDF driver project  

---

## 📦 Prerequisites & Dependencies

- Linux (Ubuntu or equivalent) with kernel headers matching your running kernel  
- `gcc`, `make`, `clang` if needed  
- OpenCL development libraries (ICD / vendor OpenCL) and `clinfo`  
- `perl` with modules: `Getopt::Long`, `Time::HiRes`, `Text::CSV`  
- For Windows parts: Visual Studio + WDK (KMDF), ability to sign/test drivers (test-signing mode)  
- MinGW (for cross-compiling Windows client) if you want to build from Linux  

---

## 🔧 Build & Run Instructions

### A. OpenCL Compute Engine (libgpufw)

```bash
cd A_libgpufw
make
./test_vecadd 1048576
````

You should see kernel time printed and correct partial results.

---

### B. Linux Driver + Daemon + Client

1. Build kernel module:

   ```bash
   cd B_gpudrv/kern
   make
   sudo insmod gpudrv.ko
   dmesg | tail
   ls -l /dev/gpudrv
   ```

2. Build user programs:

   ```bash
   cd ../user
   make
   ```

3. Start daemon (may need sudo):

   ```bash
   sudo ./gpudrv_daemon &
   ```

4. Run client:

   ```bash
   ./gpudrv_client 1048576
   ```

   Expect to see results printed (e.g. `c[0]`, `c[last]`, etc.).

5. Clean up:

   ```bash
   sudo rmmod gpudrv
   ```

---

### C. Perl Benchmark Harness

```bash
cd C_perl_harness
perl run_bench.pl --client ../B_gpudrv/user/gpudrv_client --out results.csv
perl parse_results.pl results.csv
```

This will run multiple client invocations for different buffer sizes and log results into `results.csv`.

---

### D. Windows (Client + Driver Integration)

1. Cross-compile Windows client (from Linux):

   ```bash
   cd D_windows
   x86_64-w64-mingw32-gcc gpudrv_win_client.c -o gpudrv_win_client.exe
   ```

2. On your Windows test machine:

   * Build & deploy a KMDF-based driver using the snippets in `D_windows/driver_snippets.md`
   * Ensure the driver uses a symbolic link name `\\.\gpudrv` or matching name the client expects

3. Copy `gpudrv_win_client.exe` to Windows and run it:

   ```cmd
   gpudrv_win_client.exe
   ```

   You should see messages like “Submitted workload …” and “Status … completed …”

---

## 🧠 Design Concepts & Theoretical Notes

This project touches several advanced systems concepts:

* **Control vs Data Plane**: Driver is control plane (queuing, notifications). User-space is data plane (runs GPU compute).
* **IOCTL / Device APIs**: Cross-platform abstraction to issue control commands in both Linux and Windows.
* **Buffer management / memory copying**: Prototype uses copy-based approach; advanced versions can use `mmap()`, DMA, or vendor zero-copy (e.g. GPUDirect).
* **Synchronization & queues**: FIFO queues, wake-ups (`wait_event_interruptible`), mutexes for concurrency control.
* **OpenCL runtime & profiling**: Selecting platforms/devices, building kernels, enqueueing NDRange, retrieving profiling info.
* **Cross-platform compatibility**: Shared IOCTL definitions with conditional compilation, matching semantics across Linux & Windows.
* **Automation and benchmarking**: Using Perl (or any scripting) to run systematic experiments, parse data, and plot.

---

## 📈 Future Enhancements & Roadmap

* Replace the copy-based I/O path with **mmap / zero-copy** buffer sharing
* Use **IDs & eventfd / signals** instead of simplistic read/write full buffers
* Add support for **multiple concurrent requests**, **parallel GPU streams**, **batch queues**
* Extend to more complex kernels (encryption, convolution, deep learning ops)
* Integrate with real network stacks (DPDK, XDP) to offload packet processing
* Provide a full Windows driver (KMDF) implementing symmetric behavior
* Add CI/CD pipelines, unit tests, and automated correctness verification

---

## 📝 Contributing & License

Contributions are welcome — open issues or pull requests.
This prototype is licensed under **MIT License** (unless you want another — insert your license here).

---

## 📧 Contact

For questions, feedback, or collaboration:
**GitHub**: [Abhinavcodez](https://github.com/Abhinavcodez)
