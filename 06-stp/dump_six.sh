#!/bin/bash

for i in `seq 1 6`;
do
	echo "NODE b$i dumps:";
	declare -i b=$(cat b$i-output.txt | wc -l );
	tail -`expr $b - 2` b$i-output.txt;
	echo "";
done
