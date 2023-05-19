import os
import shutil
import sys
import time
import json

framework_dir = "rans_test"

BOOT_CONFIG = {
    "blktrace_dir" : "blktrace_dir",
    "default_disk" : "/dev/vdb",
    "default_trace_file_path" : "blktrace_dir/tracefile",
    "core_dir" : "core_dir",
    "trace_path" : "blktrace_dir/blkparse_output"
}

# A set of paths used in the framework
PATHS = {
    "framework_dir": "rans_test",
    "tar_sys_info_path": f"{framework_dir}/tar_sys_info",
    "duplicate_dir": f"{framework_dir}/duplicates",
    "tar_sys_path": f"{framework_dir}/tar_sys",
    "rans_path": f"{framework_dir}/ransomware",
    "backup_dir_path": f"{framework_dir}/backup_dir",
}
# A set of parameters for the target system
TAR_SYS_PARAMS = {
    "number_of_files": 1000,
    "median_length_of_files": 1000,
    "file_type_set": 1,
    "num_groups" : 3,
    "num_users" : 1,
    "num_permissions" : 3,
    
}
# A set of magic numbers used in the framework
MAGIC_NUM = {
    # Magic number 1 is for the beginning and end of each file
    "MAGIC_NUM1_01" : 0xde,
    "MAGIC_NUM1_02" : 0xad,
    "MAGIC_NUM1_03" : 0xbe,
    "MAGIC_NUM1_04" : 0xef,
    # Magic number 2 is for the beginning and end of each file name and directory name
    "MAGIC_NUM2_01" : 0xca,
    "MAGIC_NUM2_02" : 0xfe,
    "MAGIC_NUM2_03" : 0xba,
    "MAGIC_NUM2_04" : 0x23,
    # Magic number 3 is for each byte of the file data
    "MAGIC_NUM3" : 0x41,
}

def main():
    args = []
    default_trace_file_path = BOOT_CONFIG["trace_path"]
    default_disk = BOOT_CONFIG["default_disk"]
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
        else:
            print("Invalid flag:", arg)
            sys.exit(1)
    return args

if __name__ == "__main__":
    args = main()
    # dump args delimited by space
    print(" ".join(args))