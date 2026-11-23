
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
