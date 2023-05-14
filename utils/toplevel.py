import os
import shutil
import sys
import time

ENABLE_DEBUG = False

def debug(*args, **kwargs):
    if ENABLE_DEBUG:
        print(*args, **kwargs)


# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from access import clean_up_groups_users
from config import PATHS, TAR_SYS_PARAMS

framework_dir = PATHS["framework_dir"]
tar_sys_info_path = PATHS["tar_sys_info_path"]
duplicate_dir = PATHS["duplicate_dir"]
tar_sys_path = PATHS["tar_sys_path"]
rans_path = PATHS["rans_path"]
backup_dir_path = PATHS["backup_dir_path"]


# Check if we're in ransomware_set/utils directory
if not os.path.isdir("../utils"):
    print("Please run this script in ransomware_set/utils directory")
    sys.exit(1)

os.chdir("../")

# Check if rans_test directory exists, create if not
if not os.path.isdir(f"{framework_dir}"):
    os.mkdir(f"{framework_dir}")

# Create tar_sys_info directory if not exists
if not os.path.isdir(tar_sys_info_path):
    os.mkdir(tar_sys_info_path)


number_of_files = TAR_SYS_PARAMS["number_of_files"]
median_length_of_files = TAR_SYS_PARAMS["median_length_of_files"]
file_type_set = TAR_SYS_PARAMS["file_type_set"]

# Parse command-line arguments
for arg in sys.argv[1:]:
    if arg.startswith("-t=") or arg.startswith("--tar_sys_path="):
        tar_sys_path = arg.split("=", 1)[1]
    elif arg.startswith("-r=") or arg.startswith("--rans_path="):
        rans_path = arg.split("=", 1)[1]
    elif arg.startswith("-b=") or arg.startswith("--backup_dir_path="):
        backup_dir_path = arg.split("=", 1)[1]
    elif arg.startswith("--clean"):
        # remove rams_test directory with sudo and exit
        clean_up_groups_users(f"{framework_dir}") # first remove all groups and users created by gen_tar_sys.py
        os.system("sudo rm -rf rans_test")
        sys.exit(0)
    else:
        print("Invalid flag:", arg)
        sys.exit(1)
start_time = time.time()
# Check if target system exists, if not, ask for user input
if not os.path.isdir(tar_sys_path):
    num_groups = TAR_SYS_PARAMS["num_groups"]
    num_users = TAR_SYS_PARAMS["num_users"]
    num_permissions = TAR_SYS_PARAMS["num_permissions"]
    os.system(f"python3 utils/gen_tar_sys.py {number_of_files} {median_length_of_files} {file_type_set} {tar_sys_path} {framework_dir} {num_groups} {num_users} {num_permissions}")

debug(f"gen_tar_sys.py takes {time.time() - start_time} seconds")
start_time = time.time()
# Run dump_tar_sys_info.py to dump info to corresponding files
os.system(f"python3 utils/dump_tar_sys_info.py {tar_sys_info_path} {tar_sys_path} {backup_dir_path} {number_of_files} {median_length_of_files} {file_type_set}")
debug(f"dump_tar_sys_info.py takes {time.time() - start_time} seconds")
# Duplicate directories
start_time = time.time()
os.system(f"python3 utils/mkdup.py {duplicate_dir} {tar_sys_path} {backup_dir_path}")
debug(f"mkdup.py takes {time.time() - start_time} seconds")
# Display backup paths and confirm user input
debug(f"Backup of target system is stored in {duplicate_dir}/duplicate_tar_sys")
debug(f"Backup of backup directory is stored in {duplicate_dir}/duplicate_backup_dir")
print(f"Do you want to continue? with tar_sys_path = {tar_sys_path}, rans_path = {rans_path}, backup_dir_path = {backup_dir_path} (y/n)")
input_val = input()
if input_val != "y":
    sys.exit(0)


