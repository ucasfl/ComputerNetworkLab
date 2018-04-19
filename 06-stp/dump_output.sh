#!/bin/bash

for i in `seq 1 4`;
do
	echo "NODE b$i dumps:";
	tail -5 b$i-output.txt;
	echo "";
done
