#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/bt_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8
#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/cg_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8
#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/is_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8
#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/lu_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8
# build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/mg_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level mg.C.x  8
#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/sp_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8
#  build/X86_MESI_Two_Level/gem5.opt -re  --outdir=cpt_100ms/ep_C_x  configs-npb-gapbs/npb_checkpoint.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level ep.C.x  8

   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/bt_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level bt.C.x  8 cpt_100ms/bt_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/cg_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level cg.C.x  8 cpt_100ms/cg_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/ep_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level ep.C.x  8 cpt_100ms/ep_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/is_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level is.C.x  8 cpt_100ms/is_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/lu_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level lu.C.x  8 cpt_100ms/lu_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/mg_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level mg.C.x  8 cpt_100ms/mg_C_x/cpt/ &
   build/X86_MESI_Two_Level/gem5.opt -re  --outdir=results_npb_rst/cptTest/NPB_Ram/rstr/sp_C_x   configs-npb-gapbs/npb_restore.py /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83 /home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-npb timing MESI_Two_Level sp.C.x  8 cpt_100ms/sp_C_x/cpt/ &