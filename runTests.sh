#!/bin/bash
mkdir $2

for input in ${1}/*.txt

do

  echo filename : ==== ${input} ====
  for i in $(seq 1 ${3})
  do
    echo InputFile= ${input} NumThreads= $i
    ./tecnicofs $input ${2}/"$(basename --suffix=.txt $input)"-${i}.txt $i
  done
done
