#!/bin/sh

rm -rf results
mkdir results

for i in 1 2 3 4 5 6 7 8 9; do
	echo -e "\n\n******** One Point (Run $i)\n" >> results/one_point.txt
	./eugene --labs 22 1000 0.1 1 L 20000 --quiet >> results/one_point.txt
done


for i in 1 2 3 4 5 6 7 8 9; do
	echo -e "\n\n******** Two Point (Run $i)\n" >> results/two_point.txt
	./eugene --labs 22 1000 0.1 2 L 20000 --quiet >> results/two_point.txt
done


for i in 1 2 3 4 5 6 7 8 9; do
	echo -e "\n\n******** Uniform (Run $i)\n" >> results/uniform.txt
	./eugene --labs 22 1000 0.1 U L 20000 --quiet >> results/uniform.txt
done
