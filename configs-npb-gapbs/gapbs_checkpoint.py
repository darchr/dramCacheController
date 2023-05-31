# -*- coding: utf-8 -*-
# Copyright (c) 2019 The Regents of the University of California.
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Jason Lowe-Power, Ayaz Akram

""" Script to run GAP Benchmark suites workloads.
    The workloads have two modes: synthetic and real graphs.
"""
import argparse
import time
import m5
import m5.ticks
from m5.objects import *

from system import *

supported_protocols = ["MESI_Two_Level"]
supported_cpu_types = ['kvm', 'atomic', 'timing']

def writeBenchScript(dir, benchmark_name, size, synthetic):
    """
    This method creates a script in dir which will be eventually
    passed to the simulated system (to run a specific benchmark
    at bootup).
    """
    input_file_name = '{}/run_{}_{}'.format(dir, benchmark_name, size)
    if (synthetic):
        with open(input_file_name,"w") as f:
            f.write('./{} -g {}\n'.format(benchmark_name, size))
    elif(synthetic==0):
        with open(input_file_name,"w") as f:
            # The workloads that are copied to the disk image using Packer
            # should be located in /home/gem5/.
            # Since the command running the workload will be executed with
            # pwd = /home/gem5/gapbs, the path to the copied workload is
            # ../{workload-name}
            f.write('./{} -sf ../{}'.format(benchmark_name, size))

    return input_file_name

def parse_options():

    parser = argparse.ArgumentParser(description='For use with gem5. This '
                'runs a GAPBS applications. This only works '
                'with x86 ISA.')

    # The manditry position arguments.
    parser.add_argument("benchmark", type=str,
                        help="The GAPBS application to run")
    parser.add_argument("graph", type=str,
                        help="The GAPBS application to run")
    parser.add_argument("dcache_policy", type=str,
                        help="The architecture of DRAM cache: "
                        "CascadeLakeNoPartWrs, Oracle, BearWriteOpt, Rambus")
    parser.add_argument("is_link", type=int,
                        help="whether to use a link for backing store or not")
    parser.add_argument("link_lat", type=str,
                        help="latency of the link to backing store")
    return parser.parse_args()


if __name__ == "__m5_main__":
    args = parse_options()

    kernel = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-linux-kernel-4.19.83"
    disk = "/home/babaie/projects/ispass2023/runs/hbmCtrlrTest/dramCacheController/fullSystemDisksKernel/x86-gapbs"
    num_cpus = 8
    cpu_type = "Timing"
    mem_sys = "MESI_Two_Level"
    synthetic = 1

    dcache_size = ""
    mem_size = ""
    if args.graph == "22":
        dcache_size = "512MiB"
        mem_size = "8GiB"
    elif args.graph == "25":
        dcache_size = "512MiB"
        mem_size = "85GiB"

    # create the system we are going to simulate
    system = MyRubySystem(kernel, disk, mem_sys, num_cpus, dcache_size, mem_size, args.dcache_policy,
                            args.is_link, args.link_lat, args)

    system.m5ops_base = 0xffff0000

    # Exit from guest on workbegin/workend
    system.exit_on_work_items = True

    # Create and pass a script to the simulated system to run the reuired
    # benchmark
    system.readfile = writeBenchScript(m5.options.outdir, args.benchmark, args.graph, synthetic)

    # set up the root SimObject and start the simulation
    root = Root(full_system = True, system = system)

    if system.getHostParallel():
        # Required for running kvm on multiple host cores.
        # Uses gem5's parallel event queue feature
        # Note: The simulator is quite picky about this number!
        root.sim_quantum = int(1e9) # 1 ms

    # needed for long running jobs
    # m5.disableAllListeners()

    # instantiate all of the objects we've created above
    m5.instantiate()

    globalStart = time.time()

    print("Running the simulation")
    print("Using cpu: {}".format(cpu_type))
    exit_event = m5.simulate()

    if exit_event.getCause() == "workbegin":
        print("Done booting Linux")
        # Reached the start of ROI
        # start of ROI is marked by an
        # m5_work_begin() call
        print("Resetting stats at the start of ROI!")
        m5.stats.reset()
        start_tick = m5.curTick()
        start_insts = system.totalInsts()
        # switching CPU to timing
        system.switchCpus(system.cpu, system.timingCpu)
    else:
        print("Unexpected termination of simulation !")
        exit()

    m5.stats.reset()
    print("After reset ************************************************ statring smiulation:\n")
    for interval_number in range(10):
        print("Interval number: {} \n".format(interval_number))
        exit_event = m5.simulate(100000000000)
        m5.stats.dump()
        if (exit_event.getCause() == "cacheIsWarmedup") :
            print("Caught cacheIsWarmedup exit event!")
            break
        print("-------------------------------------------------------------------")

    print("After sim ************************************************ End of warm-up \n")
    m5.stats.dump()
    #system.switchCpus(system.timingCpu, system.o3Cpu)
    #m5.checkpoint(m5.options.outdir + '/cpt')
