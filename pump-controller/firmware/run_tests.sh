#!/bin/sh
# Builds and runs the pump logic tests on a PC (needs g++)
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -Wall -Wextra -O1 -o "${TMPDIR:-/tmp}/pump_tests" test/test_logic.cpp src/pump_logic.cpp
"${TMPDIR:-/tmp}/pump_tests"
