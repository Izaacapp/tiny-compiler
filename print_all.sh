#!/bin/bash

print_files_in_directory() {
    local directory=$1
    local output_file=$2

    echo "Printing contents of $directory directory" >> $output_file
    echo "=====================================" >> $output_file

    for file in $directory/*.txt; do
        if [[ -f $file ]]; then
            echo "File: $file" >> $output_file
            cat $file >> $output_file
            echo -e "\n" >> $output_file
        fi
    done
}

output_file="all_test_cases.txt"

# Clear the output file if it already exists
> $output_file

# Print files in correct directory
print_files_in_directory "correct" $output_file

# Print files in errors directory
print_files_in_directory "errors" $output_file

echo "All files have been printed to $output_file"
