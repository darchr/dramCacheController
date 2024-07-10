#hbm_12_1000_WO_Miss

build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Miss/128 traffGen.py hbm_ddr3 16MiB 128 linear 1000000000000 128MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Miss/256 traffGen.py hbm_ddr3 16MiB 256 linear 1000000000000 128MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Miss/512 traffGen.py hbm_ddr3 16MiB 512 linear 1000000000000 128MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Miss/1024 traffGen.py hbm_ddr3 16MiB 1024 linear 1000000000000 128MiB 1000 0 nvm_2400 &

#hbm_12_1000_WO_Hit

build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Hit/128 traffGen.py hbm_ddr3  128MiB 128 linear 1000000000000  16MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Hit/256 traffGen.py hbm_ddr3  128MiB 256 linear 1000000000000  16MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Hit/512 traffGen.py hbm_ddr3  128MiB 512 linear 1000000000000  16MiB 1000 0 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_WO_Hit/1024 traffGen.py hbm_ddr3 128MiB 1024 linear 1000000000000 16MiB 1000 0 nvm_2400 &

#hbm_12_1000_RO_Hit

build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Hit/128 traffGen.py hbm_ddr3  128MiB 128 linear 1000000000000  16MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Hit/256 traffGen.py hbm_ddr3  128MiB 256 linear 1000000000000  16MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Hit/512 traffGen.py hbm_ddr3  128MiB 512 linear 1000000000000  16MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Hit/1024 traffGen.py hbm_ddr3 128MiB 1024 linear 1000000000000 16MiB 1000 100 nvm_2400 &

#hbm_12_1000_RO_Miss

build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Miss/128 traffGen.py hbm_ddr3 16MiB 128 linear 1000000000000 128MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Miss/256 traffGen.py hbm_ddr3 16MiB 256 linear 1000000000000 128MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Miss/512 traffGen.py hbm_ddr3 16MiB 512 linear 1000000000000 128MiB 1000 100 nvm_2400 &
build/X86/gem5.opt --outdir=hbm_results/hbm_12_1000_RO_Miss/1024 traffGen.py hbm_ddr3 16MiB 1024 linear 1000000000000 128MiB 1000 100 nvm_2400 &
