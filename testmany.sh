n=$(($1-1))
g++ -o main.out submission.cpp
cd tester
for i in `seq 0 $n`
do
    t=$(printf "%d" $i)
    ../main.out < input/input_$t.txt > output/output_$t.txt &
done
wait
for i in `seq 0 $n`
do
    t=$(printf "%d" $i)
    if [ $i = 0 ]; then
        python3 judge.py input/input_$t.txt output/output_$t.txt > scores.txt
    else 
        python3 judge.py input/input_$t.txt output/output_$t.txt >> scores.txt
    fi
done
python calc.py