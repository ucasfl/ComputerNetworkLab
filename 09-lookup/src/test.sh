#!/usr/bin/env bash

make clean && make;
echo "Begin Test";
cat test-data.txt | ./tree;

