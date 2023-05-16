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

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG, MAGIC_NUM

def add_magic_num_1(tar_sys_path):
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
                f.write(data)
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_01"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_02"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_03"]]))
                f.write(bytes([MAGIC_NUM["MAGIC_NUM1_04"]]))
                f.flush()

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

def add_magic_num_3(tar_sys_path):
    """
    Change each byte of the file data to magic number 3
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            data = None
            with open(os.path.join(root, file), "rb") as f:
                data = f.read()
            with open(os.path.join(root, file), "wb") as f:
                for byte in data:
                    f.write(bytes([MAGIC_NUM["MAGIC_NUM3"]]))
                f.flush()

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
        # sudo blkparse -i tracefile -f "%T %S %3d\n"
        # os.system('pwd')
        command = ['sudo', 'blkparse', '-i', trace_file, '-f', '%T %S %3d\n', '-o', output_file]
        path = os.getcwd()
        process = subprocess.run(command, cwd = path)
    # sleep for 0.2 seconds to make sure blktrace is shut down
    import time
    time.sleep(0.2)
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
    if hasattr(os, 'sync'):
        sync = os.sync
    else:
        import ctypes
        libc = ctypes.CDLL("libc.so.6")
        def sync():
            libc.sync()
    sync()

def preprocess_tar_sys(tar_sys_path):
    """
    Add magic number to the target system
    """
    files_sync(tar_sys_path)
    launch_blktrace()
    add_magic_num_1(tar_sys_path)
    add_magic_num_2(tar_sys_path)
    add_magic_num_3(tar_sys_path)
    files_sync(tar_sys_path)
    dump_trace_file()
    