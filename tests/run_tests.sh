#!/bin/bash
set -e

ZENITH_BIN="./bin/zenithpy"
TEST_FILES=(
    "tests/test_arithmetic.py"
    "tests/test_loops.py"
    "tests/test_functions.py"
    "tests/test_lists.py"
)

echo "=========================================="
echo " Running ZenithPy Automated Test Suite"
echo "=========================================="

PASSED=0
TOTAL=0

for test_file in "${TEST_FILES[@]}"; do
    TOTAL=$((TOTAL + 1))
    echo -n "Testing $test_file... "
    OUTPUT_ZENITH=$($ZENITH_BIN "$test_file" 2>&1)
    OUTPUT_PY3=$(python3 "$test_file" 2>&1)

    if [ "$OUTPUT_ZENITH" == "$OUTPUT_PY3" ]; then
        echo "PASS ✓"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL ✗"
        echo "  [Expected]: $OUTPUT_PY3"
        echo "  [Got]:      $OUTPUT_ZENITH"
        exit 1
    fi
done

echo "=========================================="
echo "All tests passed! ($PASSED / $TOTAL)"
echo "=========================================="
