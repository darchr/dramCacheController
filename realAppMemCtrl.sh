  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/bt_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 &
  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/cg_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 &
  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/ep_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level ep.C.x  8 &
  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/is_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 &
  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/lu_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 &
# build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/mg_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level mg.C.x  8 &
  build/X86_MESI_Two_Level/gem5.opt --outdir=results/NPB/Nov14/set3/sp_C_x configs-npb-small/run_npb.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 &

build/X86_MESI_Two_Level/gem5.opt --outdir=results/GAPBS/Nov14/set3/bc_22  configs-gapbs-small/run_gapbs.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bc   1 22 &
build/X86_MESI_Two_Level/gem5.opt --outdir=results/GAPBS/Nov14/set3/bfs_22 configs-gapbs-small/run_gapbs.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  bfs  1 22 &
build/X86_MESI_Two_Level/gem5.opt --outdir=results/GAPBS/Nov14/set3/cc_22  configs-gapbs-small/run_gapbs.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  cc   1 22 &
build/X86_MESI_Two_Level/gem5.opt --outdir=results/GAPBS/Nov14/set3/pr_22  configs-gapbs-small/run_gapbs.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  pr   1 22 &
build/X86_MESI_Two_Level/gem5.opt --outdir=results/GAPBS/Nov14/set3/tc_22  configs-gapbs-small/run_gapbs.py fullSystemDisksKernel/x86-linux-kernel-4.19.83 fullSystemDisksKernel/x86-gapbs  timing 8 MESI_Two_Level  tc   1 22 &