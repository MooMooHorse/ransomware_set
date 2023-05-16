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
    "MAGIC_NUM3" : 0x14,
}