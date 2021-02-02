import subprocess
import os

LRU = "bimodal-no-no-no-no-lru-1core"
SHIP = "bimodal-no-no-no-no-ship++-1core"

path = '/home/ugrads/d/dchapa999/ChampSim/ipc1_public/'
files = os.listdir(path)
"""
i=1
for f in files:
    subprocess.Popen(["taskset", str(hex(i)), "./run_champsim.sh", "bimodal-no-no-no-no-ship-1core", "50", "50", f])
    i = i+1
"""
"""
j=1
for f in files:
    subprocess.Popen(["taskset", str(hex(j)), "./run_champsim.sh", "bimodal-no-no-no-no-drrip-1core", "50", "50", f])
    j = j+1
"""

k=1
for f in files:
    subprocess.Popen(["taskset", str(hex(k)), "./run_champsim.sh", "bimodal-no-no-no-no-ship++-1core", "50", "50", f])
    k = k+1

"""
i=1
for f in files:
    subprocess.check_call(["taskset", str(hex(i)), "./run_champsim.sh", "bimodal-no-no-no-no-lru-1core", "50", "50", f])
    i = i+1

j=1
for f in files:
    subprocess.check_call(["taskset", str(hex(j)), "./run_champsim.sh", "bimodal-no-no-no-no-lru-1core", "50", "50", f])
    j = j+1

#subprocess.check_call(["taskset", "0x1", "./run_champsim.sh", "bimodal-no-no-no-no-lru-1core", "1", "10", "client_001.champsimtrace.xz"])
"""