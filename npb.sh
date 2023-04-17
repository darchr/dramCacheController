  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/50ns/bt_C_x   configs-npb-gapbs/npb_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/50ns/cg_C_x   configs-npb-gapbs/npb_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/50ns/is_C_x   configs-npb-gapbs/npb_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/50ns/lu_C_x   configs-npb-gapbs/npb_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/50ns/sp_C_x   configs-npb-gapbs/npb_restore.py 125 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas50_bt
screen -r n_cas50_cg
screen -r n_cas50_is
screen -r n_cas50_lu
screen -r n_cas50_sp


  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/100ns/bt_C_x   configs-npb-gapbs/npb_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/100ns/cg_C_x   configs-npb-gapbs/npb_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/100ns/is_C_x   configs-npb-gapbs/npb_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/100ns/lu_C_x   configs-npb-gapbs/npb_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/100ns/sp_C_x   configs-npb-gapbs/npb_restore.py 250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas100_bt
screen -r n_cas100_cg
screen -r n_cas100_is
screen -r n_cas100_lu
screen -r n_cas100_sp

  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/250ns/bt_C_x   configs-npb-gapbs/npb_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/250ns/cg_C_x   configs-npb-gapbs/npb_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/250ns/is_C_x   configs-npb-gapbs/npb_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/250ns/lu_C_x   configs-npb-gapbs/npb_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/250ns/sp_C_x   configs-npb-gapbs/npb_restore.py 625 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas250_bt
screen -r n_cas250_cg
screen -r n_cas250_is
screen -r n_cas250_lu
screen -r n_cas250_sp

  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/500ns/bt_C_x   configs-npb-gapbs/npb_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/500ns/cg_C_x   configs-npb-gapbs/npb_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/500ns/is_C_x   configs-npb-gapbs/npb_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/500ns/lu_C_x   configs-npb-gapbs/npb_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/500ns/sp_C_x   configs-npb-gapbs/npb_restore.py 1250 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas500_bt
screen -r n_cas500_cg
screen -r n_cas500_is
screen -r n_cas500_lu
screen -r n_cas500_sp

  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/1us/bt_C_x   configs-npb-gapbs/npb_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/1us/cg_C_x   configs-npb-gapbs/npb_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/1us/is_C_x   configs-npb-gapbs/npb_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/1us/lu_C_x   configs-npb-gapbs/npb_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/1us/sp_C_x   configs-npb-gapbs/npb_restore.py 2500 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas1000_bt
screen -r n_cas1000_cg
screen -r n_cas1000_is
screen -r n_cas1000_lu
screen -r n_cas1000_sp

  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/2us/bt_C_x   configs-npb-gapbs/npb_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 ../../../cptTest/NPB/cpt_100ms/bt_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/2us/cg_C_x   configs-npb-gapbs/npb_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 ../../../cptTest/NPB/cpt_100ms/cg_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/2us/is_C_x   configs-npb-gapbs/npb_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 ../../../cptTest/NPB/cpt_100ms/is_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/2us/lu_C_x   configs-npb-gapbs/npb_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 ../../../cptTest/NPB/cpt_100ms/lu_C_x/cpt
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=../results/bypsDDR4/NPB/2us/sp_C_x   configs-npb-gapbs/npb_restore.py 5000 ../../../fullSystemDisksKernel/x86-linux-kernel-4.19.83 ../../../fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 ../../../cptTest/NPB/cpt_100ms/sp_C_x/cpt

screen -r n_cas2000_bt
screen -r n_cas2000_cg
screen -r n_cas2000_is
screen -r n_cas2000_lu
screen -r n_cas2000_sp
