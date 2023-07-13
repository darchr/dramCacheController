import m5
import argparse
from m5.objects import *
from system import *

"""
Usage:
------

```
./build/X86/gem5.opt \
    drtrace.py \
    --path <path of folder containing all traces> \
    --workload <benchmark_name> \
    --players <Number of players to use> \
    --dram <DRAM device to use>
```
"""

parser = argparse.ArgumentParser(
    description="A script to run google traces."
)

benchmark_choices = ["charlie", "delta", "merced", "whiskey"]

parser.add_argument(
    "--path",
    type=str,
    required=True,
    help="Main directory containing the traces.",
)

parser.add_argument(
    "--workload",
    type=str,
    required=True,
    help="Input the benchmark program to execute.",
    choices=benchmark_choices,
)

parser.add_argument(
    "--players",
    type=int,
    required=True,
    help="Input the number of players to use.",
)

parser.add_argument(
    "--dram",
    type = str,
    help = "Memory device to use"
)

args = parser.parse_args()


MemTypes = {
    'ddr4_2400' : DDR4_2400_16x4,
    'hbm_2000' : HBM_2000_4H_1x64,
}

#system = System()
system = MyRubySystem("MESI_Two_Level", args.players, args)

root = Root(full_system=True, system=system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(100000000000)
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
