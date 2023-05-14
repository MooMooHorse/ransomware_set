import os
import shutil
import sys
import time
import json

framework_dir = "rans_test"

PATHS = {
    "framework_dir": "rans_test",
    "tar_sys_info_path": f"{framework_dir}/tar_sys_info",
    "duplicate_dir": f"{framework_dir}/duplicates",
    "tar_sys_path": f"{framework_dir}/tar_sys",
    "rans_path": f"{framework_dir}/ransomware",
    "backup_dir_path": f"{framework_dir}/backup_dir",
}

TAR_SYS_PARAMS = {
    "number_of_files": 1000,
    "median_length_of_files": 1000,
    "file_type_set": 1,
    "num_groups" : 3,
    "num_users" : 1,
    "num_permissions" : 3,
    "MAGIC_NUM1_01" : 0xde,
    "MAGIC_NUM1_02" : 0xad,
    "MAGIC_NUM1_03" : 0xbe,
    "MAGIC_NUM1_04" : 0xef,
}


