import subprocess
import os
import shlex

#Define binary to run on ChampSim
binary = input("Type binary to use: ")

path = os.getenv("HOME")+'/InstructionPrefetchers-And-CacheReplacementPolicies/ChampSimL1I/ipc1_public'
traces = os.listdir(path)

#Assigns a simulation with a different trace for 50 traces across 50 cores on the CPUs in the CESG Cluster
x = 1
for t in traces:
    cmd = "taskset "+str(hex(x))+" bin/"+binary+" -warmup_instructions 50000000 -simulation_instructions 50000000 -traces ipc1_public/"+t
    args = shlex.split(cmd)
    with open("results/"+t+"-"+binary+".txt", "w") as outfile:
    #    subprocess.run(args, stdout=outfile)
        subprocess.Popen(args, stdout=outfile)
    x=x+1
