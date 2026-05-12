total_succ=0
total=0

score_tests() {
    local folder="$1"
    local expected="$2"
    local label="$3"
    local suc_c=0
    local all=0

    for f in "$folder"/*; do
        if [ -f "$f" ]; then
            echo "$f :"
            ./bin/tpcas --dry-run < "$f" 1>/dev/null
            res=$?
            if [ "$res" -eq "$expected" ]; then
                suc_c=$((suc_c + 1))
            fi
            all=$((all + 1))
        fi
    done

    echo "$label"
    echo "$suc_c / $all tests passed successfully"
    echo
    
    total_succ=$((total_succ + suc_c))
    total=$((total + all))
}

score_tests "./tests/correct" 0 "Correct tests score:"

score_tests "./tests/incorrect" 1 "Gramatically incorrect tests score:"

score_tests "./tests/sem_inc" 2 "Sematically incorrect tests score:"

echo
echo "Total score:"
echo $total_succ / $total tests passed successfully