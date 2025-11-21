#!/bin/bash
echo "Running all.flow test suite"
echo "============================"

# Test Case 1: Simple two-component cycle
echo -e "\nTest 1: Simple two-component cycle (pipe1 ↔ pipe2)"
./flow your_tests.flow pipe1
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected cycle in pipe1"
else
    echo "✓ PASSED: Cycle detected in pipe1"
fi

# Test Case 2: Self-referencing pipe
echo -e "\nTest 2: Self-referencing pipe"
./flow your_tests.flow self_cycle
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected self-cycle"
else
    echo "✓ PASSED: Self-cycle detected"
fi

# Test Case 3: Three-component cycle
echo -e "\nTest 3: Three-component cycle (a → b → c → a)"
./flow your_tests.flow cycle_a
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected cycle in cycle_a"
else
    echo "✓ PASSED: Cycle detected in three-component chain"
fi

# Test Case 4: Cycle through concatenate
echo -e "\nTest 4: Cycle through concatenate"
./flow your_tests.flow concat_cycle
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected cycle through concatenate"
else
    echo "✓ PASSED: Cycle detected through concatenate"
fi

# Test Case 5: Complex nested cycle
echo -e "\nTest 5: Complex nested cycle"
./flow your_tests.flow complex_1
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected complex cycle"
else
    echo "✓ PASSED: Complex cycle detected"
fi

# Test Case 6: Cycle through stderr
echo -e "\nTest 6: Cycle through stderr redirect"
./flow your_tests.flow stderr_redirect
if [ $? -eq 0 ]; then
    echo "❌ FAILED: Should have detected cycle through stderr"
else
    echo "✓ PASSED: Cycle through stderr detected"
fi

# Test Case 7: Valid case (no cycle)
echo -e "\nTest 7: Valid case - no cycle (should succeed)"
./flow your_tests.flow valid_pipe
if [ $? -eq 0 ]; then
    echo "✓ PASSED: Valid pipe executed successfully"
else
    echo "❌ FAILED: Valid pipe should have executed without error"
fi

echo -e "\n============================="
echo "Cycle detection tests complete!"

echo "Other Standard tests"

declare -a tests=(
  "basic_pipe"
  "concat_test"
  "stderr_test"
  "stderr_pipe"
  "file_pipe_test"
  "chained_pipe"
  "parallel_foo"
  "parallel_bar"
  "empty_input"
  "bad_pipe"
)

for t in "${tests[@]}"; do
    echo -e "\n---- Test: $t ----"
    ./flow your_tests.flow "$t"
done


