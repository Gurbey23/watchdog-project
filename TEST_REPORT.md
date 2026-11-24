
# Watchdog - Test Report

## 1. Test Environment

- Operating System: Ubuntu (WSL2)
- Programming Language: C
- Compiler: GCC
- Build Tool: GNU Make
- Test Script: Bash
- Test Date: November 24, 2026

## 2. Test Objective

The purpose of testing is to verify that the watchdog
correctly monitors child processes, handles POSIX signals,
performs bounded restarts, and records process events.

## 3. Test Execution

Build the project:

    make

Run the automated tests:

    bash tests/test_scenarios.sh

The test script uses temporary directories to isolate
test logs and cleans up watchdog processes after testing.

## 4. Automated Test Results

| ID | Test Scenario | Expected Result | Actual Result |
|---|---|---|---|
| TC-01 | Logger functionality | START, STATUS, and EXIT events are recorded | PASS |
| TC-02 | SIGUSR1 status report | Running child PID and status are logged | PASS |
| TC-03 | Unexpected child termination | The child is restarted with a new PID | PASS |
| TC-04 | Maximum restart limit | No restart occurs after 3 attempts | PASS |
| TC-05 | Normal child termination | Exit code 0 does not trigger a restart | PASS |
| TC-06 | Graceful shutdown | Watchdog and running children terminate safely | PASS |
| TC-07 | Error exit handling | Exit code 1 triggers automatic restart | PASS |

## 5. Manual Integration Testing

The following behaviors were also verified manually:

- Watchdog successfully started two worker processes.
- SIGUSR1 generated a status record for the running worker.
- SIGKILL triggered automatic child restart.
- The restart counter increased from 1/3 to 3/3.
- A fourth unexpected termination did not trigger another restart.
- SIGTERM initiated graceful shutdown.
- Process inspection confirmed that the tested processes terminated.

## 6. Test Summary

Total automated test cases: 7

Passed: 7

Failed: 0

Result: All seven automated tests passed in the
local Ubuntu test environment.

## 7. Notes

These results demonstrate the tested scenarios.
Additional stress testing and failure-injection testing
may be needed to evaluate other edge cases.
