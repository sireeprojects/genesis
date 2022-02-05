import os
import sys

filelist = []

for file in os.listdir("."):
    if file.endswith(".data"):
        filelist.append(file)

print("Number of files to process: ", len(filelist))

for f in filelist:
    print("Processing", f,)
    cmd = f'python tostr.py {f}'
    os.system(cmd)

print("\nOutput file: cea_structs.h")   
