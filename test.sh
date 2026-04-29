suc_c=0
all=0

echo "Checking for correct tests..."
for f in ./tests/correct/*; do
    echo $f :
    ./bin/tpcas --dry-run < $f
    res=$?
    if [ $res -eq 0 ]
    then
        suc_c=$(($suc_c + 1))
    fi
    all=$(($all + 1))
done

echo
echo "Checking on  gramtically incorrect tests..."
for f in ./tests/incorrect/*; do
    echo $f :
    ./bin/tpcas --dry-run < $f
    res=$?
    if [ $res -eq 1 ]
    then
        suc_c=$(($suc_c + 1))
    fi
    all=$(($all + 1))
done

echo "Checking on semantically incorrect tests..."
for f in ./tests/sem_inc/*; do
    echo $f :
    ./bin/tpcas --dry-run < $f
    res=$?
    if [ $res -eq 2 ]
    then
        suc_c=$(($suc_c + 1))
    fi
    all=$(($all + 1))
done
#todo: separate scores for each group
echo
echo $suc_c / $all tests passed successfully