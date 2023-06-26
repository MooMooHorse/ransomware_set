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

WAIT_TIME_SEC = 1

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG, MAGIC_NUM, PATHS, LOG_NAME
from config import print_red

def add_magic_num_3(tar_sys_path, sync = False):
    """
    Add magic number 3 to replace all the contents of the file
    Reeturn : the number of files that have been synced
    """
    synced_num = 0
    processed_files = 0
    magic_3 = MAGIC_NUM["MAGIC_NUM3"]
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            # get length of the file
            length = os.path.getsize(os.path.join(root, file))
            with open(os.path.join(root, file), "r+b") as f:
                # write length bytes of magic number 3
                f.write(bytes([magic_3]) * length)
                # fsync the file if sync is enabled
                if sync:
                    f.flush()
                    os.fsync(f.fileno())
                    synced_num += 1
                processed_files += 1
                    
    return synced_num, processed_files

def add_magic_num_1_2(tar_sys_path):
    """
    Add magic number 1 at the beginning of the file name and directory name
    Add magic number 2 at the end of the file name and directory name
    """
    magic_num_1_str = ''.join([chr(MAGIC_NUM["MAGIC_NUM1_01"]), chr(MAGIC_NUM["MAGIC_NUM1_02"]),
                               chr(MAGIC_NUM["MAGIC_NUM1_03"]), chr(MAGIC_NUM["MAGIC_NUM1_04"])])
    magic_num_2_str = ''.join([chr(MAGIC_NUM["MAGIC_NUM2_01"]), chr(MAGIC_NUM["MAGIC_NUM2_02"]),
                                 chr(MAGIC_NUM["MAGIC_NUM2_03"]), chr(MAGIC_NUM["MAGIC_NUM2_04"])])
    # print(f"magic_num_1_str: {magic_num_1_str}")
    # print(f"magic_num_2_str: {magic_num_2_str}")
    # big_magic = ((ord(magic_num_1_str[0]) << 24) + (ord(magic_num_1_str[1]) << 16) +
    #              (ord(magic_num_1_str[2]) << 8) + ord(magic_num_1_str[3]))
    # print(f"big_magic: {big_magic}")
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            os.rename(os.path.join(root, file), os.path.join(root, magic_num_1_str + file + magic_num_2_str))
        for dir in dirs:
            os.rename(os.path.join(root, dir), os.path.join(root, magic_num_1_str + dir + magic_num_2_str))

def launch_blktrace(blktrace_dir, device_list):
    """
    Launch blktrace and dump the trace to blktrace_dir.
    Assumption (caller needs to make sure), blktrace_dir is an absolute path that exists
    """
    import subprocess
    # because of the stupid setting of blktrace, 
    # the tracefiles will be in current directory, so  we need to change directory to blktrace_dir
    # cur_path = os.getcwd()
    # os.chdir(blktrace_dir)

    # os.system("echo 3 | sudo tee /proc/sys/vm/drop_caches")

    command = ['sudo', 'blktrace']
    
    for device in device_list:
        command.append('-d')
        command.append(device)

    print(command)
    
    process = subprocess.Popen(command, cwd = blktrace_dir)
    # wait for 0.01 seconds to make sure blktrace is launched
    import time
    time.sleep(WAIT_TIME_SEC)
    
    # os.chdir(cur_path)

def dump_trace_file(blktrace_dir, devices):
    """
    Cloase the blktrace program and dump the trace to output file.
    The output will be in log director in file name blktrace_{test_id}.
    For each line in the parsed file, the format is as follows:
    time(ns) sector_number bytes operation_type device_number(major, minor)
    operation_type : RWBS field, a (small) string (1-3 characters) 
    
    This is a small string containing at least one character ('R' for
       read, 'W' for write, or 'D' for block discard operation), and
       optionally either a 'B' (for barrier operations) or 'S' (for
       synchronous operations).
    Assumption :
        The log directory exists
    """
    import subprocess
    if not os.path.exists(PATHS["log_dir"]):
        print_red(f"Log directory {PATHS['log_dir']} doesn't exist")
        sys.exit(1)

    test_id_path = PATHS["test_id_path"]
    with open(test_id_path, "r") as f:
        test_id = int(f.read())

    import logging 


    output_file = os.path.join(PATHS["log_dir"], f'logs_{test_id}', f'blktrace_{test_id}.log')

    with open(PATHS["trace_path_file"], "w") as f:
        f.write(output_file)
        
    cur_path = os.getcwd()
    os.chdir(blktrace_dir)
    # first we shut down blktrace
    os.system("sudo pkill blktrace")

    # sleep for 1 second to make sure blktrace is shut down
    import time
    time.sleep(WAIT_TIME_SEC)
    
    command = ['sudo', 'blkparse']
    
    for device in  devices:
        command = command + [device.split('/')[2]]
    
    command = command + ['-f', '%T.%t %S %N %3d %D\n', '-o', output_file]
    print(command, blktrace_dir)
    # with open(os.path.join(PATHS["log_dir"], f'logs_{test_id}', LOG_NAME["parse_err"]), "w") as f:
    process = subprocess.run(command, cwd = blktrace_dir)
    
    
    os.chdir(cur_path)
    

def files_sync(tar_sys_path):
    """
    Sync all the files in target system to disk
    Refer to https://stackoverflow.com/questions/15983272/does-python-have-sync
    """
    # iterate through all the files in the target system, do fsync for each file
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            with open(os.path.join(root, file), "r") as f:
                os.fsync(f.fileno())
    # os.system("echo 3 | sudo tee /proc/sys/vm/drop_caches")




def preprocess_tar_sys(tar_sys_path, blktrace_dir, device):
    """
    Add magic number to the target system
    """
    dye_info_path = LOG_NAME['dye_info']
    # files_sync(tar_sys_path)
    # launch_blktrace(blktrace_dir, device)
    add_magic_num_1_2(tar_sys_path)
    synced_files, processed_files = add_magic_num_3(tar_sys_path, sync = True)
    with open(dye_info_path, "w") as f:
        f.write(f"dye path : {tar_sys_path}\n")
        f.write(f"Synced files: {synced_files}\n")
        f.write(f"Processed files: {processed_files}\n")
    # files_sync(tar_sys_path)
    # inject_debug_file(tar_sys_path)
    # dump_trace_file(blktrace_dir) # it will implicitly close the blktrace
    # parse_trace_file(blktrace_dir)
    
def preprocess_backup_dir(tar_sys_path, blktrace_dir, device):
    """legacy code, abandoned
    """
    return
    files_sync(tar_sys_path)
    launch_blktrace(blktrace_dir, device)
    add_magic_num_3(tar_sys_path)
    add_magic_num_1_2(tar_sys_path)
    files_sync(tar_sys_path)
    dump_trace_file(blktrace_dir) # it will implicitly close the blktrace

def _run_ransomware(blktrace_dir):
    """
    Ransomware should be an executable file in the ransomware directory.
    It will only encrypte the files in the target system.
    """
    import subprocess
    cmd = 'python3 ' + PATHS['rans_exec_path'] + ' -mode=0'
    print(f"Running ransomware... with command {cmd}")
    os.system(cmd)
    # rans_proc = subprocess.Popen(cmd.split())
    # Launch strace with the ransomware command as a child process
    # rans_pid = rans_proc.pid
    # dbg_fname = f"{blktrace_dir}/strace.log"
    # strace_cmd = ["sudo","strace", "-o", dbg_fname, "-p", f"{rans_pid}"]
    # strace_proc = subprocess.Popen(strace_cmd)
    # Wait for the ransomware process to finish and obtain its exit status
    # exit_status = rans_proc.wait()
    # Print the exit status
    # print(f"Ransomware process exited with status {exit_status}")
    # Terminate the strace process
    # subprocess.run(["sudo", "pkill", "strace"])

def run_ransomware(tar_sys_path, blktrace_dir, device_list):
    """
    Enable the blk trace, run ransomware, then close the blk trace
    """
    
    launch_blktrace(blktrace_dir, device_list)
    _run_ransomware(blktrace_dir)
    dump_trace_file(blktrace_dir, device_list) # it will implicitly close the blktrace

def main():
    """
    This file mostly is called by toplevel, it's also called by C++ core framework.
    When the latter is the case, -run flag is passed, so we 
    1. clear the trace file. (we no longer need the old trace file because C++ core has already parsed it)
    1.5 run the blktrace
    2. run ransomware
    2.5 close the blktrace
    3. dump the trace file 
    4. then we go back to the C++ core framework by terminating this process
    note : 4. also implies that in C++ framework, we need to wait for this process to terminate before we can continue
    """
    for arg in sys.argv[1:]:
        if arg.startswith("-run"):
            blktrace_dir = PATHS["blktrace_dir"]
            tar_sys_path = PATHS["tar_sys_path"]
            if os.path.exists(blktrace_dir):
                shutil.rmtree(blktrace_dir)
                os.mkdir(blktrace_dir)
            else:
                os.mkdir(blktrace_dir)
            run_ransomware(tar_sys_path, blktrace_dir, [BOOT_CONFIG["default_disk"]])
        else:
            print("Invalid flag:", arg)
            sys.exit(1)

if __name__ == '__main__':
    main()
    
    
    

def find_magic_1(dev_path,sec_num, bytes, sectors_set, dump_path):
    """ Debugging purpose function
    Given sector number produced by blkparse, we pipe that into dd command and dump the sector information.
    The sector number is given, and we want to dump `bytes` bytes information starting from the sector number.
    By dumping, I mean we scan the sector information and find the magic number 1, if we find it, we increment the counter unencrpyted by 1.
    """
    return
    # magic_1 = [MAGIC_NUM["MAGIC_NUM1_01"], MAGIC_NUM["MAGIC_NUM1_02"], MAGIC_NUM["MAGIC_NUM1_03"], MAGIC_NUM["MAGIC_NUM1_04"]]
    # unencrpyted = 0 # counter for unencrpyted bytes
    # # first we need to find the offset of the sector number
    # offset = sec_num * 512
    # # then we need to find the offset of the sector number + bytes
    # offset_end = offset + bytes
    # # now we need to find the sector number of the offset
    # sec_num_offset = offset // 512
    # # now we need to find the sector number of the offset_end
    # sec_num_offset_end = offset_end // 512

    # # now we use dd to dump the sector information
    # import subprocess
    # print(f"Dumping sector information from {sec_num_offset} to {sec_num_offset_end}...")
    # command = ['sudo', 'dd', 'if=' + dev_path, 'skip=' + str(sec_num_offset), 'count=' + str(sec_num_offset_end - sec_num_offset), 'status=none']
    
    # try:
    #     output = subprocess.check_output(command)
    #     # now we need to scan the output and find the magic number 1
    #     # magic number 1 is 4 bytes
    #     for i in range(0, len(output) - 3):
    #         if output[i] == magic_1[0] and output[i + 1] == magic_1[1] and output[i + 2] == magic_1[2] and output[i + 3] == magic_1[3]:
    #             unencrpyted += 1

    #     # append the output to dump_path
    #     with open(dump_path, "ab") as f:
    #         f.write(output)
    # except subprocess.CalledProcessError as e:
    #     print(e.output)
    #     sys.exit(1)

    # return unencrpyted

def parse_trace_file(blktrace_dir):
    return
    default_trace_file_path = BOOT_CONFIG["default_trace_file_path"]
    trace_path = BOOT_CONFIG["trace_path"]
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


def inject_debug_file(tar_sys_path):
    """
    Create a file for debugging purpose with length DEBUG_FILE_LEN bytes.
    Each byte is ASCII CHAR 'd'
    """
    return
    DEBUG_FILE_LEN = 6000
    debug_file_path = f"{tar_sys_path}/debug_file"
    with open(debug_file_path, "wb+") as f:
        for i in range(0, DEBUG_FILE_LEN):
            f.write(MAGIC_NUM["DEBUG_MAGIC"].encode('utf-8'))
