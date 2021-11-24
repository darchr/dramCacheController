from __future__ import print_function

import argparse
import m5
from m5.objects import Root
from m5.objects import *

from system import MySystem

'''
This is a simple script to run applications
in SE mode of gem5 with DRAM Cache model

Currently tested with cpu type TimingSimpleCPU, and MinorCPU
'''

parser = argparse.ArgumentParser()

parser.add_argument('dram', type = str, help = "Memory device to use as a dram cache")
parser.add_argument('nvm', type = str, help = "NVM interface to use as main memory")
parser.add_argument('cpu', type = str, help = "CPU type to simulate")
parser.add_argument('binary', type = str, help = "Path to binary to run")

args  = parser.parse_args()


system = MySystem(args.dram, args.nvm, args.cpu)
system.setTestBinary(args.binary)
root = Root(full_system = False, system = system)
m5.instantiate()

exit_event = m5.simulate()

if exit_event.getCause() != 'exiting with last active thread context':
    print("Benchmark failed with bad exit cause.")
    print(exit_event.getCause())
    exit(1)
if exit_event.getCode() != 0:
    print("Benchmark failed with bad exit code.")
    print("Exit code {}".format(exit_event.getCode()))
    exit(1)

print("{} ms".format(m5.curTick()))