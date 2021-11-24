#!/bin/bash

bms=(Bubblesort Dhrystone FloatMM IntMM Makefile miniAMR miniGMG Oscar Perm Quicksort RealMM RSBench Towers Treesort XSBench)

if [ ! -d App_Results ]; then
          mkdir -p App_Results;
fi

for bm in "${bms[@]}"
do
echo $bm
build/X86/gem5.opt -re --outdir=App_Results/$bm configs-se/run_se.py ddr4_2400 nvm_2400 timing "/data/aakahlow-new/flexcpu-experiements/benchmarks/original/$bm/$bm"
done
