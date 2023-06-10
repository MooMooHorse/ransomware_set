import os
import shutil
import sys
import numpy as np
from scipy.interpolate import interp1d


def handle_flags():
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            draw_dir = arg.split('=')
            path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            if(len(draw_dir) == 1):
                # get current file path
                draw_dir = path + '/stat'
            else:
                draw_dir = path + '/' + draw_dir[1].strip()
            if os.path.exists(draw_dir):
                shutil.rmtree(draw_dir)
        elif arg.startswith("--help") or arg.startswith("-h"):
            print("Usage: python3 draw.py [--clean[=draw_dir]] [--help, -h]")
            print("Options:")
            print("--clean[=draw_dir]    clean the draw_dir")
            print("--help, -h            show this help message and exit")
            print("default : read raw data from logs/ and draw figures in stat/")
            sys.exit(0)
        else:
            print("Invalid option: {}".format(arg))
            print("Please use --help or -h to see the usage.")
            
import os
import matplotlib.pyplot as plt

def generate_graphs(log_file_path, output_dir_path, index):
    print(f"Generating graphs for {log_file_path} Output dir: {output_dir_path} Index: {index}")
    # read the log file
    with open(log_file_path, 'r') as f:
        lines = f.readlines()

    # extract the data from the log file
    data = []
    for line in lines:
        parts = line.strip().split()
        ext_name = os.path.splitext(parts[4])[1][1:]
        ext_num = int(parts[1])
        size = float(parts[2])
        depth = int(parts[3])
        data.append((ext_name, ext_num, size, depth))

    # generate size distribution graph (log scale) using plot
    # x axis is size in log scale, y axis is count
    # We plot it smoothly without using histogram
    sizes = np.array([d[2] for d in data])
    
    # Define the size intervals as powers of 2
    size_intervals = [2**k for k in range(0, int(np.log2(max(sizes))) + 1)]

    # Calculate the counts for each size interval
    counts = []
    prev_size = 0
    for size in size_intervals:
        count = np.sum((sizes > prev_size) & (sizes <= size))
        counts.append(count)
        prev_size = size

    # Create the plot
    plt.figure(figsize=(8, 6))
    plt.plot(size_intervals, counts, marker='o', linestyle='-')

    plt.xscale('log')  # Set logarithmic scale on the x-axis


    # Set the custom tick locations and labels
    tick_locations = [8, 10**3, 10**4, 10**5, 10**6, 10**7, 10**8, 10**9, 10**10, 10**11]
    tick_labels = ['8 Bytes', '1KB', '10KB' ,'100KB','1 MB', '10 MB', '100 MB' , '1 GB', '10 GB', '100 GB']
    
    # check the maximum size is less then any of the tick locations, if so, remove the last tick location
    for i in range(len(tick_locations)):
        if tick_locations[i] > 2 * max(sizes):
            tick_locations = tick_locations[:i]
            tick_labels = tick_labels[:i]
            break
    
    # Set the custom tick locations and labels on the x-axis
    ax = plt.gca()
    ax.set_xticks(tick_locations)
    ax.set_xticklabels(tick_labels)
    
    plt.xlabel('File Size (log scale)')
    plt.ylabel('Count')
    plt.title('Size Distribution')
    plt.savefig(os.path.join(output_dir_path, f'size_distribution_{index}.png'))
    plt.clf()
    print(f"Generated size_distribution_{index}.png")
    
    extensions = [
        "NUL",
        "TXT",
        "JPG",
        "EXE",
        "CPP",
        "HTM",
        "H",
        "DLL",
        "GIF",
        "PDB",
        "PST",
        "PCH",
        "MP3",
        "LIB",
        "WMA",
        "VHD",
        "PDF",
        "MPG",
        "OTHER"  # all files falling in the tail
    ]
    ext_names = [extensions[d[1]] for d in data]
    print(len(set(ext_names)))
    ext_counts = {}
    for ext in ext_names:
        if ext in ext_counts:
            ext_counts[ext] += 1
        else:
            ext_counts[ext] = 1
    sorted_ext_counts = sorted(ext_counts.items(), key=lambda x: x[1], reverse=True)
    ext_names = [x[0] for x in sorted_ext_counts]
    ext_counts = [x[1] for x in sorted_ext_counts]
    colors = plt.cm.Set3([i/float(len(ext_names)) for i in range(len(ext_names))])
    plt.barh(range(len(ext_names)), ext_counts, color=colors)
    plt.yticks(range(len(ext_names)), ext_names)
    plt.xlabel('Count')
    plt.title('File Extension Distribution')
    plt.savefig(os.path.join(output_dir_path, f'extension_distribution_{index}.png'))
    plt.clf()
    print(f"Generated extension_distribution_{index}.png")

    # generate depth distribution graph using histogram
    depths = [d[3] for d in data]
    plt.hist(depths, bins=50)
    plt.xlabel('File Depth')
    plt.ylabel('Count')
    plt.title('Depth Distribution')
    plt.savefig(os.path.join(output_dir_path, f'depth_distribution_{index}.png'))
    plt.clf()
    print(f"Generated depth_distribution_{index}.png")
    
    

handle_flags()

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
output_dir_path = os.path.join(path, 'stat')
# create the output directory if it does not exist  
if not os.path.exists(output_dir_path):
    os.makedirs(output_dir_path)

actual_logfile = f"{path}/logs" # log directory
# for all log files in the log directory (walk through the directory)
for root, dirs, files in os.walk(actual_logfile):
    for file in files:
        if file.startswith("log"):
            log_file_path = os.path.join(root, file)
            index = file.split('-')[1]
            generate_graphs(log_file_path, output_dir_path, index)