n=$(($1-1))
cd tester
for i in `seq 0 $n`
do
    t=$(printf "%d" $i)
    python3 generator.py $t > input/input_$t.txt
done