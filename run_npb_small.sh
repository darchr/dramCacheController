build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/bt.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level bt.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/ep.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level ep.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/ft.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level ft.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/sp.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level sp.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/cg.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level cg.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/is.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level is.C.x 1 &
build/X86_MESI_Two_Level/gem5.opt -re --outdir=small-npb/lu.C configs-npb-small/run_npb.py ../vmlinux-4.9.186 ../npb.img timing MESI_Two_Level lu.C.x 1 &
