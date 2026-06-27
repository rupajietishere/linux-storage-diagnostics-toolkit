#!/bin/bash

# Exit script if any command fails
set -e

echo "=== 1. Compiling Diagnostics Agent Core ==="
make clean
make

echo "=== 2. Creating Python Virtual Environment & Testing Setup ==="
# No external dependencies are needed since we only use core libraries!

# Get the PID of our shell process or a dummy process to monitor
MY_PID=$$
echo "[Setup] Tracking self-monitoring target process PID: $MY_PID"

echo "=== 3. Launching Python Analyzer Daemon (Background Process) ==="
# Start python script in background, redirection ensures cleaner stdout piping
python3 scripts/analyzer.py &
ANALYZER_PID=$!

# Ensure the background process is killed when this shell script exits
trap "kill $ANALYZER_PID 2>/dev/null || true; rm -f /tmp/storage_diag.sock" EXIT

# Give Python socket daemon half a second to initialize and bind
sleep 0.5

echo "=== 4. Launching C Monitoring Diagnostics Agent ==="
# We pass "/" as target mount partition to monitor and the current PID
./storage_diag_agent / $MY_PID

echo "=== 5. Complete. Press CTRL+C to terminate the background analyzer ==="
wait $ANALYZER_PID