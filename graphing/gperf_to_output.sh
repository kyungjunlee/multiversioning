#!/bin/bash

heap="heap"
cpu="cpu"

mkdir -p gperf

for file in ../gperf/*
do
  if [ ${file: -3} != ".sh" ] && [ -f "$file" ]; then
    to_ps=${file:3}.ps
    to_txt=${file:3}.txt
    if [ "$1" == "$heap" ] && [ ${file: -5} == ".heap" ]; then
      echo $file to $to_ps
      pprof --ps ../build/batch_db $file > ${to_ps}
      echo $file to $to_txt
      pprof --text ../build/batch_db $file > ${to_txt}
    elif [ "$1" == "$cpu" ] && [ ${file: -4} == ".cpu" ]; then
      echo $file to $to_ps
      pprof --ps ../build/batch_db $file > ${to_ps}
      echo $file to $to_txt
      pprof --text ../build/batch_db $file > ${to_txt}
    fi
  fi
done

