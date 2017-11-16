#!/bin/bash

heap="heap"
cpu="cpu"

mkdir -p gperf

for file in ../gperf/*
do
  if [ ${file: -3} != ".sh" ] && [ -f "$file" ]; then
    to_ps=${file:3}.ps
    if [ "$1" == "$heap" ] && [ ${file: -5} == ".heap" ]; then
      echo $file to $to_ps
      pprof --ps ../build/batch_db $file > ${to_ps}
    elif [ "$1" == "$cpu" ] && [ ${file: -4} == ".cpu" ]; then
      echo $file to $to_ps
      pprof --ps ../build/batch_db $file > ${to_ps}
    fi
  fi
done

