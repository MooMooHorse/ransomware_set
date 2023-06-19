import matplotlib.pyplot as plt
import numpy as np

import os
import shutil
import sys

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

def plot_trace(trace_path):
    """
    For each line in the parsed file, the format is as follows:
    time(ns) sector_number bytes operation_type device_number(major, minor)
    operation_type : RWBS field, a (small) string (1-3 characters) 
    
    This is a small string containing at least one character ('R' for
       read, 'W' for write, or 'D' for block discard operation), and
       optionally either a 'B' (for barrier operations) or 'S' (for
       synchronous operations).
    """
    # read the trace file
    with open(trace_path, 'r') as f:
        lines = f.readlines()
        # parse the trace file
        # time, sector_number, bytes, operation_type, device_number
        for line in lines:
            line = line.strip()
            if not line:
                continue
            line = line.split(' ')
            line = [x for x in line if x != '']
            if (len(line) != 5) or ('R' not in line[3] and 'W' not in line[3]):
                continue
            timeNs = int(line[0])
            timeUs = timeNs / 1000
            sector_number = int(line[1])
            bytes = int(line[2])
            operation_type = line[3]
            device_number = int(line[4])
            