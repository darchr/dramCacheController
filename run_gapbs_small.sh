#!/bin/bash

if [ $1 != 'kvm'] && [ $1 != 'atomic'] && [ $1 != 'timing'] && [ $1 != 'o3' ]
then
    echo "Wrong cpu model, expecting kvm, atomic, timing or o3"
    exit
fi

if [ $2 != 'classic'] && [ $2 != 'MI_example'] && [ $2 != 'MESI_Two_Level'] && [ $2 != 'MOESI_CMP_directory']
then
    echo "Wrong memory system, expecting clsasic, MI_example, MESI_Two_Level, or MOESI_CMP_directory"
    exit
fi

# $3 is the number of cpus

bms=(bc bfs cc sssp tc pr)

if [! -d results_gapbs_small_$1_$2_$3]; then
     mkdir -p results_gapbs_small_$1_$2_$3;
fi

# synthetic 3

#for bm in "${bms[@]}"
#do
#echo $bm
#build/X86_MESI_Two_Level/gem5.opt -re --outdir results_rate_$1_$2_$3/synthetic_3/$bm/ configs/run_gapbs.py ../vmlinux-4.9.186 ../gapbs.img $1 $3 $2 $bm 1 3 &
#done

# synthetic 15

#for bm in "${bms[@]}"
#do
#echo $bm
#build/X86_MESI_Two_Level/gem5.opt -re --outdir results_rate_$1_$2_$3/synthetic_15/$bm/ configs/run_gapbs.py ../vmlinux-4.9.186 ../gapbs.img $1 $3 $2 $bm 1 15 &
#done

# synthetic 20

for bm in "${bms[@]}"
do
echo $bm
build/X86_MESI_Two_Level/gem5.opt -re --outdir results_gapbs_small_$1_$2_$3/synthetic_22/$bm/ configs-gapbs-small/run_gapbs.py ../vmlinux-4.9.186 ../gapbs.img $1 $3 $2 $bm 1 22 &
done

# real graph

#for bm in "${bms[@]}"
#do
#echo $bm
#build/X86_MESI_Two_Level/gem5.opt -re --outdir results_rate_$1_$2_$3/real_road/$bm/ configs/run_gapbs.py ../vmlinux-4.9.186 ../gapbs.img $1 $3 $2 $bm 0 roadU.sg &
#done
