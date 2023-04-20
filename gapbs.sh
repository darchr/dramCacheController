build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/bc    configs-npb-gapbs/gapbs_checkpoint.py bc    512MiB CascadeLakeNoPartWrs 0 0  
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/bfs   configs-npb-gapbs/gapbs_checkpoint.py bfs   512MiB CascadeLakeNoPartWrs 0 0  
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/cc    configs-npb-gapbs/gapbs_checkpoint.py cc    512MiB CascadeLakeNoPartWrs 0 0  
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/pr    configs-npb-gapbs/gapbs_checkpoint.py pr    512MiB CascadeLakeNoPartWrs 0 0  
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/tc    configs-npb-gapbs/gapbs_checkpoint.py tc    512MiB CascadeLakeNoPartWrs 0 0  
build/X86_MESI_Two_Level/gem5.opt -re  --outdir=chkpt_Apr19/25/GAPBS/sssp  configs-npb-gapbs/gapbs_checkpoint.py sssp  512MiB CascadeLakeNoPartWrs 0 0  


screen -r g_ckpt2_bc
screen -r g_ckpt2_bfs
screen -r g_ckpt2_cc
screen -r g_ckpt2_pr
screen -r g_ckpt2_tc
screen -r g_ckpt2_sssp