# Linux Storage Diagnostics Toolkit

A lightweight, high-performance systems diagnostic toolkit engineered to monitor enterprise storage health and target-process memory utilization on Linux hosts. The architecture decouples low-overhead systems data collection (written in C) from policy analysis and logging (written in Python) using local **Unix Domain Sockets (IPC)**.

---

## 🏗️ Architecture Overview

The system utilizes an agent-collector design to achieve minimal CPU and memory footprints on production storage nodes:

```text
+---------------------------------------------+
|               Linux Kernel                  |
|    - Filesystem structures (statvfs)        |
|    - Virtual Process Stats (/proc/[PID])    |
+----------------------+----------------------+
                       | (Direct system queries)
                       v
+----------------------+----------------------+
|    Low-Overhead C Diagnostics Agent         |
|    - Collects filesystem space metrics      |
|    - Parses kernel /proc/PID/statm state    |
|    - Serializes metrics into standard struct|
+----------------------+----------------------+
                       |
                       | Local IPC (Unix Domain Socket: SOCK_STREAM)
                       v
+----------------------+----------------------+
|     Python Policy Analyzer Daemon          |
|    - Unpacks binary C-struct stream         |
|    - Evaluates system state against rules   |
|    - Outputs structured health alerts       |
+---------------------------------------------+
```

---

## 🚀 Key Engineering & Design Features

### 1. Zero-Allocation Binary Serialization (`#pragma pack`)
Unlike typical monitoring tools that serialize data to verbose formats like JSON or XML, this toolkit transfers raw binary structures directly. Using `#pragma pack(push, 1)`, we enforce a deterministic 84-byte network payload without compiler-added padding bytes. This allows Python's `struct` interface to unpack the socket buffer with exact memory offset mapping and zero parsing overhead.

### 2. Native Kernel State Introspection via `/proc`
Instead of fork-executing system utilities (like calling shell commands inside loop structures), the C monitoring agent parses the Virtual Filesystem (`/proc/[target_pid]/statm`) directly. This approach bypasses expensive context-switching and parses kernel page statistics in-memory.

### 3. POSIX API Integration
Filesystem geometry and available capacity parameters are safely queried using the POSIX standard `statvfs` system call. The agent computes available blocks against unprivileged user quotas to prevent system write failure scenarios.

### 4. Efficient Inter-Process Communication (IPC)
The toolkit binds to local Unix Domain stream sockets (`AF_UNIX`, `SOCK_STREAM`). This bypasses the overhead of the network stack (TCP/IP loopback parsing, sliding window validation, packet division), providing maximum local communication throughput.

---

## 📁 Repository Structure

```text
linux-storage-diagnostics-toolkit/
├── Makefile                 # Automates optimized GCC compilation (-O2)
├── README.md                # Technical documentation
├── run.sh                   # Comprehensive environment orchestrator
├── src/
│   ├── monitor.c            # Systems-level C diagnostics logic
│   └── monitor.h            # Common packed data structure contract
└── scripts/
    └── analyzer.py          # Python daemon & rule evaluation engine
```

---

## 🛠️ Compilation & Quick Start

To compile, provision, execute, and clean up the environment automatically:

```bash
# Make the wrapper execution script executable
chmod +x run.sh

# Build and execute the test pipeline
./run.sh
```

### Manual Compiling and Running
If you prefer to run the components manually in separate terminals:

**Terminal 1 (Start the Python Collector Daemon):**
```bash
python3 scripts/analyzer.py
```

**Terminal 2 (Compile and Run the C Diagnostics Agent):**
```bash
# Compile using the Makefile
make

# Run the agent: ./storage_diag_agent <mount_point> <target_pid>
# Example: Monitor root mount point and track process PID 12435
./storage_diag_agent / 12435
```

---

## 📊 Sample Execution Logs

When executed, the system collects diagnostics and runs the rules evaluation engine:

```text
==================================================
           DIAGNOSTICS REPORT RECEIVED
==================================================
Target Partition:   /
Total / Used Space: 1006.85 GB / 53.15 GB (5.3% used)
Target Process PID: 1296
Virtual Memory Size:4.64 MB
Resident Set Size:  3.25 MB
Agent Status:       ACTIVE
--------------------------------------------------
[Analyzer Status Alerts]
  [OK] Disk space utilization is within safe thresholds.
  [OK] Process memory footprint is stable.
  [HEALTHY] No issues detected in this sweep cycle.
==================================================
```

---

## 💡 System Design Q&A (Interview Cheat-Sheet)

* **Q: Why Unix Domain Sockets over TCP Sockets for local IPC?**
  * **A:** Unix Domain Sockets are faster and consume fewer system resources because they bypass the TCP/IP network stack entirely. They avoid checksum calculations, routing, and sliding-window controls, directly copying memory buffers between user spaces within the kernel boundary.
* **Q: What is the difference between Virtual Memory (VSZ) and Resident Set Size (RSS)?**
  * **A:** Virtual Memory represents the total address space the operating system has reserved for a process (including shared libraries and mapped files). RSS represents the actual physical RAM frames currently allocated to the process execution. Tracking RSS is critical for detecting silent memory leak patterns in high-availability environments.
* **Q: Why did you pack the C structure?**
  * **A:** To guarantee a strict 84-byte payload. Without `#pragma pack(1)`, different compiler versions or target processor architectures may align data fields differently (such as placing 4-byte boundaries around a `char` array), leading to memory alignment corruption when Python attempts to unpack the stream.
```

---

### How to Save This and Finish Pushing:

Once you have saved this write-up inside `README.md`, run these final commands in your WSL terminal to commit the changes and push to GitHub:

```bash
# 1. Stage the updated README.md
git add README.md

# 2. Commit the change
git commit -m "Docs: Update README with project architecture and technical deep-dive"

# 3. Rename the branch, set up remote, and push
git branch -M main
git remote add origin https://github.com/<YOUR-GITHUB-USERNAME>/linux-storage-diagnostics-toolkit.git
git push -u origin main
```