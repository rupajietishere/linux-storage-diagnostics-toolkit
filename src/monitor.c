#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/statvfs.h>
#include "monitor.h"

// Helper function to read process memory stats from /proc/[pid]/statm
int get_process_memory(uint32_t pid, uint64_t *vm_size_kb, uint64_t *rss_size_kb) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/statm", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1; // Process doesn't exist or permission denied
    }

    long pages, rss;
    // statm format: size resident shared text library data dirty
    if (fscanf(fp, "%ld %ld", &pages, &rss) != 2) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    // Get system page size (usually 4KB)
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    *vm_size_kb = (pages * page_size) / 1024;
    *rss_size_kb = (rss * page_size) / 1024;
    return 0;
}

// Helper function to fetch disk partition space using statvfs
int get_disk_usage(const char *path, uint64_t *total, uint64_t *free) {
    struct statvfs vfs;
    if (statvfs(path, &vfs) < 0) {
        return -1;
    }
    // Block size * total blocks
    *total = (uint64_t)vfs.f_blocks * vfs.f_frsize;
    // Block size * free blocks available to non-privileged users
    *free = (uint64_t)vfs.f_bavail * vfs.f_frsize;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mount_path> <target_pid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *mount_path = argv[1];
    uint32_t target_pid = (uint32_t)atoi(argv[2]);

    struct DiagData metrics;
    memset(&metrics, 0, sizeof(struct DiagData));

    // 1. Gather Disk Space Metrics
    if (get_disk_usage(mount_path, &metrics.total_bytes, &metrics.free_bytes) == 0) {
        strncpy(metrics.partition, mount_path, PARTITION_LEN - 1);
    } else {
        perror("Error fetching disk usage");
        return EXIT_FAILURE;
    }

    // 2. Gather Target Process Memory Metrics
    metrics.target_pid = target_pid;
    if (get_process_memory(target_pid, &metrics.vm_size_kb, &metrics.rss_size_kb) != 0) {
        fprintf(stderr, "Warning: Could not read /proc/%u/statm. Process might not exist.\n", target_pid);
        metrics.vm_size_kb = 0;
        metrics.rss_size_kb = 0;
    }

    strncpy(metrics.status, "ACTIVE", STATUS_LEN - 1);

    // 3. Establish IPC (Unix Domain Socket) and transmit metrics
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    printf("[C-Agent] Connecting to diagnostics collector daemon...\n");
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Connection to socket failed. Is the Python analyzer running?");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("[C-Agent] Transmitting diagnostics payload (%zu bytes) to analyzer...\n", sizeof(struct DiagData));
    if (send(sock_fd, &metrics, sizeof(struct DiagData), 0) < 0) {
        perror("Failed to send diagnostics payload");
    } else {
        printf("[C-Agent] Payload successfully transmitted.\n");
    }

    close(sock_fd);
    return EXIT_SUCCESS;
}