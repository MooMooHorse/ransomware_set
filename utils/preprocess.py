# preprocessing target system so that we can get 
# the number of files
# the file length distribution
# the file type distribution
# number of users and groups
# for each user and group, we need to know the following:
# The number of files it has read access to
# The number of files it has write access to
# The number of files it has execute access to

# After getting these information, we need to continue preprocessing the target system
# 1. Add magic number 1 at the beginning of each file and at the end of each file in the target system
# 2. add magic number 2 at the beginning and end of the file name and directory name
# 3. Change each byte of the file data to magic number 3

import os
import shutil
import sys

WAIT_TIME_SEC = 0.5

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG, MAGIC_NUM

def add_magic_num_1_3(tar_sys_path):
    """
    Add magic number 1 at the beginning of each file and at the end of each file in the target system
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            data = None
            with open(os.path.join(root, file), "rb") as f:
                data = f.read()
            with open(os.path.join(root, file), "wb") as f:
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_01"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_02"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_03"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_04"]]))
                for byte in data:
                    f.write(bytes([MAGIC_NUM["MAGIC_NUM3"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_01"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_02"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_03"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_04"]]))

def add_magic_num_2(tar_sys_path):
    """
    Add magic number 2 at the beginning and end of the file name and directory name
    """
    magic_num_2_str = f"{MAGIC_NUM['MAGIC_NUM2_01']}{MAGIC_NUM['MAGIC_NUM2_02']}{MAGIC_NUM['MAGIC_NUM2_03']}{MAGIC_NUM['MAGIC_NUM2_04']}"
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            os.rename(os.path.join(root, file), os.path.join(root, magic_num_2_str + file + magic_num_2_str))
        for dir in dirs:
            os.rename(os.path.join(root, dir), os.path.join(root, magic_num_2_str + dir + magic_num_2_str))

def launch_blktrace():
    default_disk = BOOT_CONFIG["default_disk"]
    default_trace_file_path = BOOT_CONFIG["default_trace_file_path"]
    blktrace_dir = BOOT_CONFIG["blktrace_dir"]

    # Create blktrace_dir if not exists
    if not os.path.isdir(blktrace_dir):
        os.mkdir(blktrace_dir)

    import subprocess

    def _launch_blktrace(device, output_file):
        command = ['sudo', 'blktrace', '-d', device, '-o', output_file]
        # get the current path 
        path = os.getcwd()
        process = subprocess.Popen(command, cwd = path)
        # wait for 0.01 seconds to make sure blktrace is launched
        import time
        time.sleep(WAIT_TIME_SEC)
        return process

    blktrace_process_pid = _launch_blktrace(default_disk, default_trace_file_path).pid + 1

    # dump blktrace_process.pid to a file
    with open(f"{blktrace_dir}/blktrace_pid", "w+") as f:
        f.write(f"{blktrace_process_pid}")

def dump_trace_file():
    # first we shut down blktrace
    os.system("sudo pkill blktrace")
    default_trace_file_path = BOOT_CONFIG["default_trace_file_path"]
    blktrace_dir = BOOT_CONFIG["blktrace_dir"]
    import subprocess
    def _launch_blkparse(trace_file, output_file):
        # sudo blkparse -i tracefile -f "%T %S %N %3d\n"
        # os.system('pwd')
        command = ['sudo', 'blkparse', '-i', trace_file, '-f', '%T %S %N %3d\n', '-o', output_file]
        path = os.getcwd()
        process = subprocess.run(command, cwd = path)
    # sleep for 0.01 seconds to make sure blktrace is shut down
    import time
    time.sleep(WAIT_TIME_SEC)
    _launch_blkparse(default_trace_file_path, f"{blktrace_dir}/blkparse_output")
    # now we can remove the trace files
    # os.system(f"sudo rm {default_trace_file_path}*")
    # now we re-launch blktrace
    launch_blktrace()
    

def files_sync(tar_sys_path):
    """
    Sync all the files in target system to disk
    Refer to https://stackoverflow.com/questions/15983272/does-python-have-sync
    """
    current_dir = os.getcwd()
    # print(f"Syncing files in {current_dir} / {tar_sys_path} to disk...")
    # iterate through all the files in the target system and flush them
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            with open(os.path.join(root, file), "rb") as f:
                f.flush()

    # Before we flushed the internal buffers of all the files in the target system, now we need to sync the file system buffers to disk
    
    os.sync()
    os.system("sync")
    # sleep 5 seconds to make sure all the buffers are synced to disk
    # import time
    # time.sleep(5)

def find_magic_1(dev_path,sec_num, bytes, sectors_set, dump_path):
    """
    Given sector number produced by blkparse, we pipe that into dd command and dump the sector information.
    The sector number is given, and we want to dump `bytes` bytes information starting from the sector number.
    By dumping, I mean we scan the sector information and find the magic number 1, if we find it, we increment the counter unencrpyted by 1.
    """
    magic_1 = [MAGIC_NUM["MAGIC_NUM1_01"], MAGIC_NUM["MAGIC_NUM1_02"], MAGIC_NUM["MAGIC_NUM1_03"], MAGIC_NUM["MAGIC_NUM1_04"]]
    unencrpyted = 0 # counter for unencrpyted bytes
    # first we need to find the offset of the sector number
    offset = sec_num * 512
    # then we need to find the offset of the sector number + bytes
    offset_end = offset + bytes
    # now we need to find the sector number of the offset
    sec_num_offset = offset // 512
    # now we need to find the sector number of the offset_end
    sec_num_offset_end = offset_end // 512

    # now we use dd to dump the sector information
    import subprocess
    print(f"Dumping sector information from {sec_num_offset} to {sec_num_offset_end}...")
    command = ['sudo', 'dd', 'if=' + dev_path, 'skip=' + str(sec_num_offset), 'count=' + str(sec_num_offset_end - sec_num_offset), 'status=none']
    
    try:
        output = subprocess.check_output(command)
        # now we need to scan the output and find the magic number 1
        # magic number 1 is 4 bytes
        for i in range(0, len(output) - 3):
            if output[i] == magic_1[0] and output[i + 1] == magic_1[1] and output[i + 2] == magic_1[2] and output[i + 3] == magic_1[3]:
                unencrpyted += 1

        # append the output to dump_path
        with open(dump_path, "ab") as f:
            f.write(output)
    except subprocess.CalledProcessError as e:
        print(e.output)
        sys.exit(1)

    return unencrpyted

def parse_trace_file():
    default_trace_file_path = BOOT_CONFIG["default_trace_file_path"]
    blktrace_dir = BOOT_CONFIG["blktrace_dir"]
    trace_path = f"{blktrace_dir}/blkparse_output"
    dump_path = f"{blktrace_dir}/dump"
    device_path = BOOT_CONFIG["default_disk"]

    # for each legal line the format is as follows:
    # '%T %S %N %3d\n' meaning timestamp, sector number, bytes, Operation Type
    # Time is in seconds operation type 
    # Operation Type : This is a small string containing at least one character ('R' for read, 'W' for write, or 'D' for block discard operation), and optionally either a 'B' (for barrier operations) or 'S' (for synchronous operations).
    # We only care about R and W if neither are present, we ignore the line

    # create empty set for sectors
    sectors_set = set()
    unencrpted = 0

    with open(trace_path, "r") as f:
        lines = f.readlines()
        for line in lines:
            # check if line is legal by using split(' ')
            line = line.split(' ')
            # iterate through the line and remove empty strings
            line = [x for x in line if x != '']
            if len(line) != 4:
                print(f"Line {line} is not legal")
                continue
            # now we need to check if the operation type has R or W
            if 'R' not in line[3] and 'W' not in line[3]:
                print(f"Line {line} is not legal")
                continue
            # print(line[0], line[1], line[2], line[3])
            # now it's legal
            unencrpted += find_magic_1(device_path, int(line[1]), int(line[2]), sectors_set, dump_path)
    print(f"Number of unencrpyted bytes: {unencrpted}")


def preprocess_tar_sys(tar_sys_path):
    """
    Add magic number to the target system
    """
    files_sync(tar_sys_path)
    launch_blktrace()
    add_magic_num_1_3(tar_sys_path)
    add_magic_num_2(tar_sys_path)
    files_sync(tar_sys_path)
    dump_trace_file()
    parse_trace_file()

    