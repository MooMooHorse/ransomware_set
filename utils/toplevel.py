import os
import shutil
import sys
import time

from access import clean_up_groups_users

framework_dir = "rans_test"
tar_sys_info_path = f"{framework_dir}/tar_sys_info"
duplicate_dir = f"{framework_dir}/duplicates"
tar_sys_path = f"{framework_dir}/tar_sys"
rans_path = f"{framework_dir}/ransomware"
backup_dir_path = f"{framework_dir}/backup_dir"


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


number_of_files = 1000
median_length_of_files = 1000
file_type_set = 1

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
    print("Target system does not exist. Please input <number of files> <median length of files> <file type set> to generate the target system:")
    number_of_files, median_length_of_files, file_type_set = input().split()
    os.system(f"python3 utils/gen_tar_sys.py {number_of_files} {median_length_of_files} {file_type_set} {tar_sys_path} {framework_dir}")
print(f"gen_tar_sys.py takes {time.time() - start_time} seconds")
start_time = time.time()
# Run dump_tar_sys_info.py to dump info to corresponding files
os.system(f"python3 utils/dump_tar_sys_info.py {tar_sys_info_path} {tar_sys_path} {backup_dir_path} {number_of_files} {median_length_of_files} {file_type_set}")
print(f"dump_tar_sys_info.py takes {time.time() - start_time} seconds")
# Duplicate directories
start_time = time.time()
os.system(f"python3 utils/mkdup.py {duplicate_dir} {tar_sys_path} {backup_dir_path}")
print(f"mkdup.py takes {time.time() - start_time} seconds")
# Display backup paths and confirm user input
print(f"Backup of target system is stored in {duplicate_dir}/duplicate_tar_sys")
print(f"Backup of backup directory is stored in {duplicate_dir}/duplicate_backup_dir")
print(f"Do you want to continue? with tar_sys_path = {tar_sys_path}, rans_path = {rans_path}, backup_dir_path = {backup_dir_path} (y/n)")
input_val = input()
if input_val != "y":
    sys.exit(0)


