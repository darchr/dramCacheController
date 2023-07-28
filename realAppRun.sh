# script                              #App    #Policy   #Assoc  #EnableLinkLatency #LinkLatency #EnableBypassDRAM$
# configs-npb-gapbs/restore_both.py   bt.D.x  Rambus    1       0                  0            0

#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/bt      configs-npb-gapbs/restore_both.py   bt.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/cg      configs-npb-gapbs/restore_both.py   cg.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/ft      configs-npb-gapbs/restore_both.py   ft.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/is      configs-npb-gapbs/restore_both.py   is.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/lu      configs-npb-gapbs/restore_both.py   lu.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/mg      configs-npb-gapbs/restore_both.py   mg.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/sp      configs-npb-gapbs/restore_both.py   sp.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/NPB/ua      configs-npb-gapbs/restore_both.py   ua.D.x  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/bc    configs-npb-gapbs/restore_both.py   bc-25   RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/bfs   configs-npb-gapbs/restore_both.py   bfs-25  RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/cc    configs-npb-gapbs/restore_both.py   cc-25   RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/pr    configs-npb-gapbs/restore_both.py   pr-25   RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/tc    configs-npb-gapbs/restore_both.py   tc-25   RambusTagProbOpt  1 0 0 0 &
#build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_85GB_g25_nD/GAPBS/sssp  configs-npb-gapbs/restore_both.py   sssp-25 RambusTagProbOpt  1 0 0 0 &

build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/bt2      configs-npb-gapbs/restore_both.py   bt.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/cg      configs-npb-gapbs/restore_both.py   cg.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/ft      configs-npb-gapbs/restore_both.py   ft.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/is      configs-npb-gapbs/restore_both.py   is.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/lu      configs-npb-gapbs/restore_both.py   lu.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/mg      configs-npb-gapbs/restore_both.py   mg.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/sp2      configs-npb-gapbs/restore_both.py   sp.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/NPB/ua      configs-npb-gapbs/restore_both.py   ua.C.x  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/bc    configs-npb-gapbs/restore_both.py   bc-22   RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/bfs2   configs-npb-gapbs/restore_both.py   bfs-22  RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/cc    configs-npb-gapbs/restore_both.py   cc-22   RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/pr    configs-npb-gapbs/restore_both.py   pr-22   RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/tc2    configs-npb-gapbs/restore_both.py   tc-22   RambusTagProbOpt 1 0 0 0 &
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=newResults/rambusTagPr/1GB_8GB_g22_nC/GAPBS/sssp  configs-npb-gapbs/restore_both.py   sssp-22 RambusTagProbOpt 1 0 0 0 &
