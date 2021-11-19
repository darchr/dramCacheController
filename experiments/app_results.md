# Experiment with Real Applications in SE mode

Works fine with output matching the reference output:

```sh
Bubblesort
Dhrystone
FloatMM
IntMM
Makefile
Oscar
Perm
Quicksort
RealMM
RSBench
Towers
Treesort
XSBench
```

Simulation crashes for the following:

```sh
miniAMR  
miniGMG 
XSBench
```

Assertion failure:

```sh
gem5.opt: build/X86/mem/dcache_ctrl.cc:1025: void DcacheCtrl::processDramReadEvent(): Assertion `packetReady(orbEntry->dccPkt)' failed.
```