import pandas as pd
import numpy as np
import sys
from matplotlib import pyplot as plt

import os

datadir = '/home/babaie/projects/refreshReprod'


def getStat(filename, stat):
    #filename = os.path.join(filename).replace('\\','/')
    #print(stat)
    #print(filename)
    try:
        with open(filename) as f:
            readlines = f.readlines()
            for l in readlines:
                if stat in l:
                    return l
            return 0.0 #for cases where stat was not found
    except: #for cases where the file was not found
        return 0.0



Stats = ['simTicks',
         'hostSeconds',
         'system.generator.numPackets',
         'system.generator.avgReadLatency',
         'system.generator.avgWriteLatency',
         'system.generator.readBW',
         'system.generator.writeBW',
         'system.mem_ctrl.numHits',
         'system.mem_ctrl.numMisses',
         'system.mem_ctrl.numRdHits',
         'system.mem_ctrl.numWrHits',
         'system.mem_ctrl.numRdMisses',
         'system.mem_ctrl.numWrMisses',
         'system.mem_ctrl.numColdMisses',
         'system.mem_ctrl.numHotMisses',
         'system.mem_ctrl.numWrBacks',
         'system.mem_ctrl.totNumConf',
         'system.mem_ctrl.totNumConfBufFull',
         'system.mem_ctrl.timeInDramRead',
         'system.mem_ctrl.timeInDramWrite',
         'system.mem_ctrl.timeInWaitingToIssueNvmRead',
         'system.mem_ctrl.timeInNvmRead',
         'system.mem_ctrl.timeInNvmWrite',
         'system.mem_ctrl.drRdQingTime',
         'system.mem_ctrl.drWrQingTime',
         'system.mem_ctrl.nvmRdQingTime',
         'system.mem_ctrl.nvmWrQingTime',
         'system.mem_ctrl.drRdDevTime',
         'system.mem_ctrl.drWrDevTime',
         'system.mem_ctrl.nvRdDevTime',
         'system.mem_ctrl.nvWrDevTime',
         'system.mem_ctrl.totNumPktsDrRd',
         'system.mem_ctrl.totNumPktsDrWr',
         'system.mem_ctrl.totNumPktsNvmRdWait',
         'system.mem_ctrl.totNumPktsNvmRd',
         'system.mem_ctrl.totNumPktsNvmWr',
         'system.mem_ctrl.maxNumConf',
         'system.mem_ctrl.maxDrRdEvQ',
         'system.mem_ctrl.maxDrRdRespEvQ',
         'system.mem_ctrl.maxDrWrEvQ',
         'system.mem_ctrl.maxNvRdIssEvQ',
         'system.mem_ctrl.maxNvRdEvQ',
         'system.mem_ctrl.maxNvRdRespEvQ',
         'system.mem_ctrl.maxNvWrEvQ',
         'system.mem_ctrl.dram.avgRdBW',     
         'system.mem_ctrl.dram.avgWrBW',     
         'system.mem_ctrl.dram.peakBW',      
         'system.mem_ctrl.dram.busUtil',     
         'system.mem_ctrl.dram.busUtilRead', 
         'system.mem_ctrl.dram.busUtilWrite',
         'system.mem_ctrl.nvm.avgRdBW',
         'system.mem_ctrl.nvm.avgWrBW',
         'system.mem_ctrl.nvm.peakBW',
         'system.mem_ctrl.nvm.busUtil',   
         'system.mem_ctrl.nvm.busUtilRead',
         'system.mem_ctrl.nvm.busUtilWrite',
         'system.mem_ctrl.dram.avgBusLat',
         'system.mem_ctrl.nvm.avgBusLat']
         


devices = ['ddr4']
duration = ['12']
pattern = ['R', 'L']
rd_prct = ['RO', 'WO', 'R67']
hit_miss = ['Hit', 'Miss']
orb_size = ['1', '2', '4', '8', '16', '32', '64', '128', '256', '512', '1024']

rows = []
for dev in devices:
    for dur in duration:
        for pat in pattern:
            for rds in rd_prct:
                for hm in hit_miss:
                    for orb in orb_size:
                        stats = [dev+' '+dur+' '+pat+' '+rds+' '+hm, orb]
                        for stat in Stats:
                            time_file_path = '{}/{}_{}_{}_{}_{}/{}/stats.txt'.format(datadir,dev, dur, pat, rds, hm, orb)
                            ret_line = getStat(time_file_path,stat)
                        
                            if ret_line != 0:
                                #if ret_line=='nan' :
                                #    stat_val = 0
                                #else:
                                stat_val = ret_line.split()[1]
                            else:
                                stat_val = -1
                            stats.append(stat_val)
        
                        rows.append(stats)

#print(rows)
df = pd.DataFrame(rows, columns=[
         'run',
         'orbSize',
         'simTicks',
         'hostSeconds',
         'numPackets',
         'avgReadLatency',
         'avgWriteLatency',
         'readBW',
         'writeBW',
         'numHits',
         'numMisses',
         'numRdHits',
         'numWrHits',
         'numRdMisses',
         'numWrMisses',
         'numColdMisses',
         'numHotMisses',
         'numWrBacks',
         'totNumConf',
         'totNumConfBufFull',
         'timeInDramRead',
         'timeInDramWrite',
         'timeInWaitingToIssueNvmRead',
         'timeInNvmRead',
         'timeInNvmWrite',
         'drRdQingTime',
         'drWrQingTime',
         'nvmRdQingTime',
         'nvmWrQingTime',
         'drRdDevTime',
         'drWrDevTime',
         'nvRdDevTime',
         'nvWrDevTime',
         'totNumPktsDrRd',
         'totNumPktsDrWr',
         'totNumPktsNvmRdWait',
         'totNumPktsNvmRd',
         'totNumPktsNvmWr',
         'maxNumConf',
         'maxDrRdEvQ',
         'maxDrRdRespEvQ',
         'maxDrWrEvQ',
         'maxNvRdIssEvQ',
         'maxNvRdEvQ',
         'maxNvRdRespEvQ',
         'maxNvWrEvQ',
         'dram.avgRdBW',     
         'dram.avgWrBW',     
         'dram.peakBW',      
         'dram.busUtil',     
         'dram.busUtilRead', 
         'dram.busUtilWrite',
         'nvm.avgRdBW',
         'nvm.avgWrBW',
         'nvm.peakBW',
         'nvm.busUtil',   
         'nvm.busUtilRead',
         'nvm.busUtilWrite',
         'dram.avgBusLat',
         'nvm.avgBusLat'])

df['totBW'] = (df['readBW'].astype(float) + df['writeBW'].astype(float))/1000000000
df['ARL'] = (df['avgReadLatency'].astype(float))/1000
df['AWL'] = (df['avgWriteLatency'].astype(float))/1000
df['totBusUtil%'] = df['dram.busUtil'].astype(float) + df['nvm.busUtil'].astype(float)

df['avgDramRdQingTime'] = (df['drRdQingTime'].astype(float) / df['totNumPktsDrRd'].astype(float))/1000
df['avgDramWrQingTime'] = (df['drWrQingTime'].astype(float) / df['totNumPktsDrWr'].astype(float))/1000
df['avgNvmRdQingTime'] = (df['nvmRdQingTime'].astype(float) / df['totNumPktsNvmRd'].astype(float))/1000
df['avgNvmWrQingTime'] = (df['nvmWrQingTime'].astype(float) / df['totNumPktsNvmWr'].astype(float))/1000


df['missRatio'] = df['numMisses'].astype(float) / df['numPackets'].astype(float)*100
df['HitRatio'] = df['numHits'].astype(float) / df['numPackets'].astype(float)*100

df.to_csv("res_cs2.csv")