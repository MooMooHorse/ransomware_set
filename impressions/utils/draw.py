import os
import shutil
import sys


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

    # generate size distribution graph (log scale) using stem plot
    sizes = [d[2] for d in data]
    plt.stem(sizes)
    plt.xlabel('File Size (log scale)')
    plt.ylabel('Count')
    plt.title('Size Distribution')
    plt.xscale('log')
    plt.savefig(os.path.join(output_dir_path, f'size_distribution_{index}.png'))
    plt.clf()
    print(f"Generated size_distribution_{index}.png")

    # generate file extension distribution graph
    ext_names = [d[0] for d in data]
    plt.hist(ext_names, bins=50)
    plt.xlabel('File Extension')
    plt.ylabel('Count')
    plt.title('File Extension Distribution')
    plt.savefig(os.path.join(output_dir_path, f'extension_distribution_{index}.png'))
    plt.clf()
    print(f"Generated extension_distribution_{index}.png")

    # generate depth distribution graph
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