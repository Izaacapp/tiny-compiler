for input in errors/input1.txt errors/input2.txt errors/input3.txt errors/input4.txt errors/input5.txt errors/input6.txt errors/input7.txt errors/input8.txt errors/input9.txt errors/input10.txt errors/input11.txt errors/input12.txt errors/input13.txt errors/input14.txt errors/input15.txt errors/input24.txt errors/input25.txt errors/input28.txt errors/input29.txt errors/input30.txt errors/input32.txt; do
    echo "Running $input"
    ./parsercodegen $input /dev/null
    echo "Finished $input"
    echo
done
