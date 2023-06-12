import os
import shutil
import sys
import time
import json

framework_dir = "rans_test"

# a set of chosen file system types
FS_EXT4 = 0
FS_NTFS = 1
FS_F2FS = 2
FS_EXT2 = 3

cfs_type = FS_EXT4

NORMAL_DISK = 0
BACKUP_DISK = 1
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
    "blktrace_dir" : "blktrace_dir",
    "default_disk" : MOUNT_CONFIG['dev_list'][XSMALL_DISK],
    "default_trace_file_path" : "blktrace_dir/tracefile",
    "core_dir" : "core_dir",
    "trace_path" : "blktrace_dir/blkparse_output",
    "backup_trace_path" : "backup_blktrace_dir/blkparse_output",
    "backup_blktrace_dir" : "backup_blktrace_dir",
    "backup_disk" : MOUNT_CONFIG['dev_list'][BACKUP_DISK],
}


tar_sys_info_path = os.path.join(framework_dir, "tar_sys_info")
tar_sys_dump_path = os.path.join(tar_sys_info_path, "tar_sys_dump")
tar_sys_path = os.path.join(framework_dir, "tar_sys")
tar_sys_injected_path = os.path.join(framework_dir, "injected")
backup_dir = os.path.join(framework_dir, "backup_dir")

# A set of paths used in the framework
PATHS = {
    "framework_dir": framework_dir,
    "tar_sys_info_path": tar_sys_info_path,
    "duplicate_dir": f"{framework_dir}/duplicates",
    "tar_sys_path": tar_sys_path,
    "injected_path": tar_sys_injected_path,
    "backup_dir_path": backup_dir,
    "tar_sys_dump_path": tar_sys_dump_path,
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
    # "MAGIC_NUM1_01" : 0xde,
    # "MAGIC_NUM1_02" : 0xad,
    # "MAGIC_NUM1_03" : 0xbe,
    # "MAGIC_NUM1_04" : 0xef,
    "MAGIC_NUM1_01" : 0x42,
    "MAGIC_NUM1_02" : 0x43,
    "MAGIC_NUM1_03" : 0x44,
    "MAGIC_NUM1_04" : 0x45,
    # Magic number 2 is for the end of the file and directory name
    # "MAGIC_NUM2_01" : 0xca,
    # "MAGIC_NUM2_02" : 0xfe,
    # "MAGIC_NUM2_03" : 0xba,
    # "MAGIC_NUM2_04" : 0x23,
    "MAGIC_NUM2_01" : 0x22,
    "MAGIC_NUM2_02" : 0x23,
    "MAGIC_NUM2_03" : 0x24,
    "MAGIC_NUM2_04" : 0x25,
    # Magic number 3 is for each byte of the file data
    "MAGIC_NUM3" : 0x41,
    'DEBUG_MAGIC' : 'd',
}

PYTHON_RANS_COMMAND = "python3 utils/cryptosoft/ransomware.py"
MINI_RANS_COMMAND = "./mini"


RANS_OPTIONS = {
    "cmd" : PYTHON_RANS_COMMAND,
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
    default_trace_file_path = BOOT_CONFIG["trace_path"]
    backup_trace_file_path = BOOT_CONFIG["backup_trace_path"]
    default_disk = BOOT_CONFIG["default_disk"]
    backup_disk = BOOT_CONFIG["backup_disk"]
    core_dir = BOOT_CONFIG["core_dir"]
    magic1 = ((MAGIC_NUM["MAGIC_NUM1_01"] << 24) + (MAGIC_NUM["MAGIC_NUM1_02"] << 16) + 
                (MAGIC_NUM["MAGIC_NUM1_03"] << 8) + (MAGIC_NUM["MAGIC_NUM1_04"]))
    magic2 = ((MAGIC_NUM["MAGIC_NUM2_01"] << 24) + (MAGIC_NUM["MAGIC_NUM2_02"] << 16) + 
        (MAGIC_NUM["MAGIC_NUM2_03"] << 8) + (MAGIC_NUM["MAGIC_NUM2_04"]))
    magic3 = MAGIC_NUM["MAGIC_NUM3"]
    for arg in sys.argv[1:]:
        if arg.startswith("-tfp"): # dump default_trace_file_path
            args.append(f"{default_trace_file_path}")
        elif arg.startswith("-dd"): # dump default_disk
            args.append(f"{default_disk}")
        elif arg.startswith("-cd"): # dump core_dir
            args.append(f"{core_dir}")
        elif arg.startswith("-ma"): # all magic numbers
            args.append(f"{magic1} {magic2} {magic3}")
        elif arg.startswith("-xma"): # all magic numbers in hex
            args.append(f"{hex(magic1)} {hex(magic2)} {hex(magic3)}")
        elif arg.startswith("-tsdp"): # target system dump path
            args.append(f"{PATHS['tar_sys_dump_path']}")
        elif arg.startswith("-en"): # encoding rule
            args.append(f"{encoding_rule}")
        elif arg.startswith("-bkuptfp"): # backup trace file path
            args.append(backup_trace_file_path)
        elif arg.startswith("-bkupdd"): # backup disk
            args.append(backup_disk)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)
    return args

if __name__ == "__main__":
    args = main()
    # dump args delimited by space
    print(" ".join(args))