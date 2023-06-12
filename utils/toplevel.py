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

# from access import clean_up_groups_users
from config import PATHS, TAR_SYS_PARAMS, MOUNT_CONFIG
from config import FS_EXT4, FS_NTFS, FS_F2FS, FS_EXT2, BOOT_CONFIG
from preprocess import preprocess_tar_sys


dev_list = MOUNT_CONFIG['dev_list']
dev_used = []

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
    elif cfs_type == FS_F2FS:
        os.system(f"sudo mkfs.f2fs -f {dev_path} && sudo mount -t f2fs {dev_path} {mount_path}")
    elif cfs_type == FS_EXT2:
        os.system(f"sudo mkfs.ext2 {dev_path} && sudo mount -t ext2 {dev_path} {mount_path}")
        

    dev_list.remove(dev_path)
    dev_used.append(dev_path)
    
def prepare_tar_sys(tar_sys_path, totsize, mu, fragscore):
    """re-construct the target system. Erase the original target system and create a new one.

    Args:
        tar_sys_path (string): path to the target system
        totsize (string): total size of the target system (approx.)
        mu (string): average peak of size distribution
        fragscore (string): fragmentation score
    """
    
    injectedSize = ['1%', '20%', '100%']
    injectedLimit = ['100 MB']
    
    mount_dev(BOOT_CONFIG['default_disk'], tar_sys_path, cfs_type)

    # change the owner of the root_path to the current user
    os.system(f"sudo chown -R {os.getlogin()} {tar_sys_path}")
    
    # remove all files in the target system
    os.system(f"sudo rm -rf {tar_sys_path}/*") # ALARM, high overhead, be sure to do this only several times (in current case 27 times, only 9 of which are done to 100GB system)
    
    
    
import subprocess

def prepare_backup_dir(tar_sys_path, backup_dir_path):
    mount_dev(BOOT_CONFIG['backup_disk'], backup_dir_path, cfs_type)
    os.system(f"sudo chown -R {os.getlogin()} {backup_dir_path}")
    for file in os.listdir(tar_sys_path):
        os.system(f"cp -rp {tar_sys_path}/{file} {backup_dir_path}")

def handle_flags(tar_sys_path, backup_dir_path):
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            # clean_up_groups_users(f"{framework_dir}") # first remove all groups and users created by gen_tar_sys.py
            # os.system(f"sudo umount {tar_sys_path} && sudo umount {backup_dir_path} && sudo rm -rf rans_test")
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

handle_flags(tar_sys_path, backup_dir_path)

number_of_files = TAR_SYS_PARAMS["number_of_files"]
median_length_of_files = TAR_SYS_PARAMS["median_length_of_files"]
file_type_set = TAR_SYS_PARAMS["file_type_set"]


totoalSysSize = ['10 MB', '1 GB', '100 GB']
mu = ['2', '9.28', '14'] # to be tuned
fragScore = ['0', '0.5', '1']

start = time.time()

for _totsize in totoalSysSize:
    for _mu in mu:
        for _fragscore in fragScore: 
            prepare_tar_sys(tar_sys_path, _totsize, _mu, _fragscore)
# prepare_backup_dir(tar_sys_path, backup_dir_path)

print(f"finish preparing target system using {time.time() - start} seconds")



# Display backup paths and confirm user input
print(f"Do you want to continue? with tar_sys_path = {tar_sys_path}, rans_path = {rans_path}, backup_dir_path = {backup_dir_path} (y/n)")
input_val = input()
if input_val != "y":
    sys.exit(0)

blktrace_dir = BOOT_CONFIG["blktrace_dir"]
backup_blktrace_dir = BOOT_CONFIG["backup_blktrace_dir"]
preprocess_tar_sys(tar_sys_path, blktrace_dir, BOOT_CONFIG["default_disk"])
# preprocess_backup_dir(backup_dir_path, backup_blktrace_dir, BOOT_CONFIG["backup_disk"])

os.system("./core")

