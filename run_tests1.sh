#!/bin/bash

# Compile the parsercodegen.c program
gcc parsercodegen.c -o parsercodegen
if [ $? -ne 0 ]; then
  echo "Compilation failed"
  exit 1
fi

# Define arrays of input and expected output files for both correct and errors directories
correct_input_files=("input16.txt" "input17.txt" "input18.txt" "input19.txt" "input20.txt" "input21.txt" "input22.txt" "input23.txt" "input26.txt")
correct_output_files=("output16.txt" "output17.txt" "output18.txt" "output19.txt" "output20.txt" "output21.txt" "output22.txt" "output23.txt" "output26.txt")

error_input_files=("input1.txt" "input2.txt" "input3.txt" "input4.txt" "input5.txt" "input6.txt" "input7.txt" "input8.txt" "input9.txt" "input10.txt" "input11.txt" "input12.txt" "input13.txt" "input14.txt" "input15.txt" "input24.txt" "input25.txt")
error_output_files=("output1.txt" "output2.txt" "output3.txt" "output4.txt" "output5.txt" "output6.txt" "output7.txt" "output8.txt" "output9.txt" "output10.txt" "output11.txt" "output12.txt" "output13.txt" "output14.txt" "output15.txt" "output22.txt" "output23.txt" "output24.txt" "output25.txt" "output26.txt")

# Log file to store the results
log_file="test_results.log"

# Initialize the log file
echo "Test Results - $(date)" > $log_file
echo "=====================================================================================================================" >> $log_file
printf "%-50s | %-50s\n" "Program Output" "Expected Output" >> $log_file
echo "=====================================================================================================================" >> $log_file

# Function to run a test case
run_test() {
  local dir=$1
  local input_file=$2
  local expected_output_file=$3
  local temp_output_file="${input_file}.temp.out"

  input_path="$dir/$input_file"
  expected_output_path="$dir/$expected_output_file"

  echo "Running test with input: $input_path" | tee -a $log_file

  # Run the parsercodegen program with the input file
  ./parsercodegen "$input_path" > "$temp_output_file" 2>&1
  if [ $? -ne 0 ]; then
    echo "Execution failed for input: $input_path" | tee -a $log_file
  fi

  # Remove trailing whitespaces for comparison
  awk '{$1=$1}1' "$temp_output_file" > temp && mv temp "$temp_output_file"
  awk '{$1=$1}1' "$expected_output_path" > temp && mv temp "$expected_output_path"

  # Compare the outputs and log the differences
  if diff -q "$temp_output_file" "$expected_output_path" > /dev/null; then
    echo "Test passed for input: $input_path" | tee -a $log_file
  else
    echo "Test failed for input: $input_path" | tee -a $log_file
    echo "Differences:" | tee -a $log_file
    # Append the temporary and expected output to the log file for review
    # Read the files line by line and print them side by side
    paste <(cat "$temp_output_file") <(cat "$expected_output_path") | awk -F '\t' '{printf "%-50s | %-50s\n", $1, $2}' >> $log_file
  fi

  echo "=====================================================================================================================" >> $log_file

  # Clean up temporary files
  rm "$temp_output_file"
}

# Run all test cases in the correct directory
for (( i=0; i<${#correct_input_files[@]}; i++ )); do
  run_test "correct" "${correct_input_files[$i]}" "${correct_output_files[$i]}"
done

# Run all test cases in the errors directory
for (( i=0; i<${#error_input_files[@]}; i++ )); do
  run_test "errors" "${error_input_files[$i]}" "${error_output_files[$i]}"
done

echo "Test run complete. See $log_file for details."
