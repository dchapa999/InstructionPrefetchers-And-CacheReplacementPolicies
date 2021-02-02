import os
from statistics import mean
import csv

new_prefetcher = input("What prefetcher are you checking?\n")

trace_path = '/home/ugrads/a/alexchristian09/ChampSim/ipc1_public'
traces = os.listdir(trace_path)
path = '/home/ugrads/a/alexchristian09/ChampSim/results_50M'
files = os.listdir(path)
new = []
baseline = []

traces.sort()

for f in files:
    if (".txt" in f):
        if (new_prefetcher in f):
            new.append(f)
        elif ("no-no-no-no" in f):
            baseline.append(f)

new.sort()
baseline.sort()
baseline_IPCs = []
baseline_BPaccs = []
baseline_MPKIs = []
new_IPCs = []
new_BPaccs = []
new_MPKIs = []

for fname in baseline:
    r = open("results_50M/" + fname, "r")
    lines = r.readlines()
    for line in lines:
        if ("CPU 0 cumulative IPC:" in line):
            start = line.find("cumulative IPC: ") + 16
            end = line.find(" instructions")
            #print(fname, ": ",line[start:end])
            IPC = float(line[start:end])
            baseline_IPCs.append(IPC)
        elif ("CPU 0 Branch Prediction Accuracy:" in line):
            start = line.find("Accuracy: ")+10
            end = line.find("%")
            BPacc = float(line[start:end])
            baseline_BPaccs.append(BPacc)
            start2 = line.find("MPKI: ")+6
            end2 = line.find(" Average")
            MPKI = float(line[start:end])
            baseline_MPKIs.append(MPKI)
    r.close()

    

for fname in new:
    r = open("results_50M/" + fname, "r")
    lines = r.readlines()
    for line in lines:
        if ("CPU 0 cumulative IPC:" in line):
            start = line.find("cumulative IPC: ") + 16
            end = line.find(" instructions")
            #print(fname, ": ",line[start:end])
            IPC = float(line[start:end])
            new_IPCs.append(IPC)
        elif ("CPU 0 Branch Prediction Accuracy:" in line):
            start = line.find("Accuracy: ")+10
            end = line.find("%")
            BPacc = float(line[start:end])
            new_BPaccs.append(BPacc)
            start2 = line.find("MPKI: ")+6
            end2 = line.find(" Average")
            MPKI = float(line[start:end])
            new_MPKIs.append(MPKI)
    r.close()

i = 0
rcsv = [[0]*3]*(len(traces))
for t in traces:
    row = []
    row.append(t)
    row.append(baseline_IPCs[i])
    row.append(new_IPCs[i])
    print(row)
    rcsv[i]=row
    i=i+1

rcsv.insert(0,["Traces", "Baseline IPC", new_prefetcher + " IPC"])
with open(new_prefetcher + "_results.csv","w+") as my_csv:
    csvWriter = csv.writer(my_csv,delimiter=',')
    csvWriter.writerows(rcsv)
#print(new_IPCs)
#print(baseline_IPCs)
speedup = (mean(new_IPCs))/(mean(baseline_IPCs))
#print("Baseline MPKI: ",mean(baseline_MPKIs),"\nNew MPKI: ",mean(new_MPKIs))
#print("Baseline Branch Predictor Accuracy: ",mean(baseline_BPaccs),"\nNew Branch Predictor Accuracy: ",mean(new_BPaccs))
print("Baseline IPC: ",mean(baseline_IPCs),"\nNew IPC: ",mean(new_IPCs))
print("Speedup: ",speedup)

