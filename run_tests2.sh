for input in correct/input16.txt correct/input17.txt correct/input18.txt correct/input19.txt correct/input20.txt correct/input21.txt correct/input22.txt correct/input23.txt correct/input26.txt correct/input27.txt correct/input31.txt; do
    echo "Running $input"
    ./parsercodegen $input /dev/null
    echo "Finished $input"
    echo
done
