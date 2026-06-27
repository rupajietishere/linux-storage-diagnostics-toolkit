#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

#define SOCKET_PATH "/tmp/storage_diag.sock"
#define PARTITION_LEN 32
#define STATUS_LEN 16

// Packing ensures no compiler-added padding, making it easier to parse in Python
#pragma pack(push, 1)
struct DiagData {
    char partition[PARTITION_LEN];
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t target_pid;
    uint64_t vm_size_kb;
    uint64_t rss_size_kb;
    char status[STATUS_LEN];
};
#pragma pack(pop)

#endif // MONITOR_H