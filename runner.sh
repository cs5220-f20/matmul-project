#!/bin/sh

# Make sure OpenBLAS is running serially...
export OMP_NUM_THREADS=1
hostname > info-$2.out
$1 timing-$2.csv >> info-$2.out
