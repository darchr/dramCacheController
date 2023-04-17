build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/bc_22    configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/bfs_22   configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/cc_22    configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/pr_22    configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/tc_22    configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/50ns/sssp_22  configs-npb-gapbs/gapbs_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas50_bc
screen -r g_cas50_bfs
screen -r g_cas50_cc
screen -r g_cas50_pr
screen -r g_cas50_tc
screen -r g_cas50_sssp

build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/bc_22    configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/bfs_22   configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/cc_22    configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/pr_22    configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/tc_22    configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/100ns/sssp_22  configs-npb-gapbs/gapbs_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas100_bc
screen -r g_cas100_bfs
screen -r g_cas100_cc
screen -r g_cas100_pr
screen -r g_cas100_tc
screen -r g_cas100_sssp

build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/bc_22    configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/bfs_22   configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/cc_22    configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/pr_22    configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/tc_22    configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/250ns/sssp_22  configs-npb-gapbs/gapbs_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas250_bc
screen -r g_cas250_bfs
screen -r g_cas250_cc
screen -r g_cas250_pr
screen -r g_cas250_tc
screen -r g_cas250_sssp

build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/bc_22    configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/bfs_22   configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/cc_22    configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/pr_22    configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/tc_22    configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/500ns/sssp_22  configs-npb-gapbs/gapbs_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas500_bc
screen -r g_cas500_bfs
screen -r g_cas500_cc
screen -r g_cas500_pr
screen -r g_cas500_tc
screen -r g_cas500_sssp


build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/bc_22    configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/bfs_22   configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/cc_22    configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/pr_22    configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/tc_22    configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/1us/sssp_22  configs-npb-gapbs/gapbs_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas1000_bc
screen -r g_cas1000_bfs
screen -r g_cas1000_cc
screen -r g_cas1000_pr
screen -r g_cas1000_tc
screen -r g_cas1000_sssp

build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/bc_22    configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/bfs_22   configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs   1 22  ../../../cptTest/GAPBS/cpt_100ms_22/bfs_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/cc_22    configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/cc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/pr_22    configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/pr_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/tc_22    configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc    1 22  ../../../cptTest/GAPBS/cpt_100ms_22/tc_22/cpt
build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/GAPBS/2us/sssp_22  configs-npb-gapbs/gapbs_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  sssp  1 22  ../../../cptTest/GAPBS/cpt_100ms_22/sssp_22/cpt

screen -r g_cas5000_bc
screen -r g_cas5000_bfs
screen -r g_cas5000_cc
screen -r g_cas5000_pr
screen -r g_cas5000_tc
screen -r g_cas5000_sssp
