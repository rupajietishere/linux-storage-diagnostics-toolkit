import os
import socket
import struct
import sys

SOCKET_PATH = "/tmp/storage_diag.sock"
# Format string to unpack the C-struct: 
# 32s (char[32]) + Q (uint64) + Q (uint64) + I (uint32) + Q (uint64) + Q (uint64) + 16s (char[16])
# '<' forces standard little-endian representation (used on x86_64)
STRUCT_FORMAT = "<32sQQIQQ16s"
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

def clean_socket():
    if os.path.exists(SOCKET_PATH):
        os.remove(SOCKET_PATH)

def analyze_and_log(data):
    # Unpack binary data
    unpacked = struct.unpack(STRUCT_FORMAT, data)
    
    # Decode string buffers and strip trailing null bytes
    partition = unpacked[0].decode('utf-8').rstrip('\x00')
    total_bytes = unpacked[1]
    free_bytes = unpacked[2]
    target_pid = unpacked[3]
    vm_size_kb = unpacked[4]
    rss_size_kb = unpacked[5]
    status = unpacked[6].decode('utf-8').rstrip('\x00')

    # Calculate percentages
    used_bytes = total_bytes - free_bytes
    disk_usage_pct = (used_bytes / total_bytes * 100) if total_bytes > 0 else 0.0

    print("\n" + "="*50)
    print("           DIAGNOSTICS REPORT RECEIVED")
    print("="*50)
    print(f"Target Partition:   {partition}")
    print(f"Total / Used Space: {total_bytes / (1024**3):.2f} GB / {used_bytes / (1024**3):.2f} GB ({disk_usage_pct:.1f}% used)")
    print(f"Target Process PID: {target_pid}")
    print(f"Virtual Memory Size:{vm_size_kb / 1024:.2f} MB")
    print(f"Resident Set Size:  {rss_size_kb / 1024:.2f} MB")
    print(f"Agent Status:       {status}")
    print("-"*50)

    # Simple Fault Detection Engine (Rule-based)
    print("[Analyzer Status Alerts]")
    faults_detected = 0
    
    if disk_usage_pct > 80.0:
        print(f"  [CRITICAL] HIGH DISK USAGE: {partition} usage is at {disk_usage_pct:.1f}%!")
        faults_detected += 1
    else:
        print("  [OK] Disk space utilization is within safe thresholds.")

    if target_pid != 0 and rss_size_kb > 500 * 1024:  # Alert if RSS > 500MB
        print(f"  [WARNING] HIGH MEMORY FOOTPRINT: PID {target_pid} RSS is exceeding 500MB!")
        faults_detected += 1
    elif target_pid != 0:
        print(f"  [OK] Process memory footprint is stable.")

    if faults_detected == 0:
        print("  [HEALTHY] No issues detected in this sweep cycle.")
    print("="*50 + "\n")

def run_server():
    clean_socket()

    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(SOCKET_PATH)
    server.listen(1)

    print(f"[Python-Analyzer] Daemon listening on local UNIX socket: {SOCKET_PATH}")
    print("[Python-Analyzer] Waiting for incoming system snapshots...")

    try:
        while True:
            conn, _ = server.accept()
            try:
                data = conn.recv(STRUCT_SIZE)
                if len(data) == STRUCT_SIZE:
                    analyze_and_log(data)
                else:
                    print(f"[Python-Analyzer] Warning: Received corrupted metrics pack size ({len(data)} bytes). Expected {STRUCT_SIZE}.")
            except Exception as e:
                print(f"[Python-Analyzer] Error processing data stream: {e}")
            finally:
                conn.close()
    except KeyboardInterrupt:
        print("\n[Python-Analyzer] Shutting down daemon listener gracefully.")
    finally:
        clean_socket()

if __name__ == "__main__":
    run_server()