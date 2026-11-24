
#!/usr/bin/env bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$PROJECT_DIR"

echo "Starting logger test..."

make test_logger

TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT

(
    cd "$TEST_DIR"
    "$PROJECT_DIR/test_logger"
)

if grep -q "START" "$TEST_DIR/watchdog.log" &&
   grep -q "STATUS" "$TEST_DIR/watchdog.log" &&
   grep -q "EXIT" "$TEST_DIR/watchdog.log"; then

    echo "PASS: Logger events recorded successfully"

else
    echo "FAIL: Logger events are missing"
    exit 1
fi

echo "Logger test completed."

echo "Starting SIGUSR1 status test..."

make watchdog
make worker

RUN_DIR="$TEST_DIR/watchdog_run"
mkdir -p "$RUN_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$RUN_DIR/workers/worker"

(
    cd "$RUN_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    sleep 2

    kill -USR1 "$watchdog_pid"
    sleep 1

    if grep -q "STATUS.*state=RUNNING" watchdog.log; then
        echo "PASS: SIGUSR1 status report recorded"
    else
        echo "FAIL: SIGUSR1 status report missing"
        exit 1
    fi
)

echo "SIGUSR1 status test completed."


echo "Starting automatic restart test..."

RESTART_DIR="$TEST_DIR/restart_run"
mkdir -p "$RESTART_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$RESTART_DIR/workers/worker"

(
    cd "$RESTART_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    sleep 2

    daemon_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

    if [ -z "$daemon_pid" ]; then
        echo "FAIL: Daemon worker not found"
        exit 1
    fi

    echo "Killing daemon worker PID: $daemon_pid"
    kill -KILL "$daemon_pid"

    sleep 3

    new_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

    if [ -n "$new_pid" ] &&
       [ "$new_pid" != "$daemon_pid" ] &&
       grep -q "Attempt 1/3" watchdog.log; then
        echo "PASS: Worker restarted with new PID: $new_pid"
    else
        echo "FAIL: Automatic restart did not work"
        tail -n 10 watchdog.log
        exit 1
    fi
)

echo "Automatic restart test completed."


echo "Starting restart limit test..."

LIMIT_DIR="$TEST_DIR/restart_limit"
mkdir -p "$LIMIT_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$LIMIT_DIR/workers/worker"

(
    cd "$LIMIT_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    sleep 2

    daemon_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

    if [ -z "$daemon_pid" ]; then
        echo "FAIL: Daemon worker not found"
        exit 1
    fi

    for attempt in 1 2 3; do
        kill -KILL "$daemon_pid"
        sleep 3

        new_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

        if [ -z "$new_pid" ] ||
           [ "$new_pid" = "$daemon_pid" ] ||
           ! grep -q "Attempt $attempt/3" watchdog.log; then
            echo "FAIL: Restart attempt $attempt failed"
            exit 1
        fi

        echo "PASS: Restart attempt $attempt/3"
        daemon_pid="$new_pid"
    done

    # Fourth failure must NOT restart the worker.
    kill -KILL "$daemon_pid"
    sleep 3

    remaining_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

    if [ -z "$remaining_pid" ] &&
       grep -q "Restart limit (3) reached" watchdog.log; then
        echo "PASS: Restart limit enforced after 3 attempts"
    else
        echo "FAIL: Restart limit not enforced"
        exit 1
    fi
)

echo "Restart limit test completed."


echo "Starting normal exit test..."

NORMAL_DIR="$TEST_DIR/normal_exit"
mkdir -p "$NORMAL_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$NORMAL_DIR/workers/worker"

(
    cd "$NORMAL_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    # Wait for the normal worker to finish.
    for ((second=0; second<22; second++)); do
        if grep -q "Child 'worker-1'.*terminated normally (exit code 0)" watchdog.log 2>/dev/null; then
            break
        fi
        sleep 1
    done

    if grep -q "Child 'worker-1'.*terminated normally (exit code 0)" watchdog.log &&
       ! grep -q "Restarting child 'worker-1'" watchdog.log &&
       ! pgrep -P "$watchdog_pid" -f 'workers/worker normal' >/dev/null; then

        echo "PASS: Normal worker exited without restart"
    else
        echo "FAIL: Normal exit handling failed"
        tail -n 10 watchdog.log
        exit 1
    fi
)

echo "Normal exit test completed."


echo "Starting graceful shutdown test..."

SHUTDOWN_DIR="$TEST_DIR/graceful_shutdown"
mkdir -p "$SHUTDOWN_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$SHUTDOWN_DIR/workers/worker"

(
    cd "$SHUTDOWN_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    daemon_pid=""

    for attempt in 1 2 3 4 5; do
        daemon_pid=$(pgrep -P "$watchdog_pid" -f 'workers/worker daemon' || true)

        if [ -n "$daemon_pid" ]; then
            break
        fi

        sleep 1
    done

    if [ -z "$daemon_pid" ]; then
        echo "FAIL: Daemon worker not found"
        exit 1
    fi

    echo "Sending SIGTERM to watchdog PID: $watchdog_pid"

    kill -TERM "$watchdog_pid"

    if ! wait "$watchdog_pid"; then
        echo "FAIL: Watchdog did not exit successfully"
        exit 1
    fi

    if grep -q "Shutdown signal received" watchdog.log &&
       grep -q "SIGTERM sent to child" watchdog.log &&
       grep -q "Watchdog shutdown complete" watchdog.log &&
       ! kill -0 "$daemon_pid" 2>/dev/null; then

        echo "PASS: Graceful shutdown completed successfully"

    else
        echo "FAIL: Graceful shutdown test failed"
        tail -n 10 watchdog.log
        exit 1
    fi
)

echo "Graceful shutdown test completed."


echo "Starting error exit test..."

ERROR_DIR="$TEST_DIR/error_exit"
mkdir -p "$ERROR_DIR"

# Create a temporary worker wrapper for testing.
cat > "$ERROR_DIR/worker" <<'EOF'
#!/usr/bin/env bash

if [[ "${1:-}" == "daemon" ]]; then
    exec "$REAL_WORKER" crash
fi

exec "$REAL_WORKER" "$@"
EOF

chmod +x "$ERROR_DIR/worker"

(
    cd "$ERROR_DIR"

    export REAL_WORKER="$PROJECT_DIR/workers/worker"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    test_passed=0

    for attempt in {1..10}; do
        if grep -q "exited unexpectedly with error code 1" watchdog.log 2>/dev/null &&
           grep -q "Attempt 1/3" watchdog.log 2>/dev/null; then
            test_passed=1
            break
        fi

        sleep 1
    done

    if [ "$test_passed" -eq 1 ]; then
        echo "PASS: Worker error exit detected and restarted"
    else
        echo "FAIL: Worker error exit handling failed"
        tail -n 10 watchdog.log 2>/dev/null || true
        exit 1
    fi
)

echo "Error exit test completed."


echo "Starting SIGINT shutdown test..."

SIGINT_DIR="$TEST_DIR/sigint_shutdown"
mkdir -p "$SIGINT_DIR/workers"

ln -s "$PROJECT_DIR/workers/worker" "$SIGINT_DIR/workers/worker"

(
    cd "$SIGINT_DIR"

    "$PROJECT_DIR/watchdog" &
    watchdog_pid=$!

    trap 'kill -TERM "$watchdog_pid" 2>/dev/null || true; wait "$watchdog_pid" 2>/dev/null || true' EXIT

    sleep 2

    echo "Sending SIGINT to watchdog PID: $watchdog_pid"
    kill -INT "$watchdog_pid"

    if ! wait "$watchdog_pid"; then
        echo "FAIL: Watchdog did not exit successfully"
        exit 1
    fi

    if grep -q "Shutdown signal received" watchdog.log &&
       grep -q "SIGTERM sent to child 'worker-1'" watchdog.log &&
       grep -q "SIGTERM sent to child 'worker-2'" watchdog.log &&
       grep -q "Watchdog shutdown complete" watchdog.log; then

        echo "PASS: SIGINT graceful shutdown completed"
    else
        echo "FAIL: SIGINT shutdown failed"
        tail -n 12 watchdog.log
        exit 1
    fi
)

echo "SIGINT shutdown test completed."
