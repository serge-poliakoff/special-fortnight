suc_c=0
all=0

echo "Checking for correct tests..."
for f in ./tests/correct/*; do
    echo $f :
    ./bin/tpcas --dry-run < $f
    res=$?
    suc_c=$(($suc_c + (1 - $res)))
    all=$(($all + 1))
done

echo
echo "Checking on icorrect tests..."
for f in ./tests/incorrect/*; do
    echo $f :
    ./bin/tpcas --dry-run < $f
    res=$?
    if [ $res -gt 0 ]
    then
        suc_c=$(($suc_c + 1))
    fi
    all=$(($all + 1))
done

echo
echo $suc_c / $all tests passed successfully