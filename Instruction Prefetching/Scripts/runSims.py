import subprocess
import os

#Define binary to run on ChampSim
binary = input("Type binary to use: ")
path = '/home/ugrads/a/alexchristian09/ChampSim/ipc1_public'
traces = os.listdir(path)

#Assigns a simulation with a different trace for 50 traces across 50 cores on the CPUs in the CESG Cluster
x = 1
for t in traces:
	subprocess.Popen(["taskset",str(hex(x)),"./run_champsim.sh",binary,"50","50",t])
	x=x+1
