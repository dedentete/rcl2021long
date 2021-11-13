g++ -o main.out main.cpp
cd tester
../main.out < input/input_8.txt > output/output_8.txt
python3 judge.py input/input_8.txt output/output_8.txt