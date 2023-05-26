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
from config import PATHS, TAR_SYS_PARAMS, MOUNT_CONFIG
from config import FS_EXT4, FS_NTFS, BOOT_CONFIG
from preprocess import preprocess_tar_sys


dev_list = MOUNT_CONFIG['dev_list']
dev_used = []

fs_types = MOUNT_CONFIG['fs_types']
cfs_type = MOUNT_CONFIG['cfs_type']

def mount_dev(dev_path, mount_path, cfs_type):
    # check if mount_path is already mounted
    if os.path.ismount(mount_path):
        # unmount it
        os.system(f"sudo umount {mount_path}")
    # if mount_path does not exist, create it
    if not os.path.isdir(mount_path):
        os.mkdir(mount_path)
    if cfs_type == FS_EXT4:
        # mkfs.ext4
        os.system(f"sudo mkfs.ext4 {dev_path} && sudo mount -t ext4 {dev_path} {mount_path}")
    elif cfs_type == FS_NTFS:
        os.system(f"sudo mkfs.ntfs -f {dev_path} && sudo mount -t ntfs {dev_path} {mount_path}")
        

    dev_list.remove(dev_path)
    dev_used.append(dev_path)
    
def prepare_tar_sys(tar_sys_path):
    
    dup_tar_sys_path = f"_{tar_sys_path}"

    # Check if target system exists, if not, ask for user input
    if not os.path.isdir(dup_tar_sys_path):
        num_groups = TAR_SYS_PARAMS["num_groups"]
        num_users = TAR_SYS_PARAMS["num_users"]
        num_permissions = TAR_SYS_PARAMS["num_permissions"]
        os.system(f"python3 utils/gen_tar_sys.py {number_of_files} {median_length_of_files} {file_type_set} {dup_tar_sys_path} {framework_dir} {num_groups} {num_users} {num_permissions}")

    mount_dev(BOOT_CONFIG['default_disk'], tar_sys_path, cfs_type)

    # change the owner of the root_path to the current user
    os.system(f"sudo chown -R {os.getlogin()} {tar_sys_path}")

    # move all files in dup_tar_sys_path to tar_sys_path
    os.system(f"sudo mv {dup_tar_sys_path}/* {tar_sys_path}")

def handle_flags(tar_sys_path):
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
            os.system(f"sudo umount {tar_sys_path} && sudo rm -rf rans_test")
            sys.exit(0)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)     

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

handle_flags(tar_sys_path)

start = time.time()

prepare_tar_sys(tar_sys_path)

print(f"finish preparing target system using {time.time() - start} seconds")

# Run dump_tar_sys_info.py to dump info to corresponding files
os.system(f"python3 utils/dump_tar_sys_info.py {tar_sys_info_path} {tar_sys_path} {backup_dir_path} {number_of_files} {median_length_of_files} {file_type_set}")

# Duplicate directories
os.system(f"python3 utils/mkdup.py {duplicate_dir} {tar_sys_path} {backup_dir_path}")

# Display backup paths and confirm user input
print(f"Backup of target system is stored in {duplicate_dir}/duplicate_tar_sys")
print(f"Backup of backup directory is stored in {duplicate_dir}/duplicate_backup_dir")
print(f"Do you want to continue? with tar_sys_path = {tar_sys_path}, rans_path = {rans_path}, backup_dir_path = {backup_dir_path} (y/n)")
input_val = input()
if input_val != "y":
    sys.exit(0)

preprocess_tar_sys(tar_sys_path)

os.system("./core")

