import os
import shutil
import sys
import time
import json

framework_dir = "/home/h/rans_test"

cur_file_dir = os.path.dirname(os.path.realpath(__file__))
config_file_path = os.path.realpath(__file__)

impression_path = os.path.join(cur_file_dir, 'impressions')
impression_gen_path = os.path.join(os.path.join(impression_path, 'utils'), 'gen.py')
impression_draw_path = os.path.join(os.path.join(impression_path, 'utils'), 'draw.py')

core_path = os.path.join(cur_file_dir, 'core')

# a set of chosen file system types
FS_EXT4 = 0
FS_NTFS = 1
FS_F2FS = 2
FS_EXT2 = 3

cfs_type = FS_EXT4

NORMAL_DISK = 0
BACKUP_DISK = 1
LARGE_DISK = 2
XSMALL_DISK = 3

UTF8 = 0
UTF16 = 1

encoding_rule = UTF8

if cfs_type == FS_NTFS:
    encoding_rule = UTF16

MOUNT_CONFIG = {
    'dev_list' : ['/dev/vdb', '/dev/vdc', '/dev/vdd', '/dev/vde'],
    'cfs_type' : cfs_type,
    'enconding' : encoding_rule,
}

BOOT_CONFIG = {
    "default_disk" : MOUNT_CONFIG['dev_list'][NORMAL_DISK],
    "backup_disk" : MOUNT_CONFIG['dev_list'][BACKUP_DISK],
}


tar_sys_info_path = os.path.join(framework_dir, "tar_sys_info")
tar_sys_path = os.path.join(framework_dir, "tar_sys")
tar_sys_injected_path = os.path.join(tar_sys_path, "injected")
backup_dir = os.path.join(framework_dir, "backup_dir")
log_dir = os.path.join(framework_dir, "logs")
test_id_path = os.path.join(log_dir, 'test_id') # current test id is stored in this file
blktrace_dir = os.path.join(framework_dir, "blktrace")
blktrace_result = os.path.join(log_dir, "blktrace")
syscall_dir = os.path.join(cur_file_dir, 'utils', 'syscall', 'bin')
start_record_bin = os.path.join(syscall_dir, 'start')
end_record_bin = os.path.join(syscall_dir, 'end')
blk2file_bin = os.path.join(syscall_dir, 'blk2file')
clear_bin = os.path.join(syscall_dir, 'clear')
rans_path = os.path.join(cur_file_dir, 'utils', 'preprocess.py')
rans_exec_path = os.path.join(cur_file_dir, 'utils', 'cryptosoft', 'ransomware.py')

BATCH_BASE = 100000 # a base to distinguish between config file for original system and injected system

# A set of paths used in the framework
PATHS = {
    "framework_dir": framework_dir,
    "tar_sys_path": tar_sys_path,
    "injected_path": tar_sys_injected_path,
    "backup_dir_path": backup_dir,
    "impression_path": impression_path,
    "impression_gen_path": impression_gen_path,
    "impression_draw_path": impression_draw_path,
    "log_dir" : log_dir,
    "test_id_path" : test_id_path,
    "blktrace_dir" : blktrace_dir,
    "blktrace_result" : blktrace_result,
    "syscall_dir" : syscall_dir,
    "start_record_bin" : start_record_bin,
    "end_record_bin" : end_record_bin,
    "blk2file_bin" : blk2file_bin,
    "clear_bin" : clear_bin,
    "rans_path" : rans_path,
    "core_path" : core_path,
    'rans_exec_path' : rans_exec_path,
}



LOG_NAME = {
    'test_info' : 'test_info.log',
    'dye_info' : 'dye_info.log',
    'trace_info' : 'trace.log',
}

# A set of parameters for the target system
TAR_SYS_PARAMS = {
    "number_of_files": 100,
    "median_length_of_files": 100000,
    "file_type_set": 1,
    # "num_groups" : 3,
    # "num_users" : 1,
    # "num_permissions" : 3,
    
}
# A set of magic numbers used in the framework
MAGIC_NUM = {
    # Magic number 1 is for the beginning of the file and directory name
    "MAGIC_NUM1_01" : 0x42,
    "MAGIC_NUM1_02" : 0x43,
    "MAGIC_NUM1_03" : 0x44,
    "MAGIC_NUM1_04" : 0x45,
    # Magic number 2 is for the end of the file and directory name
    "MAGIC_NUM2_01" : 0x22,
    "MAGIC_NUM2_02" : 0x23,
    "MAGIC_NUM2_03" : 0x24,
    "MAGIC_NUM2_04" : 0x25,
    # Magic number 3 is for each byte of the file data
    "MAGIC_NUM3" : 0x41,
    'DEBUG_MAGIC' : 'd',
}


# ANSI escape sequences for different colors
colors = {
    'red': '\033[91m',
    'green': '\033[92m',
    'blue': '\033[94m',
    'yellow': '\033[93m',
    'reset': '\033[0m'
}

def print_red(msg):
    print(colors['red'] + msg + colors['reset'])

def print_green(msg):
    print(colors['green'] + msg + colors['reset'])
    
def print_blue(msg):
    print(colors['blue'] + msg + colors['reset'])
    
def print_yellow(msg):
    print(colors['yellow'] + msg + colors['reset'])


def main():
    args = []
    trace_log = os.path.join(PATHS["log_dir"], LOG_NAME["trace_info"])
    log_dir = PATHS["log_dir"]
    default_disk = BOOT_CONFIG["default_disk"]
    backup_disk = BOOT_CONFIG["backup_disk"]
    magic1 = ((MAGIC_NUM["MAGIC_NUM1_01"] << 24) + (MAGIC_NUM["MAGIC_NUM1_02"] << 16) + 
                (MAGIC_NUM["MAGIC_NUM1_03"] << 8) + (MAGIC_NUM["MAGIC_NUM1_04"]))
    magic2 = ((MAGIC_NUM["MAGIC_NUM2_01"] << 24) + (MAGIC_NUM["MAGIC_NUM2_02"] << 16) + 
        (MAGIC_NUM["MAGIC_NUM2_03"] << 8) + (MAGIC_NUM["MAGIC_NUM2_04"]))
    magic3 = MAGIC_NUM["MAGIC_NUM3"]
    for arg in sys.argv[1:]:
        if arg.startswith("-tfp"): # trace file path
            args.append(f"{trace_log}")
        elif arg.startswith("-dd"): # dump default_disk
            args.append(f"{default_disk}")
        elif arg.startswith("-log"): # dump log directory
            args.append(log_dir)
        elif arg.startswith("-ma"): # all magic numbers
            args.append(f"{magic1} {magic2} {magic3}")
        elif arg.startswith("-sys"): # dump system call related bins' path
            args.append(f"{start_record_bin} {end_record_bin} {blk2file_bin}")
        elif arg.startswith("-rans"): # ransomware path
            args.append(rans_path)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)
    return args

if __name__ == "__main__":
    args = main()
    # dump args delimited by space
    print(" ".join(args))