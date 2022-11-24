# ddr4_ddr4

build/NULL/gem5.opt --outdir=results/hbm2/RO_hit_dirty    traffGen_def.py  random  100  1 1 &
build/NULL/gem5.opt --outdir=results/hbm2/RO_hit_clean    traffGen_def.py  random  100  1 0 &
build/NULL/gem5.opt --outdir=results/hbm2/RO_miss_dirty   traffGen_def.py  random  100  0 1 &
build/NULL/gem5.opt --outdir=results/hbm2/RO_miss_clean   traffGen_def.py  random  100  0 0 &
build/NULL/gem5.opt --outdir=results/hbm2/WO_hit_dirty    traffGen_def.py  random  0    1 1 &
build/NULL/gem5.opt --outdir=results/hbm2/WO_hit_clean    traffGen_def.py  random  0    1 0 &
build/NULL/gem5.opt --outdir=results/hbm2/WO_miss_dirty   traffGen_def.py  random  0    0 1 &
build/NULL/gem5.opt --outdir=results/hbm2/WO_miss_clean   traffGen_def.py  random  0    0 0 &
build/NULL/gem5.opt --outdir=results/hbm2/R67_hit_dirty   traffGen_def.py  random  67    1 1 &
build/NULL/gem5.opt --outdir=results/hbm2/R67_hit_clean   traffGen_def.py  random  67    1 0 &
build/NULL/gem5.opt --outdir=results/hbm2/R67_miss_dirty  traffGen_def.py  random  67    0 1 &
build/NULL/gem5.opt --outdir=results/hbm2/R67_miss_clean  traffGen_def.py  random  67    0 0 &
