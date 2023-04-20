  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/bt_C_x  configs-npb-gapbs/npb_checkpoint.py bt.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/cg_C_x  configs-npb-gapbs/npb_checkpoint.py cg.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/ft_C_x  configs-npb-gapbs/npb_checkpoint.py ft.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/is_C_x  configs-npb-gapbs/npb_checkpoint.py is.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/lu_C_x  configs-npb-gapbs/npb_checkpoint.py lu.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/mg_C_x  configs-npb-gapbs/npb_checkpoint.py mg.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/sp_C_x  configs-npb-gapbs/npb_checkpoint.py sp.C.x 256MiB CascadeLakeNoPartWrs 0 0
  build/X86_MESI_Two_Level/gem5.opt  -re --outdir=chkpt_Apr19/256/NPB/ua_C_x  configs-npb-gapbs/npb_checkpoint.py ua.C.x 256MiB CascadeLakeNoPartWrs 0 0

screen -r n_ckpt2_bt
screen -r n_ckpt2_cg
screen -r n_ckpt2_ft
screen -r n_ckpt2_is
screen -r n_ckpt2_lu
screen -r n_ckpt2_mg
screen -r n_ckpt2_sp
screen -r n_ckpt2_ua