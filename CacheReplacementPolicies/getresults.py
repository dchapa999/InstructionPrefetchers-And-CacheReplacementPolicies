import os
from statistics import mean
import csv

trace_path = '/home/ugrads/d/dchapa999/ChampSim/ipc1_public'
path = '/home/ugrads/d/dchapa999/ChampSim/results_50M'
files = os.listdir(path)
shippp = []
lru = []

for f in files:
    if (".txt" in f):
        if ("srrip" in f):
            shippp.append(f)
        elif ("lru" in f):
            lru.append(f)

lru_IPCs = []
shippp_IPCs = []
lru_geo = 1
shippp_geo = 1

counter = 0

for fname in lru:
    r = open("results_50M/" + fname, "r")
    lines = r.readlines()
    for line in lines:
        if ("Finished CPU 0 instructions" in line) and ("cumulative IPC:" in line):
            start = line.find("cumulative IPC: ") + 16
            end = line.find("(Simulation") - 1
            print(fname, ": ",line[start:end])
            IPC = float(line[start:end])
            lru_IPCs.append(IPC)
            lru_geo = lru_geo * IPC
            counter = counter + 1
            r.close()
            break

    

for fname in shippp:
    r = open("results_50M/" + fname, "r")
    lines = r.readlines()
    for line in lines:
        if ("Finished CPU 0 instructions" in line) and ("cumulative IPC:" in line):
            start = line.find("cumulative IPC: ") + 16
            end = line.find("(Simulation") - 1
            print(fname, ": ",line[start:end])
            IPC = float(line[start:end])
            shippp_IPCs.append(IPC)
            shippp_geo = shippp_geo * IPC
            r.close()
            break

lru_geo = lru_geo ** (1. / counter)
shippp_geo = shippp_geo ** (1. / counter)
geo_speedup = (shippp_geo/lru_geo)

print("\n<-----------------Results----------------->")

print("Geo Mean Speedup: ",geo_speedup)

print("Geo Mean lru:", lru_geo)
print("Geo Mean ship++:", shippp_geo)

print("Mean lru:", mean(lru_IPCs))
print("Mean ship++:", mean(shippp_IPCs))
speedup = (mean(shippp_IPCs))/(mean(lru_IPCs))
print("Speedup: ",speedup)

print("<-----------------Results----------------->")

traces = os.listdir(trace_path)
traces.sort()

i = 0
rcsv = [[0]*3]*(len(traces))
for t in traces:
    row = []
    row.append(t)
    row.append(lru_IPCs[i])
    row.append(shippp_IPCs[i])
    print(row)
    rcsv[i]=row
    i=i+1

rcsv.insert(0,["Traces", "Baseline IPC", "srrip" + " IPC"])
with open("srrip" + "_results.csv","w+") as my_csv:
    csvWriter = csv.writer(my_csv,delimiter=',')
    csvWriter.writerows(rcsv)