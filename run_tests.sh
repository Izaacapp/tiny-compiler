#!/bin/bash

# Compile the parsercodegen.c program
gcc parsercodegen.c -o parsercodegen
if [ $? -ne 0 ]; then
  echo "Compilation failed"
  exit 1
fi

# Define arrays of input and expected output files
input_files=("input1.txt" "input2.txt" "input3.txt" "input4.txt" "input5.txt" "input6.txt" "input7.txt" "input8.txt" "input9.txt" "input10.txt" "input11.txt" "input12.txt" "input13.txt" "input14.txt" "input15.txt")
output_files=("output1.txt" "output2.txt" "output3.txt" "output4.txt" "output5.txt" "output6.txt" "output7.txt" "output8.txt" "output9.txt" "output10.txt" "output11.txt" "output12.txt" "output13.txt" "output14.txt" "output15.txt")

# Log file to store the results
log_file="test_results.log"

# Initialize the log file
echo "Test Results - $(date)" > $log_file
echo "=====================================================================================================================" >> $log_file
printf "%-50s | %-50s\n" "Program Output" "Expected Output" >> $log_file
echo "=====================================================================================================================" >> $log_file

# Function to run a test case
run_test() {
  input_file=$1
  expected_output_file=$2
  temp_output_file="${input_file}.temp.out"

  echo "Running test with input: $input_file" | tee -a $log_file

  # Run the parsercodegen program with the input file
  ./parsercodegen "$input_file" > "$temp_output_file" 2>&1
  if [ $? -ne 0 ]; then
    echo "Execution failed for input: $input_file" | tee -a $log_file
  fi

  # Remove trailing whitespaces for comparison
  sed -i 's/[[:space:]]*$//' "$temp_output_file"
  sed -i 's/[[:space:]]*$//' "$expected_output_file"

  # Compare the outputs and log the differences
  if diff -q "$temp_output_file" "$expected_output_file" > /dev/null; then
    echo "Test passed for input: $input_file" | tee -a $log_file
  else
    echo "Test failed for input: $input_file" | tee -a $log_file
    echo "Differences:" | tee -a $log_file
  fi

  # Append the temporary and expected output to the log file for review
  # Read the files line by line and print them side by side
  paste <(cat "$temp_output_file") <(cat "$expected_output_file") | awk -F '\t' '{printf "%-50s | %-50s\n", $1, $2}' >> $log_file

  echo "=====================================================================================================================" >> $log_file

  # Clean up temporary files
  rm "$temp_output_file"
}

# Run all test cases
for (( i=0; i<${#input_files[@]}; i++ )); do
  run_test "${input_files[$i]}" "${output_files[$i]}"
done

echo "Test run complete. See $log_file for details."
