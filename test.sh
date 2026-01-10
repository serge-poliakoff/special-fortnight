suc_c=0
all=0
for f in ./tests/correct/*; do
    ./bin/tpcas < $f
    res=$?
    if [ $res -gt 0 ]
    then
        echo "test $f has failed !"
    fi
    suc_c=$(($suc_c + (1 - $res)))
    all=$(($all + 1))
done

echo $suc_c / $all tests passed successfully