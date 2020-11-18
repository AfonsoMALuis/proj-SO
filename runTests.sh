#!/bin/bash
mkdir $2

for input in ${1}/*.txt
do
  #echo filename : ==== ${input} ====
  for i in $(seq 1 ${3})
  do
    input_name="$(basename --suffix=.txt $input)"
    echo InputFile= ${input_name} NumThreads= ${i}
    output="$(basename --suffix=.txt $input)"-${i}.txt
    ./tecnicofs $input ${2}/${output} ${i} | grep TecnicoFS
  done
done
