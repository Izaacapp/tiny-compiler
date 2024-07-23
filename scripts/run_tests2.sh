#!/bin/bash

# Compile the parsercodegen.c program
gcc parsercodegen.c -o a.out
if [ $? -ne 0 ]; then
  echo "Compilation failed"
  exit 1
fi

# Define arrays of input and expected output files
inputs=(
    "correct/input16.txt"
    "correct/input17.txt"
    "correct/input18.txt"
    "correct/input19.txt"
    "correct/input20.txt"
    "correct/input21.txt"
    "correct/input22.txt"
    "correct/input23.txt"
    "correct/input26.txt"
    "correct/input27.txt"
    "correct/input31.txt"
)
expected_outputs=(
    "correct/output16.txt"
    "correct/output17.txt"
    "correct/output18.txt"
    "correct/output19.txt"
    "correct/output20.txt"
    "correct/output21.txt"
    "correct/output22.txt"
    "correct/output23.txt"
    "correct/output26.txt"
    "correct/output27.txt"
    "correct/output31.txt"
)

# Log file to store the results
log_file="test_results.log"

# Initialize the log file
echo "Test Results - $(date)" > $log_file
echo "=====================================================================================================================" >> $log_file
printf "%-60s | %-60s\n" "Expected Output" "Program Output" >> $log_file
echo "=====================================================================================================================" >> $log_file

# Function to run a test case
run_test() {
  input_file=$1
  expected_output_file=$2
  temp_output_file="${input_file}.temp.out"

  echo "Running test with input: $input_file" | tee -a $log_file

  # Run the a.out program with the input file
  ./a.out "$input_file" "$temp_output_file"
  if [ $? -ne 0 ]; then
    echo "Execution failed for input: $input_file" | tee -a $log_file
    echo "=====================================================================================================================" >> $log_file
    return
  fi

  # Remove trailing whitespaces for comparison
  awk '{$1=$1}1' "$temp_output_file" > temp && mv temp "$temp_output_file"
  awk '{$1=$1}1' "$expected_output_file" > temp && mv temp "$expected_output_file"

  # Compare the outputs and log the differences
  if diff -q "$temp_output_file" "$expected_output_file" > /dev/null; then
    echo "Test passed for input: $input_file" | tee -a $log_file
  else
    echo "Test failed for input: $input_file" | tee -a $log_file
    echo "Differences:" | tee -a $log_file

    # Extract and compare Symbol Table
    awk '/Symbol Table:/,/Assembly Code:/' "$expected_output_file" | sed '/Assembly Code:/d' > temp_symbol_exp
    awk '/Symbol Table:/,/Assembly Code:/' "$temp_output_file" | sed '/Assembly Code:/d' > temp_symbol_out
    paste -d'|' <(awk '{printf "%-60s\n", $0}' temp_symbol_exp) <(awk '{printf "%-60s\n", $0}' temp_symbol_out) >> $log_file

    # Extract and compare Assembly Code
    awk '/Assembly Code:/,0' "$expected_output_file" > temp_assembly_exp
    awk '/Assembly Code:/,0' "$temp_output_file" > temp_assembly_out
    paste -d'|' <(awk '{printf "%-60s\n", $0}' temp_assembly_exp) <(awk '{printf "%-60s\n", $0}' temp_assembly_out) >> $log_file
  fi

  echo "=====================================================================================================================" >> $log_file

  # Clean up temporary files
  rm "$temp_output_file" temp_assembly_out temp_assembly_exp temp_symbol_out temp_symbol_exp
}

# Run all test cases
for (( i=0; i<${#inputs[@]}; i++ )); do
  run_test "${inputs[$i]}" "${expected_outputs[$i]}"
done

echo "Test run complete. See $log_file for details."
